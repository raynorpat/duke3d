#include "profile_test_support.h"
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
#include "displaymode.h"

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

#if !PLATFORM_WIN32
/* The engine's window: the only one. */
static SDL_Window *smoke_window(void)
{
    Uint32 id;

    for (id = 1; id < 1000; id++)
        if (SDL_GetWindowFromID(id) != NULL)
            return SDL_GetWindowFromID(id);
    return NULL;
}

/* The window's full-screen flags: 0, SDL_WINDOW_FULLSCREEN or _DESKTOP. */
static Uint32 smoke_fsflags(void)
{
    SDL_Window *w = smoke_window();
    return (w != NULL) ? (SDL_GetWindowFlags(w) & SDL_WINDOW_FULLSCREEN_DESKTOP) : 0xffffffff;
}

static void smoke_alt_enter(void)
{
    SDL_Event ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = SDL_KEYDOWN;
    ev.key.state = SDL_PRESSED;
    ev.key.keysym.scancode = SDL_SCANCODE_RETURN;
    ev.key.keysym.sym = SDLK_RETURN;
    ev.key.keysym.mod = KMOD_LALT;
    SDL_PushEvent(&ev);
    _handle_events();
}

/* The renderer presents on vsync. */
static int smoke_vsynced(void)
{
    SDL_RendererInfo info;
    SDL_Renderer *r = SDL_GetRenderer(smoke_window());

    return (r != NULL) && (SDL_GetRendererInfo(r, &info) == 0) &&
           ((info.flags & SDL_RENDERER_PRESENTVSYNC) != 0);
}

/* Full screen: sdl2-compat can report exclusive with the desktop flag. */
#define SMOKE_EXCLUSIVE() ((smoke_fsflags() & SDL_WINDOW_FULLSCREEN) != 0)

#define SMOKE_CHECK(what, cond) \
    do { if (!(cond)) { fprintf(stderr, "FAIL: %s (mode %d, flags 0x%x)\n", what, \
                                getvideodisplaymode(), (unsigned) smoke_fsflags()); return 1; } } while (0)
#endif

/*
 * setvideodisplaymode: the window's flags for each mode, at once when there's
 *  a window and at the next mode set when there isn't; BUILD_WINDOWED wins;
 *  Alt+Enter toggles windowed and the chosen full-screen style. setvideovsync
 *  sets the renderer's vsync. The dummy driver has a display mode of its own,
 *  so going full screen here changes no real display.
 */
