/*
 * Smoke test for the SDL2 port of sdl_driver.c: brings up the video mode,
 *  sets a palette, pushes frames, pumps events and shuts down. It needs no
 *  game data. Run with SDL_VIDEODRIVER=dummy for headless CI.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !PLATFORM_WIN32
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include "platform.h"
#include "build.h"
#include "display.h"
#include "engine.h"

/* Callbacks the game normally provides to the engine. */
static volatile int timer_calls = 0;
void timerhandler(void) { timer_calls++; }
void keyhandler(void) { }
void faketimerhandler(void) { }

/* Room for 8K drawables in renderer 3; without it, the original limits. */
#ifdef POLYMOST
typedef char maxxdim_holds_8k[(MAXXDIM >= 7680) ? 1 : -1];
typedef char maxydim_holds_8k[(MAXYDIM >= 4320) ? 1 : -1];
#else
typedef char maxxdim_is_1600[(MAXXDIM == 1600) ? 1 : -1];
typedef char maxydim_is_1200[(MAXYDIM == 1200) ? 1 : -1];
#endif

/*
 * The dummy driver has no OpenGL, so setgamemode_gl must fail without
 *  exiting and leave the classic 8-bit mode usable.
 */
static int gl_fallback(int argc, char **argv)
{
    _platform_init(argc, argv, "sdl_driver_smoke", "smoke");
    if (_setgamemode(1, 320, 200) != 0)
    {
        fprintf(stderr, "FAIL: _setgamemode(320x200)\n");
        return 1;
    }

    if (setgamemode_gl(1) != -1)
    {
        fprintf(stderr, "FAIL: setgamemode_gl succeeded under the dummy driver\n");
        return 1;
    }

    if (setgamemode(2, 320, 200) != 0)
    {
        fprintf(stderr, "FAIL: setgamemode(2,320,200) after the GL fallback\n");
        return 1;
    }

    if ((xdim != 320) || (ydim != 200) || (frameplace == 0) || (getrendermode() != 0))
    {
        fprintf(stderr, "FAIL: after fallback xdim=%ld ydim=%ld frameplace=%ld rendermode=%d\n",
                (long) xdim, (long) ydim, (long) frameplace, getrendermode());
        return 1;
    }

    memset((void *) frameplace, 7, xdim * ydim);
    _nextpage();
    _uninitengine();
    printf("PASS: gl_fallback\n");
    return 0;
}

/*
 * setvideofullscreen picks fullscreen for the next mode set; BUILD_WINDOWED
 *  still wins over it.
 */
static int fullscreen_setter(int argc, char **argv)
{
#if !PLATFORM_WIN32
    unsetenv(BUILD_WINDOWED);
    setvideofullscreen(1);
    if (getvideofullscreen() != 1)
    {
        fprintf(stderr, "FAIL: setvideofullscreen(1) gave %d\n", getvideofullscreen());
        return 1;
    }

    setvideofullscreen(0);
    _platform_init(argc, argv, "sdl_driver_smoke", "smoke");
    if (_setgamemode(1, 320, 200) != 0)
    {
        fprintf(stderr, "FAIL: _setgamemode(320x200)\n");
        return 1;
    }

    if (getvideofullscreen() != 0)
    {
        fprintf(stderr, "FAIL: windowed request, fullscreen=%d\n", getvideofullscreen());
        return 1;
    }

    setenv(BUILD_WINDOWED, "1", 1);
    setvideofullscreen(1);
    if (getvideofullscreen() != 0)
    {
        fprintf(stderr, "FAIL: BUILD_WINDOWED did not win, fullscreen=%d\n", getvideofullscreen());
        return 1;
    }

    _uninitengine();
#endif
    printf("PASS: fullscreen_setter\n");
    return 0;
}

/*
 * Outside renderer 3, setpalettefade blends the last setbrightness palette
 *  toward a colour exactly as the game's palto does, then applies the
 *  brightness table.
 */
extern char britable[16][64];

