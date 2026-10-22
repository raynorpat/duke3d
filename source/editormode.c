/*
 * The editor's 3D video mode with its renderer, as the game's
 *  setgamemoderenderer: going to 2D drops OpenGL, so 3D sets it again.
 */
#include <stdio.h>
#include "build/build.h"
#include "build/engine.h"
#include "build/display.h"
#include "build/glext.h"
#include "editormode.h"
#include "SDL.h"

int32_t editorrendermode;
int32_t editorglfailed;
int32_t editorrendererpicked;
int32_t editormousesensitivity = 65536;

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

int editorcyclerenderer(int current, int glavailable)
{
#ifndef POLYMOST
    (void) current;
    (void) glavailable;
    return 0;
#else
    int next = (current + 1) & 3;

    if ((next == 3) && !glavailable)
        next = 0;
    return next;
#endif
}

int32_t editormousemove(int32_t d, int32_t pixels, int32_t points, int32_t *rem)
{
    const int64_t unit = 2 << 16;  /* /2 as the old searchx += mousx>>1 */
    int64_t v = (int64_t) d * editormousesensitivity * pixels / points + *rem;
    int64_t q = v / unit;

    if ((v % unit) < 0)
        q--;  /* round down, as >>1 does */
    *rem = (int32_t) (v - q * unit);
    return (int32_t) q;
}

int editorsetgamemode(const editormode_t *m)
{
    if (setgamemode(vidoption, m->xdim3d, m->ydim3d) < 0)
        return -1;
    editorrendermode = editorclamprenderer(m->renderer);
#ifdef USE_OPENGL
    gltexmiplevel = m->gltexmiplevel;  /* build.cfg: drop this many mipmap levels */
    gldrawpathrequest = m->gldrawpath;  /* setgamemode_gl picks what the driver has */
    glinternalwidth = m->glinternalwidth;
    glinternalheight = m->glinternalheight;
    glsupersample = m->supersampling;
#endif
    if (editorrendermode == 3 && editorglfailed)
        editorrendermode = 0;
    if (editorrendermode == 3 && setgamemode_gl(vidoption) != 0)
    {
        printf("OpenGL renderer unavailable (%s); using the classic renderer.\n", SDL_GetError());
        setgamemode(vidoption, m->xdim3d, m->ydim3d);
        editorrendermode = 0;
        editorglfailed = 1;
    }
#ifdef USE_OPENGL
    if (editorrendermode == 3)
        polymosttexfullbright = 240;  /* lights glow, as in the game's palettes */
#endif
    setrendermode(editorrendermode);
    return 0;
}
