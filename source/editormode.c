/*
 * The editor's 3D video mode with its renderer, as the game's
 *  setgamemoderenderer: going to 2D drops OpenGL, so 3D sets it again.
 */
#include <stdio.h>
#include "build/build.h"
#include "build/engine.h"
#include "build/display.h"
#include "editormode.h"
#include "SDL.h"

int32_t editorrendermode;

int32_t editorclamprenderer(int32_t r)
{
#ifndef POLYMOST
    (void) r;
    return 0;
#else
    if (r < 0) return 0;
    if (r > 3) return 3;
    return r;
#endif
}

int editorsetgamemode(const editormode_t *m)
{
    if (setgamemode(vidoption, m->xdim3d, m->ydim3d) < 0)
        return -1;
    editorrendermode = editorclamprenderer(m->renderer);
#ifdef USE_OPENGL
    gltexmiplevel = m->gltexmiplevel;  /* build.cfg: drop this many mipmap levels */
#endif
    if (editorrendermode == 3 && setgamemode_gl(vidoption) != 0)
    {
        printf("OpenGL renderer unavailable (%s); using the classic renderer.\n", SDL_GetError());
        setgamemode(vidoption, m->xdim3d, m->ydim3d);
        editorrendermode = 0;
    }
#ifdef USE_OPENGL
    if (editorrendermode == 3)
        polymosttexfullbright = 240;  /* lights glow, as in the game's palettes */
#endif
    setrendermode(editorrendermode);
    return 0;
}