static int palette_fade(int argc, char **argv)
{
    unsigned char pal[768], want[768];
    char plain[1024], expect[1024], got[1024];
    int i, k;

    _platform_init(argc, argv, "sdl_driver_smoke", "smoke");
    if (setgamemode(2, 320, 200) != 0)
    {
        fprintf(stderr, "FAIL: setgamemode(2,320,200)\n");
        return 1;
    }

    /* no TABLES.DAT here: a brightness table that isn't the identity. */
    for (i = 0; i < 64; i++)
        britable[5][i] = (char) ((i * 3 / 4) + 8);

    for (i = 0; i < 256; i++)
    {
        pal[i*3+0] = (unsigned char) (i & 63);
        pal[i*3+1] = (unsigned char) ((i * 3) & 63);
        pal[i*3+2] = (unsigned char) ((i * 7) & 63);
    }
    for (k = 0; k < 768; k++)
    {
        int t = ((k % 3) == 0) ? 63 : 0;
        want[k] = (unsigned char) (pal[k] + (((t - (int) pal[k]) * 32) >> 6));
    }

    setbrightness(5, want);
    VBE_getPalette(0, 256, expect);

    setbrightness(5, pal);
    VBE_getPalette(0, 256, plain);
    setpalettefade(63, 0, 0, 32);
    VBE_getPalette(0, 256, got);

    if (memcmp(expect, got, sizeof (got)) != 0)
    {
        fprintf(stderr, "FAIL: setpalettefade(63,0,0,32) differs from palto's blend\n");
        return 1;
    }
    if (memcmp(plain, got, sizeof (got)) == 0)
    {
        fprintf(stderr, "FAIL: setpalettefade(63,0,0,32) left the palette unchanged\n");
        return 1;
    }

    setpalettefade(0, 0, 0, 0);
    VBE_getPalette(0, 256, got);
    if (memcmp(plain, got, sizeof (got)) != 0)
    {
        fprintf(stderr, "FAIL: setpalettefade(0,0,0,0) isn't the unfaded palette\n");
        return 1;
    }

    _uninitengine();
    printf("PASS: palette_fade\n");
    return 0;
}

static int file_head(const char *name, unsigned char *buf, int n)
{
    FILE *f = fopen(name, "rb");
    int got;
    if (f == NULL)
        return 0;
    got = (int) fread(buf, 1, n, f);
    fclose(f);
    return got;
}

#define LE32(p) ((p)[0] | ((p)[1] << 8) | ((p)[2] << 16) | ((p)[3] << 24))
#define BE32(p) (((p)[0] << 24) | ((p)[1] << 16) | ((p)[2] << 8) | (p)[3])

/*
 * screencapture writes PNG, JPEG or BMP by the file name's extension, from
 *  the 8-bit frame through the palette on screen. The BMP is uncompressed,
 *  so its pixels are checked against the palette.
 */
