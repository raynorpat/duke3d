/*
 * Smoke test for the SDL2 port of sdl_driver.c: brings up the video mode,
 *  sets a palette, pushes frames, pumps events and shuts down. It needs no
 *  game data. Run with SDL_VIDEODRIVER=dummy for headless CI.
 */
#include <stdio.h>
#include <string.h>
#if !PLATFORM_WIN32
#include <signal.h>
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

/* Room for 8K drawables in renderer 3. */
typedef char maxxdim_holds_8k[(MAXXDIM >= 7680) ? 1 : -1];
typedef char maxydim_holds_8k[(MAXYDIM >= 4320) ? 1 : -1];

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

int main(int argc, char **argv)
{
    char pal[256 * 4];
    char readback[256 * 4];
    unsigned long start;
    int i;

    if ((argc > 1) && (strcmp(argv[1], "gl_fallback") == 0))
        return gl_fallback(argc, argv);
    if ((argc > 1) && (strcmp(argv[1], "palette_fade") == 0))
        return palette_fade(argc, argv);

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
