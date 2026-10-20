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
    editormode_t m = { 3, 640, 480, 2 };
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
#ifdef USE_OPENGL
    if (gltexmiplevel != 2)
    {
        fprintf(stderr, "FAIL: gltexmiplevel=%d (expected 2)\n", (int) gltexmiplevel);
        return 1;
    }

    /* GL failed once, so the next trip back to 3D doesn't try it again. */
    if (editorglfailed != 1)
    {
        fprintf(stderr, "FAIL: editorglfailed=%d after the fallback\n", (int) editorglfailed);
        return 1;
    }
    editorglfailed = 2;  /* a retry that failed would set it back to 1 */
    if ((editorsetgamemode(&m) != 0) || (getrendermode() != 0) || (editorglfailed != 2))
    {
        fprintf(stderr, "FAIL: second renderer 3 call gave rendermode=%d glfailed=%d\n",
                getrendermode(), (int) editorglfailed);
        return 1;
    }
#endif

    /* 8-bit modes keep the engine's 1600x1200 limit. */
    m.renderer = 0;
    m.xdim3d = 1920;
    m.ydim3d = 1080;
    if ((editorsetgamemode(&m) != 0) || (xdim != 1600) || (ydim != 1200))
    {
        fprintf(stderr, "FAIL: 1920x1080 in renderer 0 gave %dx%d\n", (int) xdim, (int) ydim);
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

    /* Alt+K: 0, 1, 2, 3 and round, 3 skipped without OpenGL; classic only
     * without Polymost. */
    if ((editorcyclerenderer(0, 1) != (polymost ? 1 : 0)) ||
        (editorcyclerenderer(1, 1) != (polymost ? 2 : 0)) ||
        (editorcyclerenderer(2, 1) != (polymost ? 3 : 0)) ||
        (editorcyclerenderer(3, 1) != 0) ||
        (editorcyclerenderer(2, 0) != 0) ||
        (editorcyclerenderer(0, 0) != (polymost ? 1 : 0)))
    {
        fprintf(stderr, "FAIL: editorcyclerenderer\n");
        return 1;
    }

    _uninitengine();
    printf("PASS: editor_mode\n");
    return 0;
}
