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

/* Set once OpenGL failed this session: renderer 3 then uses classic untried. */
extern int32_t editorglfailed;

/* A renderer from build.cfg or -renderer: 0 (classic) to 3 (OpenGL); always
 * 0 without Polymost. */
int32_t editorclamprenderer(int32_t r);

/* Set once Alt+K picked a renderer: build.cfg then saves editormode's. */
extern int32_t editorrendererpicked;

/* Alt+K: the renderer after current, 0 to 3 and round, skipping 3 when
 * glavailable is 0; always 0 without Polymost. */
int editorcyclerenderer(int current, int glavailable);

/* Sets the 8-bit 3D mode at m's size, then OpenGL for renderer 3 (classic if
 * it's unavailable), then the renderer. m is left as asked. 0 ok, -1 when the
 * 8-bit mode can't be set. */
int editorsetgamemode(const editormode_t *m);

#endif
