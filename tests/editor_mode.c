/*
 * The editor's 3D mode helper (source/editormode.c): renderer 3 falls back
 *  to classic when OpenGL is unavailable (the dummy driver has none) without
 *  touching the configured renderer, and the software renderers survive the
 *  8-bit mode set. Run with SDL_VIDEODRIVER=dummy and BUILD_WINDOWED=1.
 */
#include <stdio.h>
#include "platform.h"
#include "build.h"
#include "display.h"
#include "engine.h"
#include "editormode.h"

/* Callbacks the game normally provides to the engine. */
void timerhandler(void) { }
void keyhandler(void) { }
void faketimerhandler(void) { }

int main(int argc, char **argv)
{
    editormode_t m = { 3, 640, 480, 0 };
#ifdef POLYMOST
    int polymost = 1;
#else
    int polymost = 0;  /* every renderer is classic */
#endif
    int expect2 = polymost ? 2 : 0;

    _platform_init(argc, argv, "editor_mode", "editor_mode");

    if (editorsetgamemode(&m) != 0)
    {
        fprintf(stderr, "FAIL: editorsetgamemode(renderer 3) failed\n");
        return 1;
    }
    if ((getrendermode() != 0) || (editorrendermode != 0) || (m.renderer != 3) ||
        (xdim != 640) || (ydim != 480) || (frameplace == 0))
    {
        fprintf(stderr, "FAIL: renderer 3 gave rendermode=%d in use=%d configured=%d %dx%d\n",
                getrendermode(), (int) editorrendermode, (int) m.renderer,
                (int) xdim, (int) ydim);
        return 1;
    }

    m.renderer = 2;
    if ((editorsetgamemode(&m) != 0) || (getrendermode() != expect2) ||
        (editorrendermode != expect2))
    {
        fprintf(stderr, "FAIL: renderer 2 gave rendermode=%d in use=%d (expected %d)\n",
                getrendermode(), (int) editorrendermode, expect2);
        return 1;
    }

    if ((editorclamprenderer(-1) != 0) || (editorclamprenderer(9) != (polymost ? 3 : 0)) ||
        (editorclamprenderer(1) != (polymost ? 1 : 0)))
    {
        fprintf(stderr, "FAIL: editorclamprenderer\n");
        return 1;
    }

    _uninitengine();
    printf("PASS: editor_mode\n");
    return 0;
}