static int screencapture_formats(int argc, char **argv)
{
    static const unsigned char pngsig[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
    unsigned char head[64], *bmp;
    char pal[256 * 4];
    int i, w, h, stride, n;
    FILE *f;

    _platform_init(argc, argv, "sdl_driver_smoke", "smoke");
    if (setgamemode(2, 320, 200) != 0)
    {
        fprintf(stderr, "FAIL: setgamemode(2,320,200)\n");
        return 1;
    }

    memset(pal, 0, sizeof (pal));
    pal[5*4+0] = 10; pal[5*4+1] = 20; pal[5*4+2] = 30;  /* BGR, 0-63 */
    pal[9*4+0] = 63; pal[9*4+1] = 0;  pal[9*4+2] = 33;
    VBE_setPalette(0, 256, pal);
    memset((void *) frameplace, 5, xdim * ydim);
    ((unsigned char *) frameplace)[0] = 9;  /* top left */

    remove("smoke0000.png"); remove("smoke0000.jpg"); remove("smoke0000.bmp");
    remove("smoke0001.bmp");
    if ((screencapture("smokexxxx.png", 0) != 0) ||
        (screencapture("smokexxxx.jpg", 0) != 0) ||
        (screencapture("smokexxxx.bmp", 0) != 0))
    {
        fprintf(stderr, "FAIL: screencapture returned an error\n");
        return 1;
    }

    n = file_head("smoke0000.png", head, 64);
    if ((n < 24) || (memcmp(head, pngsig, 8) != 0) ||
        (BE32(head + 16) != xdim) || (BE32(head + 20) != ydim))
    {
        fprintf(stderr, "FAIL: smoke0000.png isn't a %ldx%ld PNG\n", (long) xdim, (long) ydim);
        return 1;
    }

    n = file_head("smoke0000.jpg", head, 64);
    if ((n < 3) || (head[0] != 0xFF) || (head[1] != 0xD8) || (head[2] != 0xFF))
    {
        fprintf(stderr, "FAIL: smoke0000.jpg doesn't start with FF D8 FF\n");
        return 1;
    }

    n = file_head("smoke0000.bmp", head, 64);
    if ((n < 54) || (head[0] != 'B') || (head[1] != 'M'))
    {
        fprintf(stderr, "FAIL: smoke0000.bmp doesn't start with BM\n");
        return 1;
    }
    w = LE32(head + 18);
    h = LE32(head + 22);
    if ((w != xdim) || (h != ydim))
    {
        fprintf(stderr, "FAIL: smoke0000.bmp is %dx%d\n", w, h);
        return 1;
    }

    stride = (w * 3 + 3) & ~3;
    bmp = (unsigned char *) malloc(54 + stride * h);
    f = fopen("smoke0000.bmp", "rb");
    if ((bmp == NULL) || (f == NULL) || (fread(bmp, 1, 54 + stride * h, f) != (size_t) (54 + stride * h)))
    {
        fprintf(stderr, "FAIL: smoke0000.bmp is short\n");
        return 1;
    }
    fclose(f);

    /* VBE_setPalette scales 0-63 to 0-255 as v/63*255; BMP rows run bottom-up, pixels BGR. */
    {
        const unsigned char *top = bmp + 54 + (h - 1) * stride;
        const unsigned char *next = top + 3;
        const unsigned char want0[3] = { 255, 0, (unsigned char) (33 / 63.0 * 255.0) };
        const unsigned char want1[3] = { (unsigned char) (10 / 63.0 * 255.0),
                                         (unsigned char) (20 / 63.0 * 255.0),
                                         (unsigned char) (30 / 63.0 * 255.0) };
        if ((memcmp(top, want0, 3) != 0) || (memcmp(next, want1, 3) != 0))
        {
            fprintf(stderr, "FAIL: BMP pixels %d,%d,%d / %d,%d,%d\n",
                    top[0], top[1], top[2], next[0], next[1], next[2]);
            return 1;
        }
    }

    /* A capture with setpalettefade on has the faded colours, as the screen
     * does (brightness 0 as the identity: no TABLES.DAT here). The name
     * taken, it's smoke0001. Index 5 is 30,20,10 faded halfway to 63,0,0. */
    {
        unsigned char gamepal[768];
        const unsigned char *next = bmp + 54 + (h - 1) * stride + 3;
        const unsigned char want[3] = { (unsigned char) (5 / 63.0 * 255.0),
                                        (unsigned char) (10 / 63.0 * 255.0),
                                        (unsigned char) (46 / 63.0 * 255.0) };

        for (i = 0; i < 64; i++)
            britable[0][i] = (char) i;
        memset(gamepal, 0, sizeof (gamepal));
        gamepal[5*3+0] = 30; gamepal[5*3+1] = 20; gamepal[5*3+2] = 10;
        setbrightness(0, gamepal);
        setpalettefade(63, 0, 0, 32);
        f = NULL;
        if ((screencapture("smokexxxx.bmp", 0) != 0) || ((f = fopen("smoke0001.bmp", "rb")) == NULL) ||
            (fread(bmp, 1, 54 + stride * h, f) != (size_t) (54 + stride * h)))
        {
            fprintf(stderr, "FAIL: a second capture didn't write a whole smoke0001.bmp\n");
            return 1;
        }
        fclose(f);
        remove("smoke0001.bmp");
        if (memcmp(next, want, 3) != 0)
        {
            fprintf(stderr, "FAIL: faded BMP pixel %d,%d,%d, want %d,%d,%d\n",
                    next[0], next[1], next[2], want[0], want[1], want[2]);
            return 1;
        }
    }
    free(bmp);

    for (i = 0; i < 3; i++)
        remove(i == 0 ? "smoke0000.png" : i == 1 ? "smoke0000.jpg" : "smoke0000.bmp");

#if !PLATFORM_WIN32
    /* A file that can't be written whole (a 64-byte file size limit): an
     * error, and no file left behind. */
    {
        struct rlimit old, lim;

        signal(SIGXFSZ, SIG_IGN);
        getrlimit(RLIMIT_FSIZE, &old);
        lim = old;
        lim.rlim_cur = 64;
        setrlimit(RLIMIT_FSIZE, &lim);
        i = screencapture("smokexxxx.bmp", 0);
        setrlimit(RLIMIT_FSIZE, &old);
        f = fopen("smoke0000.bmp", "rb");
        if (f != NULL)
            fclose(f);
        remove("smoke0000.bmp");
        if ((i != -1) || (f != NULL))
        {
            fprintf(stderr, "FAIL: a short write returned %d and %s the file\n", i,
                    (f != NULL) ? "left" : "removed");
            return 1;
        }
    }
#endif

    _uninitengine();
    printf("PASS: screencapture_formats\n");
    return 0;
}

int main(int argc, char **argv)
{
    char pal[256 * 4];
    char readback[256 * 4];
    unsigned long start;
    int i;

    if ((argc > 1) && (strcmp(argv[1], "gl_fallback") == 0))
        return gl_fallback(argc, argv);
    if ((argc > 1) && (strcmp(argv[1], "fullscreen_setter") == 0))
        return fullscreen_setter(argc, argv);
    if ((argc > 1) && (strcmp(argv[1], "palette_fade") == 0))
        return palette_fade(argc, argv);
    if ((argc > 1) && (strcmp(argv[1], "screencapture_formats") == 0))
        return screencapture_formats(argc, argv);

    _platform_init(argc, argv, "sdl_driver_smoke", "smoke");
    if (_setgamemode(1, 320, 200) != 0)
    {
        fprintf(stderr, "FAIL: _setgamemode(320x200)\n");
        return 1;
    }

    if ((xdim != 320) || (ydim != 200) || (frameplace == 0))
    {
        fprintf(stderr, "FAIL: mode vars xdim=%ld ydim=%ld frameplace=%ld\n",
                (long) xdim, (long) ydim, (long) frameplace);
        return 1;
    }

    for (i = 0; i < 256; i++)  /* BUILD's BGRx format, 0-63 per channel */
    {
        pal[i*4+0] = (char) (i >> 2);          /* blue */
        pal[i*4+1] = (char) (63 - (i >> 2));   /* green */
        pal[i*4+2] = (char) ((i * 7) & 63);    /* red */
        pal[i*4+3] = 0;
    }
    if (VBE_setPalette(0, 256, pal) != 1)
    {
        fprintf(stderr, "FAIL: VBE_setPalette\n");
        return 1;
    }

    memset(readback, 0, sizeof (readback));
    VBE_getPalette(0, 256, readback);
    /*
     * 0-63 -> 0-255 -> 0-63 truncates, so allow off-by-one; a swapped or
     *  dropped channel is off by far more than that.
     */
    for (i = 0; i < 256 * 4; i++)
    {
        int diff = (int) readback[i] - (int) pal[i];
        if ((i % 4 != 3) && ((diff < -1) || (diff > 1)))
        {
            fprintf(stderr, "FAIL: palette entry %d channel %d: wrote %d, read %d\n",
                    i / 4, i % 4, (int) pal[i], (int) readback[i]);
            return 1;
        }
    }

    /* A partial update must land at its own offset and leave the rest alone. */
    for (i = 0; i < 16; i++)
    {
        pal[i*4+0] = (char) (63 - i);          /* blue */
        pal[i*4+1] = (char) (i * 3);           /* green */
        pal[i*4+2] = (char) (32 + i);          /* red */
        pal[i*4+3] = 0;
    }
    if (VBE_setPalette(16, 16, pal) != 1)
    {
        fprintf(stderr, "FAIL: VBE_setPalette(16, 16)\n");
        return 1;
    }

    memset(readback, 0, sizeof (readback));
    VBE_getPalette(0, 256, readback);
    for (i = 0; i < 256 * 4; i++)
    {
        int entry = i / 4;
        int diff;
        if (i % 4 == 3)
            continue;
        if ((entry >= 16) && (entry < 32))
            diff = (int) readback[i] - (int) pal[(entry - 16) * 4 + (i % 4)];
        else
        {
            int orig = (i % 4 == 0) ? (entry >> 2) :
                       (i % 4 == 1) ? (63 - (entry >> 2)) : ((entry * 7) & 63);
            diff = (int) readback[i] - orig;
        }
        if ((diff < -1) || (diff > 1))
        {
            fprintf(stderr, "FAIL: after partial update, palette entry %d channel %d: read %d\n",
                    entry, i % 4, (int) readback[i]);
            return 1;
        }
    }

    for (i = 0; i < 30; i++)
    {
        memset((void *) frameplace, i * 8, xdim * ydim);
        _nextpage();
    }
    _handle_events();

    inittimer();
    start = getticks();
    while (getticks() - start < 1000)
        _idle();
    uninittimer();

    /* The game clock runs at TICRATE (120), so ~120 calls in a second; a
     *  10ms SDL timer gave 100. The callback catches up on missed ticks, so
     *  only a timer thread starved for 80ms at the very end reads low. */
    if ((timer_calls < 110) || (timer_calls > 121))
    {
        fprintf(stderr, "FAIL: timer fired %d times in 1000ms (expected 110-121)\n",
                timer_calls);
        return 1;
    }

#if !PLATFORM_WIN32
    /*
     * After a stall (a debugger, Ctrl-Z, App Nap) the clock must carry on
     *  from where it is, not replay every tick it missed: that fast-forwards
     *  the game. Stop the whole process for a second, then count.
     */
    {
        pid_t child;
        int before;

        timer_calls = 0;
        inittimer();
        start = getticks();
        while (getticks() - start < 100)
            _idle();
        child = fork();
        if (child == 0)
        {
            sleep(1);
            kill(getppid(), SIGCONT);
            _exit(0);
        }
        before = timer_calls;
        raise(SIGSTOP);
        waitpid(child, NULL, 0);
        start = getticks();
        while (getticks() - start < 100)
            _idle();
        uninittimer();
        if (timer_calls - before > 60)
        {
            fprintf(stderr, "FAIL: timer fired %d times in the 100ms after a 1s stall (expected under 60)\n",
                    timer_calls - before);
            return 1;
        }
    }
#endif

    _uninitengine();
    printf("PASS: sdl_driver smoke test\n");
    return 0;
}
