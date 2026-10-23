/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */

#ifndef _INCLUDE_DISPLAY_H_
#define _INCLUDE_DISPLAY_H_

#include <stdint.h>
#include "platform.h"
#include "displaymode.h"

#if (defined PLATFORM_SUPPORTS_SDL)
#include "SDL.h"

/* environment variables names... */
/* These are only honored by the SDL driver. */
#define BUILD_NOMOUSEGRAB    "BUILD_NOMOUSEGRAB"
#define BUILD_WINDOWED       "BUILD_WINDOWED"
#define BUILD_SDLDEBUG       "BUILD_SDLDEBUG"
#define BUILD_RENDERER       "BUILD_RENDERER"
#define BUILD_GLLIBRARY      "BUILD_GLLIBRARY"
#define BUILD_USERSCREENRES  "BUILD_USERSCREENRES"
#define BUILD_MAXSCREENRES   "BUILD_MAXSCREENRES"
#define BUILD_HALLOFMIRRORS  "BUILD_HALLOFMIRRORS"
#define BUILD_GLDUMP         "BUILD_GLDUMP"
#define BUILD_SDLJOYSTICK    "BUILD_SDLJOYSTICK"

/* This part sucks. */

#endif  /* defined PLATFORM_SUPPORTS_SDL */

/* set these in your _platform_init() implementation. */
extern int _argc;
extern char **_argv;

/* !!! gads, look at all the namespace polution... */
extern int32_t xres, yres, bytesperline, imageSize, maxpages;
extern char *screen, vesachecked;
extern int32_t buffermode, origbuffermode, linearmode;
extern char permanentupdate, vgacompatible;
extern char moustat;
extern int32_t *horizlookup, *horizlookup2, horizycent;
extern int32_t oxdimen, oviewingrange, oxyaspect;
extern int32_t curbrightness;
extern int32_t qsetmode;
extern intptr_t frameplace;
extern int32_t pageoffset, ydim16;
extern char textfont[1024], smalltextfont[1024];
extern char pow2char[8];
extern volatile int32_t stereomode, visualpage, activepage, whiteband, blackband;
extern int32_t searchx, searchy;
extern int32_t wx1, wy1, wx2, wy2, ydimen;
extern int32_t xdimen, xdimenrecip, halfxdimen, xdimenscale, xdimscale;

/*
 * !!! used to be static. If we ever put the original setgamemode() back, this
 * !!! can be made static again.  --ryan.
 * !!! (Actually, most of these extern declarations used to be static...rcg.)
 */
extern unsigned char permanentlock;


/* these need to be implemented by YOUR driver. */
void _platform_init(int argc, char **argv, const char *title, const char *icon);
void _idle(void);
void _handle_events(void);
void *_getVideoBase(void);
void initkeys(void);
void uninitkeys(void);
void _nextpage(void);
void _uninitengine(void);
void _joystick_init(void);
void _joystick_deinit(void);
int _joystick_update(void);
int _joystick_axis(int axis);
int _joystick_hat(int hat);
int _joystick_button(int button);

/*
 * VESA replacement code: The Unix (not-actually-VESA) version of this is
 *  originally using SDL (Simple Directmedia Layer: http://www.libsdl.org/),
 *  and is stored in sdl_window.c, but there's no reason another driver
 *  couldn't be dropped in, so long as it implements these functions. Please
 *  reference sdl_window.c and ves2.h (the original code) for all the nuances
 *  and global variables that need to get set up correctly.
 */
void getvalidvesamodes(void);
int VBE_getPalette(int32_t start, int32_t num, char *dapal);
int VBE_setPalette(int32_t start, int32_t num, char *palettebuffer);
int setvesa(int32_t x, int32_t y);
void uninitvesa(void);
void setvmode(int mode);
unsigned char readpixel(intptr_t offset);
void drawpixel(intptr_t offset, Uint8 pixel);
void drawpixels(int32_t offset, Uint16 pixels);
void drawpixelses(int32_t offset, Uint32 pixelses);
void drawpixel16(int32_t offset);
void fillscreen16 (int32_t input1, int32_t input2, int32_t input3);
void limitrate(void);
void setactivepage(int32_t dapagenum);
void clear2dscreen(void);
void _updateScreenRect(int32_t x, int32_t y, int32_t w, int32_t h);

/* Mouse/key input is implemented in sdl_input.c; view dimensions in sdl_window.c. */
int setupmouse(void);
/* The size readmousexy's motion is in: the 8-bit mode's, or in OpenGL the
 *  window's points, which a high-DPI drawable (xdim, ydim) outnumbers; the
 *  frame's part of them when the shader path draws at an internal size. */
