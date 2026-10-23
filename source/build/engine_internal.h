/* Private declarations shared by the engine implementation modules. */
#ifndef ENGINE_INTERNAL_H
#define ENGINE_INTERNAL_H

#include <stdint.h>

void engine_loadtables(void);
int32_t engine_render_sqrt(uint32_t value);
int32_t engine_recip(int32_t value);
int32_t getclipmask(int i1, int i2, int i3, int i4);
extern short radarang[1280];
extern int32_t pow2long[32];
extern char britable[16][64];

int animateoffs(short tilenum, short fakevar);
void engine_tiles_uninit(void);
#if defined(USE_I386_ASM)
int setgotpic(int32_t value);
#else
void setgotpic(uint32_t value);
#endif
extern char picsiz[MAXTILES];
extern int32_t totalclocklock;
#ifdef SUPERBUILD
#define MAXVOXELS 512
#define MAXVOXMIPS 5
extern intptr_t voxoff[MAXVOXELS][MAXVOXMIPS];
extern int32_t voxlock[MAXVOXELS][MAXVOXMIPS];
#endif

void engine_loadpalette(void);
extern int32_t viewoffset, viewingrangerecip, pixelaspect;
extern int32_t stereopixelwidth, ostereopixelwidth;
extern char *transluc, paletteloaded;
extern char *globalpalwritten;
extern int32_t globalpal;
extern char kensmessage[128];

#if defined(POLYMOST) && defined(USE_OPENGL)
/* Binds the shader frame as needed before checking its colour representation. */
int engine_gl_indexedtarget(void);
void engine_gl_2dcolor(int32_t col);
void engine_gl_lookupchanged(void);
void engine_gl_resetview(void);
#endif

#endif