static int displaymode_setter(int argc, char **argv)
{
#if !PLATFORM_WIN32
    unsetenv(BUILD_WINDOWED);
    SMOKE_CHECK("borderless by default", getvideodisplaymode() == DISPLAY_BORDERLESS);
    SMOKE_CHECK("vsync on by default", getvideovsync() == 1);
    setvideodisplaymode(DISPLAY_WINDOWED);
    SMOKE_CHECK("windowed asked for", getvideodisplaymode() == DISPLAY_WINDOWED);

    /* a new window made exclusive */
    setvideodisplaymode(DISPLAY_EXCLUSIVE);
    _platform_init(argc, argv, "sdl_driver_smoke", "smoke");
    if (_setgamemode(1, 320, 200) != 0)
    {
        fprintf(stderr, "FAIL: _setgamemode(320x200)\n");
        return 1;
    }
    SMOKE_CHECK("exclusive window", (getvideodisplaymode() == DISPLAY_EXCLUSIVE) && SMOKE_EXCLUSIVE());
    SMOKE_CHECK("vsync on", smoke_vsynced());

    setvideodisplaymode(DISPLAY_WINDOWED);
    SMOKE_CHECK("windowed", (getvideodisplaymode() == DISPLAY_WINDOWED) && (smoke_fsflags() == 0));
    smoke_alt_enter();  /* windowed set: borderless */
    SMOKE_CHECK("Alt+Enter to borderless", (getvideodisplaymode() == DISPLAY_BORDERLESS) &&
                                           (smoke_fsflags() == SDL_WINDOW_FULLSCREEN_DESKTOP));
    smoke_alt_enter();
    SMOKE_CHECK("Alt+Enter back to windowed", (getvideodisplaymode() == DISPLAY_WINDOWED) && (smoke_fsflags() == 0));

    setvideodisplaymode(DISPLAY_EXCLUSIVE);
    SMOKE_CHECK("exclusive", (getvideodisplaymode() == DISPLAY_EXCLUSIVE) && SMOKE_EXCLUSIVE());
    smoke_alt_enter();
    SMOKE_CHECK("Alt+Enter to windowed", (getvideodisplaymode() == DISPLAY_WINDOWED) && (smoke_fsflags() == 0));
    smoke_alt_enter();
    SMOKE_CHECK("Alt+Enter back to exclusive", (getvideodisplaymode() == DISPLAY_EXCLUSIVE) && SMOKE_EXCLUSIVE());

    setvideodisplaymode(DISPLAY_BORDERLESS);
    SMOKE_CHECK("borderless", (getvideodisplaymode() == DISPLAY_BORDERLESS) &&
                              (smoke_fsflags() == SDL_WINDOW_FULLSCREEN_DESKTOP));
    if (_setgamemode(1, 640, 400) != 0)
    {
        fprintf(stderr, "FAIL: _setgamemode(640x400)\n");
        return 1;
    }
    SMOKE_CHECK("borderless kept by a mode set", (getvideodisplaymode() == DISPLAY_BORDERLESS) &&
                                                 (smoke_fsflags() == SDL_WINDOW_FULLSCREEN_DESKTOP));

    /* no display mode fits: borderless for this run (the dummy display is 1024x768) */
    setvideodisplaymode(DISPLAY_EXCLUSIVE);
    SMOKE_CHECK("exclusive at 640x400", getvideodisplaymode() == DISPLAY_EXCLUSIVE);
    if (_setgamemode(1, 1600, 1200) != 0)
    {
        fprintf(stderr, "FAIL: _setgamemode(1600x1200)\n");
        return 1;
    }
    SMOKE_CHECK("exclusive falls back to borderless", (getvideodisplaymode() == DISPLAY_BORDERLESS) &&
                                                      (smoke_fsflags() == SDL_WINDOW_FULLSCREEN_DESKTOP));

    setvideovsync(0);
    SMOKE_CHECK("vsync off", (getvideovsync() == 0) && !smoke_vsynced());
    setvideovsync(1);
    SMOKE_CHECK("vsync back on", (getvideovsync() == 1) && smoke_vsynced());

    setenv(BUILD_WINDOWED, "1", 1);
    setvideodisplaymode(DISPLAY_EXCLUSIVE);
    SMOKE_CHECK("BUILD_WINDOWED wins", (getvideodisplaymode() == DISPLAY_WINDOWED) && (smoke_fsflags() == 0));

    _uninitengine();
#endif
    printf("PASS: displaymode_setter\n");
    return 0;
}

/*
 * A game mode's sizes (the VIDEO page's RESOLUTION and DISPLAY): windowed,
 *  a window the size asked for, drawn at that size up to 1600x1200 with the
 *  aspect kept (width a multiple of 4), small modes in a bigger window;
 *  borderless, drawn at the desktop's size (the dummy display is 1024x768);
 *  exclusive, drawn at the size asked for. setvideodisplaymodenext takes
 *  effect at the next mode set. A mode set at the same size leaves a moved
 *  window where it is.
 */
#if !PLATFORM_WIN32
static int smoke_mode(int mode, int w, int h, int wantmode, int xd, int yd, int ww, int wh)
{
    int gotw, goth;

    if (mode >= 0)
        setvideodisplaymodenext(mode);
    if (_setgamemode(1, w, h) != 0)
    {
        fprintf(stderr, "FAIL: _setgamemode(%dx%d)\n", w, h);
        return 0;
    }
    SDL_GetWindowSize(smoke_window(), &gotw, &goth);
    if ((getvideodisplaymode() != wantmode) || (xdim != xd) || (ydim != yd) ||
        ((ww > 0) && ((gotw != ww) || (goth != wh))))
    {
        fprintf(stderr, "FAIL: %dx%d in mode %d: mode %d, drawn at %ldx%ld, window %dx%d; "
                        "want mode %d, %dx%d, window %dx%d\n", w, h, mode, getvideodisplaymode(),
                (long) xdim, (long) ydim, gotw, goth, wantmode, xd, yd, ww, wh);
        return 0;
    }
    memset((void *) frameplace, 7, xdim * ydim);
    _nextpage();
    printf("%dx%d, mode %d: drawn at %ldx%ld, window %dx%d\n", w, h, wantmode,
           (long) xdim, (long) ydim, gotw, goth);
    return 1;
}

