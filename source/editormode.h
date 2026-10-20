#ifndef EDITORMODE_H
#define EDITORMODE_H

#include <stdint.h>

/* The editor's 3D mode: build.cfg's renderer (or -renderer's), 3D size and
 * mipmap levels dropped. */
typedef struct {
    int32_t renderer, xdim3d, ydim3d, gltexmiplevel;
} editormode_t;

extern editormode_t editormode;  /* build.c: this run's */

/* The renderer in use: the asked-for one, or 0 when OpenGL was unavailable. */
extern int32_t editorrendermode;

/* A renderer from build.cfg or -renderer: 0 (classic) to 3 (OpenGL); always
 * 0 without Polymost. */
int32_t editorclamprenderer(int32_t r);

/* Sets the 8-bit 3D mode at m's size, then OpenGL for renderer 3 (classic if
 * it's unavailable), then the renderer. m is left as asked. 0 ok, -1 when the
 * 8-bit mode can't be set. */
int editorsetgamemode(const editormode_t *m);

#endif
