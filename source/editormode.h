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

/* build.cfg's mousesensitivity in 16.16 (65536 is 1.0). */
extern int32_t editormousesensitivity;

/* The cursor's move for mouse motion d: d * sensitivity / 2, scaled by
 * pixels/points (drawable over window size), with what's under a pixel kept
 * in *rem for the next call. At 1.0 and pixels == points it's d>>1 for d a
 * multiple of 2. */
int32_t editormousemove(int32_t d, int32_t pixels, int32_t points, int32_t *rem);

/* Sets the 8-bit 3D mode at m's size, then OpenGL for renderer 3 (classic if
 * it's unavailable), then the renderer. m is left as asked. 0 ok, -1 when the
 * 8-bit mode can't be set. */
int editorsetgamemode(const editormode_t *m);

#endif