/* The window is centred on the dummy display (1024x768). */
static int smoke_centred(int w, int h)
{
    int x, y;

    SDL_GetWindowPosition(smoke_window(), &x, &y);
    if ((x != (1024 - w) / 2) || (y != (768 - h) / 2))
    {
        fprintf(stderr, "FAIL: a %dx%d window at %d,%d, not centred\n", w, h, x, y);
        return 0;
    }
    printf("%dx%d window centred at %d,%d\n", w, h, x, y);
    return 1;
}
#endif

static int mode_sizes(int argc, char **argv)
{
#if !PLATFORM_WIN32
    int x, y;

    unsetenv(BUILD_WINDOWED);
    setvideodisplaymode(DISPLAY_WINDOWED);
    setvideodrawfit(1);  /* the game's rule */
    _platform_init(argc, argv, "sdl_driver_smoke", "smoke");
    if (!smoke_mode(-1, 640, 480, DISPLAY_WINDOWED, 640, 480, 640, 480)) return 1;

    SDL_SetWindowPosition(smoke_window(), 50, 60);
    if (!smoke_mode(-1, 640, 480, DISPLAY_WINDOWED, 640, 480, 640, 480)) return 1;
    SDL_GetWindowPosition(smoke_window(), &x, &y);
    if ((x != 50) || (y != 60))
    {
        fprintf(stderr, "FAIL: a mode set at the same size moved the window to %d,%d\n", x, y);
        return 1;
    }
    printf("a mode set at the same size kept the window at %d,%d\n", x, y);

    if (!smoke_mode(-1, 2560, 1440, DISPLAY_WINDOWED, 1600, 900, 2560, 1440)) return 1;
    if (!smoke_mode(-1, 1200, 1600, DISPLAY_WINDOWED, 900, 1200, 1200, 1600)) return 1;
    if (!smoke_mode(-1, 1366, 768, DISPLAY_WINDOWED, 1366, 768, 1366, 768)) return 1;
    if (!smoke_mode(-1, 320, 200, DISPLAY_WINDOWED, 320, 200, 640, 400)) return 1;

    setvideodisplaymodenext(DISPLAY_BORDERLESS);
    if (getvideodisplaymode() != DISPLAY_WINDOWED)
    {
        fprintf(stderr, "FAIL: setvideodisplaymodenext changed the window before the mode set\n");
        return 1;
    }
    if (!smoke_mode(DISPLAY_BORDERLESS, 640, 480, DISPLAY_BORDERLESS, 1024, 768, 0, 0)) return 1;
    if (smoke_fsflags() != SDL_WINDOW_FULLSCREEN_DESKTOP)
        { fprintf(stderr, "FAIL: borderless flags 0x%x\n", (unsigned) smoke_fsflags()); return 1; }
    if (!smoke_mode(DISPLAY_EXCLUSIVE, 800, 600, DISPLAY_EXCLUSIVE, 800, 600, 0, 0)) return 1;
    if (!SMOKE_EXCLUSIVE())
        { fprintf(stderr, "FAIL: exclusive flags 0x%x\n", (unsigned) smoke_fsflags()); return 1; }
    if (!smoke_mode(DISPLAY_WINDOWED, 1024, 768, DISPLAY_WINDOWED, 1024, 768, 1024, 768)) return 1;
    if (smoke_fsflags() != 0)
        { fprintf(stderr, "FAIL: windowed flags 0x%x\n", (unsigned) smoke_fsflags()); return 1; }

    /* A size set while full screen: the window is centred when windowed again. */
    if (!smoke_mode(DISPLAY_WINDOWED, 320, 200, DISPLAY_WINDOWED, 320, 200, 640, 400)) return 1;
    if (!smoke_mode(DISPLAY_EXCLUSIVE, 800, 600, DISPLAY_EXCLUSIVE, 800, 600, 0, 0)) return 1;
    if (!smoke_mode(DISPLAY_WINDOWED, 800, 600, DISPLAY_WINDOWED, 800, 600, 800, 600)) return 1;
    if (!smoke_centred(800, 600)) return 1;
    if (!smoke_mode(DISPLAY_WINDOWED, 640, 480, DISPLAY_WINDOWED, 640, 480, 640, 480)) return 1;
    if (!smoke_mode(DISPLAY_BORDERLESS, 1000, 700, DISPLAY_BORDERLESS, 1024, 768, 0, 0)) return 1;
    if (!smoke_mode(DISPLAY_WINDOWED, 1000, 700, DISPLAY_WINDOWED, 1000, 700, 1000, 700)) return 1;
    if (!smoke_centred(1000, 700)) return 1;

    /* The editor's rule: over 1600x1200 clamped to it. */
    setvideodrawfit(0);
    if (!smoke_mode(-1, 1920, 1080, DISPLAY_WINDOWED, 1600, 1200, 1920, 1080)) return 1;
    setvideodrawfit(1);

    setenv(BUILD_WINDOWED, "1", 1);
    if (!smoke_mode(DISPLAY_EXCLUSIVE, 800, 600, DISPLAY_WINDOWED, 800, 600, 800, 600)) return 1;

    _uninitengine();
#endif
    printf("PASS: mode_sizes\n");
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

/* A frame shown with the fade timeline ms_a to ms_b (from t0) ago: its
 *  darkness is between videofade_at at either end (out: a, b capped at the
 *  hold), 1 if so. */
static int fade_shown_between(Uint32 t0, int held, const char *what)
{
    Uint32 a = SDL_GetTicks() - t0, b;
    int d, lo, hi;

    _nextpage();
    b = SDL_GetTicks() - t0;
    d = getvideofade();
    if (held) { a = (a > VIDEOFADE_MS) ? VIDEOFADE_MS : a; b = (b > VIDEOFADE_MS) ? VIDEOFADE_MS : b; }
    lo = videofade_at(a); hi = videofade_at(b);
    if (lo > hi) { int t = lo; lo = hi; hi = t; }
    if ((d < lo) || (d > hi))
    {
        fprintf(stderr, "FAIL: %s: darkness %d %u-%u ms in, not %d-%d\n", what, d, (unsigned) a, (unsigned) b, lo, hi);
        return 0;
    }
    return 1;
}

/*
 * The VIDEO page's fade around a change: videofade_at (the darkness by ms
 *  into it, 0 none to 256 black: out over VIDEOFADE_MS, then in), and the
 *  driver's run of it in real time over the frames shown: held black until
 *  setvideofadein, back from where it got to when asked out again mid-way,
 *  and done after.
 */
static int video_fade(int argc, char **argv)
{
    static const struct { uint32_t ms; int want; } at[] = {
        { 0, 0 }, { 125, 128 }, { 250, 256 }, { 375, 128 }, { 500, 0 }, { 5000, 0 }
    };
    Uint32 t0;
    int i, d;

    for (i = 0; i < (int) (sizeof (at) / sizeof (at[0])); i++)
        if (videofade_at(at[i].ms) != at[i].want)
            { fprintf(stderr, "FAIL: videofade_at(%u) %d, not %d\n", (unsigned) at[i].ms, videofade_at(at[i].ms), at[i].want); return 1; }
    for (i = 1; i <= 2 * VIDEOFADE_MS; i++)
        if ((i <= VIDEOFADE_MS) ? (videofade_at(i) < videofade_at(i - 1)) : (videofade_at(i) > videofade_at(i - 1)))
            { fprintf(stderr, "FAIL: videofade_at isn't out then in at %d ms\n", i); return 1; }

    _platform_init(argc, argv, "sdl_driver_smoke", "smoke");
    if (_setgamemode(1, 320, 200) != 0) { fprintf(stderr, "FAIL: _setgamemode(320x200)\n"); return 1; }
    memset((void *) frameplace, 31, xdim * ydim);
    _nextpage();
    if ((getvideofade() != 0) || (videofading())) { fprintf(stderr, "FAIL: a fade before one was asked for\n"); return 1; }

    setvideofadeout();
    t0 = SDL_GetTicks();
    while (SDL_GetTicks() - t0 < 2 * VIDEOFADE_MS)  /* out, then held */
    {
        if (!fade_shown_between(t0, 1, "out")) return 1;
        SDL_Delay(5);
    }
    if ((getvideofade() != 256) || (!videofading())) { fprintf(stderr, "FAIL: not held black\n"); return 1; }
    setvideofadeout();  /* asked again while black: stays */
    _nextpage();
    if (getvideofade() != 256) { fprintf(stderr, "FAIL: a second change undid the black\n"); return 1; }

    t0 = SDL_GetTicks() - VIDEOFADE_MS;
    setvideofadein();
    while (SDL_GetTicks() - t0 < VIDEOFADE_MS + VIDEOFADE_MS / 2)  /* half way in */
    {
        if (!fade_shown_between(t0, 0, "in")) return 1;
        SDL_Delay(5);
    }
    d = getvideofade();
    setvideofadeout();  /* turned back from about half way */
    _nextpage();
    if (abs(getvideofade() - d) > 64)  /* some ms on, not from 0 or 256 */
        { fprintf(stderr, "FAIL: turned back from %d to %d, not from where it was\n", d, getvideofade()); return 1; }
    printf("turned back at darkness %d: %d\n", d, getvideofade());
    for (t0 = SDL_GetTicks(); SDL_GetTicks() - t0 < VIDEOFADE_MS; SDL_Delay(5))
        _nextpage();
    _nextpage();  /* Sample after the deadline, even if the last delay crossed it. */
    if (getvideofade() != 256) { fprintf(stderr, "FAIL: not black again: %d\n", getvideofade()); return 1; }

    setvideofadein();
    for (t0 = SDL_GetTicks(); SDL_GetTicks() - t0 < VIDEOFADE_MS + 20; SDL_Delay(5))
        _nextpage();
    _nextpage();
    if ((getvideofade() != 0) || (videofading())) { fprintf(stderr, "FAIL: not done: %d\n", getvideofade()); return 1; }
    setvideofadein();  /* with none under way: nothing */
    _nextpage();
    if ((getvideofade() != 0) || (videofading())) { fprintf(stderr, "FAIL: a fade-in with none under way\n"); return 1; }

    /* No setvideofadein (frames shown by a loop that doesn't make the
     *  change): black held 2 * VIDEOFADE_MS, then faded in by itself. */
    setvideofadeout();
    for (t0 = SDL_GetTicks(); SDL_GetTicks() - t0 < 3 * VIDEOFADE_MS - 20; SDL_Delay(5))
        _nextpage();
    if (getvideofade() != 256) { fprintf(stderr, "FAIL: not held black: %d\n", getvideofade()); return 1; }
    for (t0 = SDL_GetTicks(); SDL_GetTicks() - t0 < VIDEOFADE_MS + 60; SDL_Delay(5))
        _nextpage();
    _nextpage();
    printf("held black with no fade-in: %d after %d ms\n", getvideofade(), 4 * VIDEOFADE_MS + 40);
    if ((getvideofade() != 0) || (videofading())) { fprintf(stderr, "FAIL: black held with no end\n"); return 1; }

    _uninitengine();
    printf("PASS: video_fade\n");
    return 0;
}

int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,"1.5")) return 1;
    char pal[256 * 4];
    char readback[256 * 4];
    unsigned long start;
    int i;

    if ((argc > 1) && (strcmp(argv[1], "gl_fallback") == 0))
        return gl_fallback(argc, argv);
    if ((argc > 1) && (strcmp(argv[1], "displaymode_setter") == 0))
        return displaymode_setter(argc, argv);
    if ((argc > 1) && (strcmp(argv[1], "mode_sizes") == 0))
        return mode_sizes(argc, argv);
    if ((argc > 1) && (strcmp(argv[1], "palette_fade") == 0))
        return palette_fade(argc, argv);
    if ((argc > 1) && (strcmp(argv[1], "screencapture_formats") == 0))
        return screencapture_formats(argc, argv);
    if ((argc > 1) && (strcmp(argv[1], "video_fade") == 0))
        return video_fade(argc, argv);

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
