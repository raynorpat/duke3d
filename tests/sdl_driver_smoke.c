/*
 * Smoke test for the SDL2 port of sdl_driver.c: brings up the video mode,
 *  sets a palette, pushes frames, pumps events and shuts down. It needs no
 *  game data. Run with SDL_VIDEODRIVER=dummy for headless CI.
 */
#include <stdio.h>
#include <string.h>
#include "platform.h"
#include "build.h"
#include "display.h"
#include "engine.h"

/* Callbacks the game normally provides to the engine. */
static volatile int timer_calls = 0;
void timerhandler(void) { timer_calls++; }
void keyhandler(void) { }
void faketimerhandler(void) { }

int main(int argc, char **argv)
{
    char pal[256 * 4];
    char readback[256 * 4];
    unsigned long start;
    int i;

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
    while (getticks() - start < 500)
        _idle();
    uninittimer();

    /* PLATFORM_TIMER_HZ is 100, so ~50 calls in 500ms; SDL2 at 1ms would be ~500. */
    if ((timer_calls < 25) || (timer_calls > 100))
    {
        fprintf(stderr, "FAIL: timer fired %d times in 500ms (expected ~50)\n",
                timer_calls);
        return 1;
    }

    _uninitengine();
    printf("PASS: sdl_driver smoke test\n");
    return 0;
}