void getmousearea(int *w, int *h);
void readmousexy(short *x, short *y);
void readmousebstatus(short *bstatus);
void keyhandler(void);
unsigned char _readlastkeyhit(void);

/* timer krap. */
void inittimer(void);
void uninittimer(void);

/* this is implemented in the game, and your driver calls it. */
void timerhandler(void);

/* Resolution initialization, in sdl_window.c. */
int _setgamemode(char davidoption, int32_t daxdim, int32_t daydim);
/* Switches to an OpenGL window for renderer 3. Returns 0, or -1 (and leaves
 *  the 8-bit modes usable) when SDL or OpenGL can't provide one. */
int setgamemode_gl(char davidoption);
/* DISPLAY_* (displaymode.h): now when there's a window, else at the next mode
 *  set. BUILD_WINDOWED forces windowed. Alt+Enter toggles windowed and the
 *  full-screen style set here (borderless when windowed is set). */
void setvideodisplaymode(int mode);
/* The same, set by the next mode set together with its size: one change on
 *  screen (the VIDEO page's DISPLAY and RESOLUTION). */
void setvideodisplaymodenext(int mode);
/* 1: game modes' 8-bit frames follow the game's VIDEO rule (borderless at
 *  the window's size, over 1600x1200 capped keeping the aspect); 0 (the
 *  default, the editor's): the size asked for, over 1600x1200 clamped. */
void setvideodrawfit(int on);
int getvideodisplaymode(void);  /* the window's */
/* Alt+Enter calls hook with the display mode to go to instead of changing it
 *  (the game makes it a mode set); NULL (the default): it changes now. */
void setvideotogglehook(void (*hook)(int mode));
/* The main display's modes as SDL lists them (w[i] by h[i], at most max) and
 *  its desktop size; returns how many. */
int getvideomodelist(int32_t *w, int32_t *h, int max, int32_t *desktopw, int32_t *desktoph);
/*
 * The VIDEO page's fade around a change, at the present, over the game's own
 *  palette and fade (left alone), in real time. setvideofadeout fades to
 *  black over VIDEOFADE_MS (from where a fade-in has got to) and holds it;
 *  setvideofadein then fades back in over VIDEOFADE_MS, as it does by
 *  itself after 2 * VIDEOFADE_MS held (frames shown with nothing to make
 *  the change). getvideofade: the
 *  last frame shown's darkness, 0 (none) to 256 (black); videofading: one
 *  is under way. videofade_at: the darkness ms into one never held.
 */
#define VIDEOFADE_MS 250
void setvideofadeout(void);
void setvideofadein(void);
int getvideofade(void);
int videofading(void);
int videofade_at(uint32_t ms);
/*
 * Renderer 3's supersampling (glext.h glsupersample) set to level now, the
 *  frame made again at its size, keeping the palette and fade as a resize
 *  does; 1 if it's in use. videosupersampleavailable: whether level can be
 *  used now (off always; the others on the shader path without an internal
 *  size, at a size larger than the window's: videoopts_ssavailable).
 */
int setvideosupersample(int level);
int videosupersampleavailable(int level);
/*
 * Renderer 3's texture filter (glext.h gltexturefilter, videoopts.h
 *  TEXFILTER_*) set to filter now: the 1.1/1.5 paths' textures take it at
 *  once, the shader path draws with it from its next frame (a switch to or
 *  from nearest starts that frame black). 1 if it's in use now.
 *  videotexfilteravailable: whether filter can be used now (nearest always;
 *  the others in renderer 3, on the shader path when its filtering shaders
 *  link).
 */
int setvideotexfilter(int filter);
int videotexfilteravailable(int filter);
void setvideovsync(int on);  /* 1: present on vsync (the default) */
int getvideovsync(void);
/* Renderer 3: draws the last frame shown, unfaded, as the new frame's start. */
void glredrawlastframe(void);
#ifdef USE_OPENGL
/* Renderer 3: the engine drew to the back buffer since the last swap. */
void _glframedrawn(void);
/* Renderer 3: what was drawn since the swap only repeats the frame shown. */
void _glframereplayed(void);
/* Renderer 3: show palfadergb/palfadedelta now if no frame is being drawn. */
void _glpalettefade(void);
#endif
void qsetmode640350(void);
void qsetmode640480(void);

uint32_t getticks();

void drawline16(int32_t XStart, int32_t YStart, int32_t XEnd, int32_t YEnd, char Color);
void setcolor16(int i1);

#endif  /* _INCLUDE_DISPLAY_H_ */

/* end of display.h ... */


