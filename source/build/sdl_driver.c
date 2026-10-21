/*
 * An SDL replacement for BUILD's VESA code.
 *
 *  Written by Ryan C. Gordon. (icculus@clutteredmind.org)
 *
 * Please do NOT harrass Ken Silverman about any code modifications
 *  (including this file) to BUILD.
 */

/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef PLATFORM_WIN32
#include <sys/param.h>
#endif
#include "platform.h"

#if (!defined PLATFORM_SUPPORTS_SDL)
#error This platform apparently does not use SDL. Do not compile this.
#endif

#include "SDL.h"
#include "build.h"
#include "display.h"
#include "pragmas.h"
#include "engine.h"
#ifdef USE_OPENGL
#include "SDL_opengl.h"
#include "glext.h"
#define GLSUB_WRAP
#include "glsubmit.h"
#endif

/* need VirtualProtect() from win32 API... */
#if ((defined PLATFORM_WIN32) && (defined USE_I386_ASM))
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#endif

typedef enum
{
    RENDERER_SOFTWARE,
    RENDERER_TOTAL
} sdl_renderer_type;

const char *renderer_name[RENDERER_TOTAL];

#define ENVRSTR_RENDERER_SOFTWARE  "software"

static sdl_renderer_type renderer = RENDERER_SOFTWARE;

/* !!! ugh. Clean this up. */
#if (!defined __WATCOMC__)
#include "a.h"
#else
extern int32_t setvlinebpl(int32_t);
#pragma aux setvlinebpl parm [eax];
#endif  /* __WATCOMC__ */

#include "cache1d.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


/*
 * !!! I'd like this to be temporary. --ryan.
 * !!!  That is, the self-modifying part, so I can ditch the mprotect() stuff.
 */
#if ((defined PLATFORM_UNIX) && (defined USE_I386_ASM))

#include <sys/mman.h>
#include <limits.h>

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

#endif

#if (defined PLATFORM_WIN32)
#include <malloc.h>  /* alloca() */
#ifndef alloca
#define alloca _alloca
#endif
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#ifndef S_IRGRP
#define S_IRGRP 0
#endif
#ifndef S_IROTH
#define S_IROTH 0
#endif
#endif


/*
 * !!! remove the surface_end checks, for speed's sake. They are a
 * !!!  needed safety right now. --ryan.
 */

#define DEFAULT_MAXRESWIDTH  1600
#define DEFAULT_MAXRESHEIGHT 1200


#define UNLOCK_SURFACE_AND_RETURN  if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface); return;

static char vgapal16[48] =
{
        00,00,00,00,00,42,00,42,00,00,42,42,42,00,00,42,00,42,42,21,00,42,42,42,
        21,21,21,21,21,63,21,63,21,21,63,63,63,21,21,63,21,63,63,63,21,63,63,63,
};


int _argc = 0;
char **_argv = NULL;

    /* !!! move these elsewhere? */
int32_t xres, yres, bytesperline, imageSize, maxpages;
intptr_t frameplace;
char *screen, vesachecked;
int32_t buffermode, origbuffermode, linearmode;
char permanentupdate = 0, vgacompatible;

#ifdef POLYMOST
int32_t bpp = 8;
#endif
#ifdef USE_OPENGL
struct glinfo_t glinfo;
char nofog = 0;
extern void polymost_glreset(void);
extern int32_t polymost_present_shader(const int32_t *r, int32_t dw, int32_t dh);
extern void polymost_prepare_shader(void);
extern unsigned char *polymost_capture_shader(void);
extern GLuint polymost_takeframe_shader(void);
extern void polymost_putframe_shader(GLuint tex);
extern void polymost_unbind_shader(void);
extern unsigned char *getbasepalette(void);
extern void gltexinvalidateall(void);
extern void polymost_buildshaderamp(void);
/*
 * Made at the first renderer 3 and kept, with its window, until shutdown:
 *  Apple's GL on Metal can leave the heap damaged when a context is deleted.
 *  The 8-bit modes are shown through it from then on.
 */
static SDL_GLContext sdl_glcontext = NULL;
static int gl_drawing = 0;  /* renderer 3 is drawing in it */
static GLuint gl8tex = 0;  /* an 8-bit mode's frame, as shown through it */
static int gl8tex_w, gl8tex_h;  /* its texture size */
static float gl8xrel = 0, gl8yrel = 0;  /* mouse motion under a pixel, carried */
static char gl_davidoption = 1;
static int gl_window_w = 640;  /* windowed size: the last 8-bit mode's, unclamped */
static int gl_window_h = 480;
static int framedrawn = 0;  /* something was drawn since the last swap */
static GLuint lastframe = 0;  /* the last swapped frame, before its fade */
static int lastframe_set = 0;  /* lastframe holds a frame yet */
static int swapped = 0;  /* a swap has shown something in this mode */
static int lastframe_w, lastframe_h;  /* its texture size */
static int gl_draw_w, gl_draw_h;  /* the drawable */
static int gl_scaled = 0;  /* the shader path draws at its internal size (xres, yres)... */
static int32_t gl_showrect[4];  /* ...shown here in the drawable (gl_fitrect) */
#endif

SDL_Surface *surface = NULL; /* This isn't static so that we can use it elsewhere AH */
static SDL_Window *sdl_window = NULL;
static SDL_Renderer *sdl_renderer = NULL;
static SDL_Texture *sdl_texture = NULL;
static SDL_Surface *sdl_rgbsurface = NULL;  /* 32-bit staging copy of surface */
static int debug_hall_of_mirrors = 0;
static int sdl_displaymode = DISPLAY_BORDERLESS;  /* the window's */
static int sdl_displaymode_wanted = DISPLAY_BORDERLESS;  /* unless BUILD_WINDOWED */
static int sdl_vsync = 1;
static int exclusive_w = 0, exclusive_h = 0;  /* exclusive's size: the game mode's */
static int32_t mouse_x = 0;
static int32_t mouse_y = 0;
static int32_t mouse_relative_x = 0;
static int32_t mouse_relative_y = 0;
static short mouse_buttons = 0;
static int mouse_grabbed = 0;
static unsigned int lastkey = 0;
static SDL_TimerID primary_timer = 0;
/* so we can make use of setcolor16()... - DDOI */
static unsigned char drawpixel_color=0;
/* These hold the colors for the 256 color palette when in 16-color mode - DDOI */
static char backup_palette[48];
static int in_egapalette = 0;
/* The standard EGA palette in BUILD format - DDOI */
static char egapalette[] = { 0, 0, 0,
			    0, 0, 42,
			    0, 42, 0,
			    0, 42, 42,
			    42, 0, 0,
			    42, 0, 42,
			    42, 21, 0,
			    42, 42, 42,
			    21, 21, 21,
			    21, 21, 63,
			    21, 63, 21,
			    21, 63, 63,
			    63, 21, 21,
			    63, 21, 63,
			    63, 63, 21,
			    63, 63, 63
			   };

static unsigned int scancodes[SDL_NUM_SCANCODES];

static int32_t last_render_ticks = 0;
int32_t total_render_time = 1;
int32_t total_rendered_frames = 0;

static char *titlelong = NULL;
static char *titleshort = NULL;

void restore256_palette (void);
void set16color_palette (void);


static FILE *_sdl_debug_file = NULL;

#ifdef DC
#define sdldebug(fmt,args...) printf("BUILDSDL: " fmt "\n",##args)
#else
static __inline void __out_sdldebug(const char *subsystem,
                                  const char *fmt, va_list ap)
{
    fprintf(_sdl_debug_file, "%s: ", subsystem);
    vfprintf(_sdl_debug_file, fmt, ap);
    fprintf(_sdl_debug_file, "\n");
    fflush(_sdl_debug_file);
} /* __out_sdldebug */


static void sdldebug(const char *fmt, ...)
{
    va_list ap;

    if (_sdl_debug_file)
    {
        va_start(ap, fmt);
        __out_sdldebug("BUILDSDL", fmt, ap);
        va_end(ap);
    } /* if */
} /* sdldebug */
#endif


static void output_surface_info(SDL_Surface *_surface)
{
    SDL_RendererInfo info;

    if (!_sdl_debug_file)
        return;

    if (_surface == NULL)
    {
        sdldebug("-WARNING- You've got a NULL screen surface!");
        return;
    } /* if */

    sdldebug("screen surface is (%dx%dx%dbpp), %s.",
            _surface->w, _surface->h, _surface->format->BitsPerPixel,
            display_modename(sdl_displaymode));

    if ((sdl_renderer != NULL) && (SDL_GetRendererInfo(sdl_renderer, &info) == 0))
        sdldebug("SDL renderer is \"%s\".", info.name);
} /* output_surface_info */


static void output_driver_info(void)
{
    const char *driver;

    if (!_sdl_debug_file)
        return;

    sdldebug("Using BUILD renderer \"%s\".", renderer_name[renderer]);

    driver = SDL_GetCurrentVideoDriver();
    if (driver == NULL)
        sdldebug("-WARNING- SDL_GetCurrentVideoDriver() returned NULL!");
    else
        sdldebug("Using SDL video driver \"%s\".", driver);
} /* output_driver_info */


#ifdef USE_OPENGL
static void gl_present8(void);
#endif

/* Push the 8-bit surface through the palette to the window. */
static void present_surface(void)
{
#ifdef USE_OPENGL
    if (sdl_glcontext != NULL)
    {
        gl_present8();
        return;
    } /* if */
#endif
    if ((surface == NULL) || (sdl_texture == NULL))
        return;

    SDL_BlitSurface(surface, NULL, sdl_rgbsurface, NULL);
    SDL_UpdateTexture(sdl_texture, NULL, sdl_rgbsurface->pixels,
                      sdl_rgbsurface->pitch);
    SDL_RenderClear(sdl_renderer);
    SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
    SDL_RenderPresent(sdl_renderer);
} /* present_surface */


static void free_video_surfaces(void)
{
#ifdef USE_OPENGL
    if (gl8tex != 0)  /* the context is current while there is one */
        glDeleteTextures(1, &gl8tex);
    gl8tex = 0;
#endif

    if (sdl_texture != NULL)
    {
        SDL_DestroyTexture(sdl_texture);
        sdl_texture = NULL;
    } /* if */

    if (sdl_rgbsurface != NULL)
    {
        SDL_FreeSurface(sdl_rgbsurface);
        sdl_rgbsurface = NULL;
    } /* if */

    if (surface != NULL)
    {
        SDL_FreeSurface(surface);
        surface = NULL;
    } /* if */
} /* free_video_surfaces */


#ifdef USE_OPENGL
/* Leave renderer 3: drop its GL textures. The context and window stay. */
static void gl_leave(void)
{
    if (!gl_drawing)
        return;

    polymost_glreset();
    glsub_uninit();
    if (lastframe != 0)
        glDeleteTextures(1, &lastframe);
    lastframe = 0;
    framedrawn = 0;
    gl_drawing = 0;
    bpp = 8;
    numpages = 0;
    setrendermode(0);
} /* gl_leave */


/* The texture an 8-bit mode's w by h frame is shown from (GL 1.1 without
 *  GL_ARB_texture_non_power_of_two: rounded up to powers of two). */
static void gl_alloc8(int w, int h)
{
    gl8tex_w = w;
    gl8tex_h = h;
    if (!glinfo.texnpot)
    {
        for (gl8tex_w = 1; gl8tex_w < w; gl8tex_w <<= 1) ;
        for (gl8tex_h = 1; gl8tex_h < h; gl8tex_h <<= 1) ;
    } /* if */
    glGenTextures(1, &gl8tex);
    glBindTexture(GL_TEXTURE_2D, gl8tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gl8tex_w, gl8tex_h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    gl8xrel = gl8yrel = 0;
} /* gl_alloc8 */


/*
 * Shows the 8-bit surface through the GL context, as the SDL renderer did:
 *  through its palette (faded and brightened as the engine set it), scaled
 *  to fit the drawable, nearest pixel, with black bars. Renderer 3 may have
 *  left any state, so what the draw depends on is set here.
 */
static void gl_present8(void)
{
    int32_t r[4];
    int dw, dh;
    float u, v;

    if ((surface == NULL) || (gl8tex == 0) || (gl_drawing))
        return;

    SDL_BlitSurface(surface, NULL, sdl_rgbsurface, NULL);  /* the palette applied */
    SDL_GL_GetDrawableSize(sdl_window, &dw, &dh);
    gl_fitrect(surface->w, surface->h, dw, dh, r);
    u = (float) surface->w / (float) gl8tex_w;
    v = (float) surface->h / (float) gl8tex_h;

    if (bglBindFramebuffer != NULL)
        bglBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (bglUseProgram != NULL)
        bglUseProgram(0);
    if (bglActiveTexture != NULL)
        bglActiveTexture(GL_TEXTURE0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_FOG);
    glColorMask(1, 1, 1, 1);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glViewport(0, 0, dw, dh);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(r[0], r[1], r[2], r[3]);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, gl8tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, surface->w, surface->h,
                    GL_RGBA, GL_UNSIGNED_BYTE, sdl_rgbsurface->pixels);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);  /* the surface's top row at the top */
    glTexCoord2f(0, v); glVertex2f(-1, -1);
    glTexCoord2f(u, v); glVertex2f(1, -1);
    glTexCoord2f(u, 0); glVertex2f(1, 1);
    glTexCoord2f(0, 0); glVertex2f(-1, 1);
    glEnd();
    SDL_GL_SwapWindow(sdl_window);
} /* gl_present8 */


/*
 * An 8-bit mode shown through the GL context: a mouse motion's window
 *  points to the frame's pixels, as the SDL renderer's logical size did,
 *  the relative motion's part pixel carried to the next.
 */
static void gl_mouse8(SDL_MouseMotionEvent *m)
{
    int32_t r[4];
    int ww, wh;
    float sx, sy;

    SDL_GetWindowSize(sdl_window, &ww, &wh);
    gl_fitrect(surface->w, surface->h, ww, wh, r);
    if ((r[2] <= 0) || (r[3] <= 0))
        return;
    sx = (float) surface->w / (float) r[2];
    sy = (float) surface->h / (float) r[3];
    m->x = (Sint32) ((m->x - r[0]) * sx);
    m->y = (Sint32) ((m->y - r[1]) * sy);
    gl8xrel += m->xrel * sx;
    gl8yrel += m->yrel * sy;
    m->xrel = (Sint32) gl8xrel;
    m->yrel = (Sint32) gl8yrel;
    gl8xrel -= m->xrel;
    gl8yrel -= m->yrel;
} /* gl_mouse8 */
#endif


static void shutdown_video(void)
{
    free_video_surfaces();
#ifdef USE_OPENGL
    gl_leave();
    if (sdl_glcontext != NULL)
    {
        /* let the driver finish with the context before it goes (Apple's
         *  GL-on-Metal can complete its command buffers after the delete). */
        glFinish();
        SDL_GL_MakeCurrent(sdl_window, NULL);
        SDL_GL_DeleteContext(sdl_glcontext);
        sdl_glcontext = NULL;
    } /* if */
#endif

    if (sdl_renderer != NULL)
    {
        SDL_DestroyRenderer(sdl_renderer);
        sdl_renderer = NULL;
    } /* if */

    if (sdl_window != NULL)
    {
        SDL_DestroyWindow(sdl_window);
        sdl_window = NULL;
    } /* if */

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
} /* shutdown_video */


static void set_mouse_grab(int grab)
{
    SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE);
    if (sdl_window != NULL)
        SDL_SetWindowGrab(sdl_window, grab ? SDL_TRUE : SDL_FALSE);
} /* set_mouse_grab */


static Uint8 *get_framebuffer(void)
{
    if (renderer == RENDERER_SOFTWARE)
        return((Uint8 *) surface->pixels);

    return(NULL);
} /* get_framebuffer */


/*
 * !!! This is almost an entire copy of the original setgamemode().
 * !!!  Figure out what is needed for just 2D mode, and separate that
 * !!!  out. Then, place the original setgamemode() back into engine.c,
 * !!!  and remove our simple implementation (and this function.)
 * !!!  Just be sure to keep the non-DOS things, like the window's
 * !!!  titlebar caption.   --ryan.
 */
static char screenalloctype = 255;
static void init_res_lookups(int davidoption, int framebuffer);

static void init_new_res_vars(int davidoption)
{
    setupmouse();

    SDL_SetWindowTitle(sdl_window, titlelong);

    xdim = xres = surface->w;
    ydim = yres = surface->h;

    bytesperline = surface->w;
	qsetmode = surface->h;

    frameplace = (intptr_t) ( ((Uint8 *) surface->pixels) );

    init_res_lookups(davidoption, 1);
} /* init_new_res_vars */


/*
 * The part of a mode set shared with OpenGL: ylookup, horizlookup and the
 *  view. framebuffer is 0 in OpenGL, where screen holds only the lookups,
 *  frameplace stays 0 and nothing is cleared through it.
 */
static void init_res_lookups(int davidoption, int framebuffer)
{
    int i = 0;
    int j = 0;

    vesachecked = 1;
    vgacompatible = 1;
    linearmode = 1;
	activepage = visualpage = 0;
    horizlookup = horizlookup2 = NULL;

  	if (screen != NULL)
   	{
       	if (screenalloctype == 0) kkfree((void *)screen);
   	    if (screenalloctype == 1) suckcache((int32_t *)screen);
   		screen = NULL;
   	} /* if */

    if (davidoption != -1)
    {
    	if (!framebuffer)
    		i = 0;
    	else switch(vidoption)
    	{
    		case 1:i = xdim*ydim; break;
    		case 2: xdim = 320; ydim = 200; i = xdim*ydim; break;
    		case 6: xdim = 320; ydim = 200; i = 131072; break;
    		default: assert(0);
    	}
    	j = ydim*4*sizeof(int32_t);  /* Leave room for horizlookup&horizlookup2 */

    	screenalloctype = 0;
	    if ((screen = (char *)kkmalloc(i+(j<<1))) == NULL)
    	{
	    	 allocache((intptr_t *)&screen,i+(j<<1),&permanentlock);
    		 screenalloctype = 1;
    	}

        if (framebuffer)
            frameplace = FP_OFF(screen);
        horizlookup = (int32_t *)(FP_OFF(screen)+i);
        horizlookup2 = (int32_t *)(FP_OFF(screen)+i+j);
    } /* if */

    calcpixelaspect();

    j = 0;
  	for(i = 0; i <= ydim; i++)
    {
        ylookup[i] = j;
        j += bytesperline;
    } /* for */

   	horizycent = ((ydim*4)>>1);

		/* Force drawrooms to call dosetaspect & recalculate stuff */
	oxyaspect = oxdimen = oviewingrange = -1;

	setvlinebpl(bytesperline);

    if (davidoption != -1)
    {
    	setview(0L,0L,xdim-1,ydim-1);
    	if (framebuffer)
    		clearallviews(0L);
    } /* if */

	setbrightness((char) curbrightness, (unsigned char *) &palette[0]);

	if (searchx < 0) { searchx = halfxdimen; searchy = (ydimen>>1); }
} /* init_res_lookups */


/*
 * Moves a window from display mode now to mode, exclusive at the display
 *  mode nearest exclusive_w by exclusive_h (borderless when there's none).
 *  Between the full-screen styles it goes through windowed. Returns the mode
 *  it's in. (Its flags can't always tell: under sdl2-compat the dummy
 *  driver reports exclusive as desktop full screen.)
 */
static int set_window_display(SDL_Window *window, int now, int mode)
{
    SDL_DisplayMode want, got;
    Uint32 flags = (mode == DISPLAY_EXCLUSIVE) ? SDL_WINDOW_FULLSCREEN :
                   (mode == DISPLAY_BORDERLESS) ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

    if (mode == DISPLAY_EXCLUSIVE)
    {
        memset(&want, '\0', sizeof (want));
        want.w = exclusive_w;
        want.h = exclusive_h;
        if ((SDL_GetClosestDisplayMode(SDL_GetWindowDisplayIndex(window), &want, &got) == NULL) ||
            (SDL_SetWindowDisplayMode(window, &got) != 0))
        {
            printf("No exclusive fullscreen display mode fits %dx%d (%s); using borderless.\n",
                   exclusive_w, exclusive_h, SDL_GetError());
            return(set_window_display(window, now, DISPLAY_BORDERLESS));
        } /* if */
        sdldebug("exclusive fullscreen at %dx%d, %d Hz.", got.w, got.h, got.refresh_rate);
    } /* if */

    if (now == mode)
        return(mode);
    if ((now != DISPLAY_WINDOWED) && (mode != DISPLAY_WINDOWED) &&
        (SDL_SetWindowFullscreen(window, 0) == 0))
        now = DISPLAY_WINDOWED;
    if (SDL_SetWindowFullscreen(window, flags) != 0)
    {
        sdldebug("SDL_SetWindowFullscreen() failed: [%s].", SDL_GetError());
        return(now);
    } /* if */
    return(mode);
} /* set_window_display */


/*
 * A window in sdl_displaymode. An exclusive one is made hidden and shown
 *  once its display mode is set, so the display changes mode only once.
 */
static SDL_Window *create_window(int w, int h, Uint32 flags)
{
    SDL_Window *window;

    if (sdl_displaymode == DISPLAY_BORDERLESS)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    else if (sdl_displaymode == DISPLAY_EXCLUSIVE)
        flags |= SDL_WINDOW_HIDDEN;
    window = SDL_CreateWindow(titlelong,
                    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, flags);
    if ((window != NULL) && (sdl_displaymode == DISPLAY_EXCLUSIVE))
    {
        sdl_displaymode = set_window_display(window, DISPLAY_WINDOWED, DISPLAY_EXCLUSIVE);
        SDL_ShowWindow(window);
    } /* if */
    return(window);
} /* create_window */


static void go_to_new_vid_mode(int vidoption, int w, int h)
{
    int scale;

    getvalidvesamodes();
    SDL_ClearError();

    if ((SDL_WasInit(SDL_INIT_VIDEO) == 0) && (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0))
        goto vidmode_failed;

    free_video_surfaces();
#ifdef USE_OPENGL
    gl_leave();  /* the GL window, if there is one, shows the 8-bit mode */
#endif

    /* low-res modes get a bigger window so they aren't a postage stamp. */
    scale = (w < 640) ? (640 / w) : 1;
    if (scale < 1)
        scale = 1;
#ifdef USE_OPENGL
    gl_window_w = w * scale;  /* a fullscreen window reports the desktop size. */
    gl_window_h = h * scale;
#endif

    if (exclusive_w == 0)  /* no game mode yet (the editor's 2D) */
    {
        exclusive_w = w * scale;
        exclusive_h = h * scale;
    } /* if */

    if (sdl_window == NULL)
    {
        sdl_window = create_window(w * scale, h * scale, 0);
        if (sdl_window == NULL)
            goto vidmode_failed;

        sdl_renderer = SDL_CreateRenderer(sdl_window, -1,
                                          sdl_vsync ? SDL_RENDERER_PRESENTVSYNC : 0);
        if (sdl_renderer == NULL)  /* the hinted driver may not be there. */
        {
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "");
            sdl_renderer = SDL_CreateRenderer(sdl_window, -1,
                                              sdl_vsync ? SDL_RENDERER_PRESENTVSYNC : 0);
        } /* if */
        if (sdl_renderer == NULL)
            sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);
        if (sdl_renderer == NULL)
            goto vidmode_failed;
    } /* if */
    else
    {
        /* set the size even when fullscreen, so going windowed restores it. */
        SDL_SetWindowSize(sdl_window, w * scale, h * scale);
        if (sdl_displaymode == DISPLAY_WINDOWED)
            SDL_SetWindowPosition(sdl_window, SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED);
        else  /* exclusive keeps the game mode's display mode. */
            sdl_displaymode = set_window_display(sdl_window, sdl_displaymode, sdl_displaymode);
    } /* else */

    surface = SDL_CreateRGBSurface(0, w, h, 8, 0, 0, 0, 0);
#ifdef USE_OPENGL
    if (sdl_glcontext != NULL)  /* shown through the GL context: gl_present8 */
    {
        sdl_rgbsurface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32,
                                                        SDL_PIXELFORMAT_RGBA32);
        if ((surface == NULL) || (sdl_rgbsurface == NULL))
            goto vidmode_failed;
        gl_alloc8(w, h);
    } /* if */
    else
#endif
    {
        SDL_RenderSetLogicalSize(sdl_renderer, w, h);
        sdl_rgbsurface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32,
                                                        SDL_PIXELFORMAT_ARGB8888);
        sdl_texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_STREAMING, w, h);
        if ((surface == NULL) || (sdl_rgbsurface == NULL) || (sdl_texture == NULL))
            goto vidmode_failed;
    } /* else */

    /* the engine assumes bytesperline == width. */
    assert(surface->pitch == surface->w);

    output_surface_info(surface);
    init_new_res_vars(vidoption);
    return;

vidmode_failed:
    fprintf(stderr, "BUILDSDL: Failed to set %dx%d video mode!\n"
                    "BUILDSDL: SDL_Error() says [%s].\n",
                    w, h, SDL_GetError());
    SDL_Quit();
    exit(13);
} /* go_to_new_vid_mode */


#ifdef USE_OPENGL
/*
 * The texture that keeps the last frame, for fades over a frame that isn't
 *  being redrawn. GL 1.1 has no non-power-of-two textures, so without
 *  GL_ARB_texture_non_power_of_two it's rounded up to one.
 */
static void gl_alloc_lastframe(void)
{
    GLint maxsize = 0;

    if (lastframe != 0)
        glDeleteTextures(1, &lastframe);
    lastframe = 0;
    lastframe_set = 0;
    swapped = 0;

    lastframe_w = xres;
    lastframe_h = yres;
    if (!glinfo.texnpot)
    {
        for (lastframe_w = 1; lastframe_w < xres; lastframe_w <<= 1) ;
        for (lastframe_h = 1; lastframe_h < yres; lastframe_h <<= 1) ;
    } /* if */
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
    if ((lastframe_w > maxsize) || (lastframe_h > maxsize))
    {
        sdldebug("No last-frame texture: %dx%d is over GL_MAX_TEXTURE_SIZE %d.",
                 lastframe_w, lastframe_h, (int) maxsize);
        return;
    } /* if */

    glGenTextures(1, &lastframe);
    glBindTexture(GL_TEXTURE_2D, lastframe);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, lastframe_w, lastframe_h, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, NULL);
} /* gl_alloc_lastframe */


/* A full-screen quad of palfadergb at palfadedelta/256 (JFBuild's showframe). */
static void gl_draw_fade(void)
{
    setpolymost2dview();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4ub(palfadergb.r, palfadergb.g, palfadergb.b, palfadedelta);
    glBegin(GL_QUADS);
    glVertex2i(0, 0);
    glVertex2i(xres, 0);
    glVertex2i(xres, yres);
    glVertex2i(0, yres);
    glEnd();
    glDisable(GL_BLEND);
} /* gl_draw_fade */


/* Fills the back buffer with (0,0)-(u,v) of a frame texture: bottom-up rows. */
static void gl_draw_frame(GLuint tex, float u, float v)
{
    setpolymost2dview();
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glColor4f(1, 1, 1, 1);
    glBegin(GL_QUADS);
    glTexCoord2f(0, v); glVertex2i(0, 0);
    glTexCoord2f(u, v); glVertex2i(xres, 0);
    glTexCoord2f(u, 0); glVertex2i(xres, yres);
    glTexCoord2f(0, 0); glVertex2i(0, yres);
    glEnd();
    glDisable(GL_TEXTURE_2D);
} /* gl_draw_frame */


/* Fills the back buffer with the last frame. */
static void gl_draw_lastframe(void)
{
    gl_draw_frame(lastframe, (float) xres / (float) lastframe_w,
                  (float) yres / (float) lastframe_h);
} /* gl_draw_lastframe */


/*
 * Shows the back buffer: keeps it in lastframe, fades it, swaps. The GL 2.1
 *  tier keeps the frame itself, and shows it through the faded palette.
 */
static void gl_swap(void)
{
    if (polymost_present_shader(gl_showrect, gl_draw_w, gl_draw_h))
    {
        SDL_GL_SwapWindow(sdl_window);
        swapped = 1;
        framedrawn = 0;
        return;
    } /* if */

    glsub_flush();
    if ((framedrawn) && (lastframe != 0))
    {
        glBindTexture(GL_TEXTURE_2D, lastframe);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, xres, yres);
        lastframe_set = 1;
    } /* if */

    if (palfadedelta)
        gl_draw_fade();

    SDL_GL_SwapWindow(sdl_window);
    swapped = 1;
    framedrawn = 0;
} /* gl_swap */


void _glframedrawn(void)
{
    framedrawn = 1;
} /* _glframedrawn */


/*
 * nextpage's permanent-sprite replay after the swap: it repeats sprites of
 *  the frame just shown, so it doesn't count as drawn, and a still screen
 *  drawn once can fade.
 */
void _glframereplayed(void)
{
    framedrawn = 0;
    polymost_unbind_shader();  /* the GL 2.1 tier's drawing went into its frame */
} /* _glframereplayed */


/*
 * setpalettefade in renderer 3. A frame being drawn gets the fade when it's
 *  shown; otherwise the last frame is shown again with the new fade, as a
 *  title screen fading in or out isn't redrawn.
 */
void _glpalettefade(void)
{
    if ((!gl_drawing) || (framedrawn))
        return;
    if (polymost_present_shader(gl_showrect, gl_draw_w, gl_draw_h))  /* the GL 2.1 tier keeps the frame */
    {
        SDL_GL_SwapWindow(sdl_window);
        swapped = 1;
        framedrawn = 0;
        return;
    } /* if */
    if (!lastframe_set)
        return;

    gl_draw_lastframe();
    if (palfadedelta)
        gl_draw_fade();
    SDL_GL_SwapWindow(sdl_window);
    swapped = 1;
    framedrawn = 0;
} /* _glpalettefade */


/*
 * xdim/ydim are the drawable size, clamped to what the engine's arrays hold,
 *  or on the shader path its internal size (glinternalwidth/height), which
 *  the present scales up to the drawable. say: tell why a setting isn't used.
 */
static void init_gl_res_vars(int say)
{
    int w, h, why;
    int32_t iw, ih;

    SDL_GL_GetDrawableSize(sdl_window, &gl_draw_w, &gl_draw_h);
    w = (gl_draw_w > MAXXDIM) ? MAXXDIM : gl_draw_w;
    h = (gl_draw_h > MAXYDIM) ? MAXYDIM : gl_draw_h;

    why = gl_internalsize(glinternalwidth, glinternalheight, &iw, &ih);
    if ((say) && (why == 1))
        printf("GL internal size %dx%d needs a width and a height; drawing at the window's size.\n",
               (int) glinternalwidth, (int) glinternalheight);
    else if ((say) && (why == 2))
        printf("GL internal size %dx%d is under 320x200; drawing at %dx%d.\n",
               (int) glinternalwidth, (int) glinternalheight, (int) iw, (int) ih);
    if ((iw != 0) && (gldrawpath != GLPATH_SHADER))
    {
        if (say)
            printf("GL internal size %dx%d needs draw path shader; drawing at the window's size.\n",
                   (int) iw, (int) ih);
        iw = 0;
    } /* if */
    gl_scaled = (iw != 0);
    if (gl_scaled)
    {
        w = iw;
        h = ih;
    } /* if */
    gl_fitrect(w, h, gl_draw_w, gl_draw_h, gl_showrect);

    xdim = xres = w;
    ydim = yres = h;
    bytesperline = w;
    qsetmode = 200;
    frameplace = 0;
    sdldebug("OpenGL drawable is %dx%d, drawn at %dx%d.", gl_draw_w, gl_draw_h, w, h);

    SDL_SetWindowTitle(sdl_window, titlelong);
    setupmouse();
    gl_alloc_lastframe();
    init_res_lookups(gl_davidoption, 0);
} /* init_gl_res_vars */


/*
 * The drawable changed size (fullscreen toggle, a move between displays).
 *  With no frame being drawn (a still screen, which isn't drawn again), the
 *  last frame is shown again, stretched to the new size, and kept as the
 *  new last frame, so the screen and its fades don't go black.
 */
static void gl_resize(void)
{
    int w, h;
    palette_t pal[256], fadergb;
    unsigned char fadedelta, base[768];
    GLuint old, shframe;
    float oldu = 0, oldv = 0;
    int drawn;

    SDL_GL_GetDrawableSize(sdl_window, &w, &h);
    if ((w == gl_draw_w) && (h == gl_draw_h))
        return;
    if (gl_scaled)  /* the internal size stays; only where it shows changes. */
    {
        gl_draw_w = w;
        gl_draw_h = h;
        gl_fitrect(xres, yres, w, h, gl_showrect);
        if ((!framedrawn) && (polymost_present_shader(gl_showrect, w, h)))
        {
            SDL_GL_SwapWindow(sdl_window);  /* a still screen: shown again. */
            swapped = 1;
            framedrawn = 0;
        } /* if */
        return;
    } /* if */

    /* init_res_lookups sets the base palette and drops the fade: keep both. */
    memcpy(pal, curpalette, sizeof (pal));
    memcpy(base, getbasepalette(), sizeof (base));  /* the GL 2.1 tier fades it */
    fadergb = palfadergb;
    fadedelta = palfadedelta;
    old = 0;
    if ((lastframe_set) && (!framedrawn))
    {
        old = lastframe;
        oldu = (float) xres / (float) lastframe_w;
        oldv = (float) yres / (float) lastframe_h;
        lastframe = 0;  /* keep it from gl_alloc_lastframe. */
    } /* if */
    shframe = polymost_takeframe_shader();  /* the GL 2.1 tier's, drawn or being drawn */
    polymost_glreset();
    init_gl_res_vars(0);
    memcpy(curpalette, pal, sizeof (pal));
    memcpy(getbasepalette(), base, sizeof (base));
    palfadergb = fadergb;
    palfadedelta = fadedelta;
    gltexinvalidateall();

    if (shframe != 0)
    {
        drawn = framedrawn;
        polymost_putframe_shader(shframe);
        if (!drawn)
            gl_swap();  /* a still screen: shown again at the new size. */
    } /* if */
    else if (old != 0)
    {
        gl_draw_frame(old, oldu, oldv);
        glDeleteTextures(1, &old);
        gl_swap();  /* framedrawn: it becomes lastframe, faded and shown. */
    } /* if */
} /* gl_resize */
#endif


int setgamemode_gl(char davidoption)
{
#ifdef USE_OPENGL
    SDL_Window *window;
    SDL_GLContext context;
    const char *exts;

    SDL_ClearError();
    if ((SDL_WasInit(SDL_INIT_VIDEO) == 0) && (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0))
        return(-1);

    if (sdl_glcontext != NULL)  /* the run's GL window: set its size again */
    {
        free_video_surfaces();
        gl_leave();
        SDL_SetWindowSize(sdl_window, gl_window_w, gl_window_h);
        if (sdl_displaymode == DISPLAY_WINDOWED)
            SDL_SetWindowPosition(sdl_window, SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED);
        else  /* exclusive keeps the game mode's display mode. */
            sdl_displaymode = set_window_display(sdl_window, sdl_displaymode, sdl_displaymode);
    } /* if */
    else
    {
        /*
         * Make the GL window before dropping the 8-bit one, so a failure
         *  leaves that mode as it was. SDL2 can't add GL to a window that
         *  has an SDL_Renderer, so it's a new window.
         */
        int displaymode = sdl_displaymode;  /* create_window may change it */

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        window = create_window(gl_window_w, gl_window_h,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
        context = ((window != NULL) && (gl_forcefail != 4)) ? SDL_GL_CreateContext(window) : NULL;
        if (context == NULL)
        {
            if (window != NULL)
                SDL_DestroyWindow(window);
            sdl_displaymode = displaymode;  /* the 8-bit window's, still */
            if (gl_forcefail == 4)
                SDL_SetError("no context (gl_forcefail 4)");
            return(-1);
        } /* if */

        free_video_surfaces();
        if (sdl_renderer != NULL)
        {
            SDL_DestroyRenderer(sdl_renderer);
            sdl_renderer = NULL;
        } /* if */
        if (sdl_window != NULL)
            SDL_DestroyWindow(sdl_window);
        sdl_window = window;
        sdl_glcontext = context;
    } /* else */

    /* destroying an SDL_Renderer can leave no context current. */
    SDL_GL_MakeCurrent(sdl_window, sdl_glcontext);
    if (SDL_GL_SetSwapInterval(sdl_vsync) != 0)
        sdldebug("SDL_GL_SetSwapInterval() failed: [%s].", SDL_GetError());

    /* the state JFBuild sets after every GL mode change. */
    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_SMOOTH);
    glClearColor(0, 0, 0, 0.5);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_FOG_HINT, GL_NICEST);  /* fog per pixel, as classic darkens */
    glDisable(GL_DITHER);

    exts = (const char *) glGetString(GL_EXTENSIONS);
    if (exts == NULL)
        exts = "";
    glinfo.maxanisotropy = 1.0;
    if (gl_has_extension(exts, "GL_EXT_texture_filter_anisotropic"))
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glinfo.maxanisotropy);
    glinfo.bgra = gl_has_extension(exts, "GL_EXT_bgra");
    glinfo.clamptoedge = (gl_has_extension(exts, "GL_EXT_texture_edge_clamp") ||
                          gl_has_extension(exts, "GL_SGIS_texture_edge_clamp"));
    glinfo.texcompr = gl_has_extension(exts, "GL_ARB_texture_compression");
    glinfo.texnpot = gl_has_extension(exts, "GL_ARB_texture_non_power_of_two");
    gl_initdrawpath(exts);
    glsub_init();
    sdldebug("OpenGL %s on %s: bgra %d, clamptoedge %d, texcompr %d, texnpot %d, maxanisotropy %g, vbo %d, glsl %d, fbo %d.",
             (const char *) glGetString(GL_VERSION), (const char *) glGetString(GL_RENDERER),
             glinfo.bgra, glinfo.clamptoedge, glinfo.texcompr, glinfo.texnpot, glinfo.maxanisotropy,
             glinfo.vbo, glinfo.glsl, glinfo.fbo);

    bpp = 32;
    numpages = 2;  /* as JFBuild: permanent rotatesprites go on both pages. */
    gl_drawing = 1;
    gl_davidoption = davidoption;
    init_gl_res_vars(1);
    setrendermode(3);
    polymost_glreset();
    if (gl_scaled)  /* the shader path failing now draws at the window's size. */
    {
        polymost_prepare_shader();
        if (gldrawpath != GLPATH_SHADER)
            init_gl_res_vars(1);
    } /* if */
    last_render_ticks = SDL_GetTicks();
    return(0);
#else
    (void) davidoption;
    return(SDL_SetError("this build has no OpenGL"));
#endif
} /* setgamemode_gl */


/*
 * Renderer 3: starts the frame being drawn from the last frame shown, without
 *  its fade, for a screen that draws over what's already there.
 */
void glredrawlastframe(void)
{
#ifdef USE_OPENGL
    if ((gl_drawing) && (lastframe_set))
        gl_draw_lastframe();
#endif
} /* glredrawlastframe */


static __inline int sdl_mouse_button_filter(SDL_MouseButtonEvent const *event)
{
        /*
         * What bits BUILD expects:
         *  0 left button pressed if 1
         *  1 right button pressed if 1
         *  2 middle button pressed if 1
         *
         *   (That is, this is what Int 33h (AX=0x05) returns...)
         *
         *  additionally 3&4 are set for the mouse wheel
         */
    Uint8 button = event->button;

    /* SDL2 reports the wheel separately; buttons 4/5 are X1/X2 now. */
    if (button > SDL_BUTTON_RIGHT)
        return(0);

    if (button == SDL_BUTTON_RIGHT)
        button = SDL_BUTTON_MIDDLE;
    else if (button == SDL_BUTTON_MIDDLE)
        button = SDL_BUTTON_RIGHT;

    if (((const SDL_MouseButtonEvent*)event)->state)
        mouse_buttons |= 1<<(button-1);
    else if (button != 4 && button != 5)
        mouse_buttons ^= 1<<(button-1);

#if 0
    Uint8 bmask = SDL_GetMouseState(NULL, NULL);
    mouse_buttons = 0;
    if (bmask & SDL_BUTTON_LMASK) mouse_buttons |= 1;
    if (bmask & SDL_BUTTON_RMASK) mouse_buttons |= 2;
    if (bmask & SDL_BUTTON_MMASK) mouse_buttons |= 4;
#endif

    return(0);
} /* sdl_mouse_up_filter */


static int sdl_mouse_wheel_filter(SDL_MouseWheelEvent const *event)
{
    /* bits 3 and 4 are wheel up/down; readmousebstatus() clears them. */
    Sint32 y = event->y;

    if (event->direction == SDL_MOUSEWHEEL_FLIPPED)
        y = -y;

    if (y > 0)
        mouse_buttons |= 8;
    else if (y < 0)
        mouse_buttons |= 16;
    return(0);
} /* sdl_mouse_wheel_filter */


/*
 * The size mouse_x/mouse_y run over: SDL's mouse events are in the 8-bit
 *  renderer's logical size, but in the window's points in OpenGL, where
 *  the drawable (xres, yres) can be bigger (a high-DPI display).
 */
static void mouse_area(int *w, int *h)
{
#ifdef USE_OPENGL
    if (gl_drawing)
    {
        SDL_GetWindowSize(sdl_window, w, h);
        return;
    } /* if */
#endif
    *w = (surface != NULL) ? surface->w : xres;
    *h = (surface != NULL) ? surface->h : yres;
} /* mouse_area */


void getmousearea(int *w, int *h)
{
    mouse_area(w, h);
#ifdef USE_OPENGL
    /* drawn at an internal size: the frame's part of the window, in points. */
    if ((gl_drawing) && (gl_scaled) && (gl_draw_w > 0) && (gl_draw_h > 0))
    {
        *w = (int) (((int64_t) gl_showrect[2] * *w + gl_draw_w / 2) / gl_draw_w);
        *h = (int) (((int64_t) gl_showrect[3] * *h + gl_draw_h / 2) / gl_draw_h);
    } /* if */
#endif
} /* getmousearea */


static int sdl_mouse_motion_filter(SDL_Event const *event)
{
    int w, h;
#ifdef USE_OPENGL
    SDL_Event scaled;
#endif

    if (sdl_window == NULL)
        return(0);

#ifdef USE_OPENGL
    if ((event->type == SDL_MOUSEMOTION) && (sdl_glcontext != NULL) &&
        (!gl_drawing) && (surface != NULL))
    {
        scaled = *event;
        gl_mouse8(&scaled.motion);
        event = &scaled;
    } /* if */
#endif

    if (event->type == SDL_JOYBALLMOTION)
    {
        mouse_relative_x = event->jball.xrel/100;
        mouse_relative_y = event->jball.yrel/100;
       	mouse_x += mouse_relative_x;
       	mouse_y += mouse_relative_y;
    } /* if */
    else
    {
        if (mouse_grabbed)
        {
          	mouse_relative_x += event->motion.xrel;
       	        mouse_relative_y += event->motion.yrel;
           	mouse_x += mouse_relative_x;
           	mouse_y += mouse_relative_y;
        } /* if */
        else
        {
          	mouse_relative_x += event->motion.x - mouse_x;
           	mouse_relative_y += event->motion.y - mouse_y;
           	mouse_x = event->motion.x;
           	mouse_y = event->motion.y;
        } /* else */
    } /* else */

    mouse_area(&w, &h);
   	if (mouse_x < 0) mouse_x = 0;
   	if (mouse_x > w) mouse_x = w;
   	if (mouse_y < 0) mouse_y = 0;
   	if (mouse_y > h) mouse_y = h;

    return(0);
} /* sdl_mouse_motion_filter */


/* With SDL2 the surface survives the change, so no pointer fixups needed. */
static void show_display_mode(int mode)
{
    if (sdl_window == NULL)
    {
        sdl_displaymode = mode;  /* the next mode set makes the window. */
        return;
    } /* if */

    sdl_displaymode = set_window_display(sdl_window, sdl_displaymode, mode);
#ifdef USE_OPENGL
    if (gl_drawing)
    {
        gl_resize();  /* macOS may report the new size later, as an event. */
        return;
    } /* if */
#endif
    output_surface_info(surface);
#ifdef USE_OPENGL
    if (sdl_glcontext != NULL)
        present_surface();  /* the GL window shows it again at the new size */
#endif
} /* show_display_mode */


void setvideodisplaymode(int mode)
{
    if ((mode < 0) || (mode >= DISPLAY_COUNT))
        mode = DISPLAY_BORDERLESS;
    sdl_displaymode_wanted = mode;
    show_display_mode((getenv(BUILD_WINDOWED) != NULL) ? DISPLAY_WINDOWED : mode);
} /* setvideodisplaymode */


int getvideodisplaymode(void)
{
    return(sdl_displaymode);
} /* getvideodisplaymode */


void setvideovsync(int on)
{
    sdl_vsync = (on != 0);
#ifdef USE_OPENGL
    if (sdl_glcontext != NULL)
        if (SDL_GL_SetSwapInterval(sdl_vsync) != 0)
            sdldebug("SDL_GL_SetSwapInterval() failed: [%s].", SDL_GetError());
#endif
#if SDL_VERSION_ATLEAST(2, 0, 18)
    if (sdl_renderer != NULL)
        if (SDL_RenderSetVSync(sdl_renderer, sdl_vsync) != 0)
            sdldebug("SDL_RenderSetVSync() failed: [%s].", SDL_GetError());
#endif  /* older SDL: at the next window's renderer. */
} /* setvideovsync */


int getvideovsync(void)
{
    return(sdl_vsync);
} /* getvideovsync */


/* Alt+Enter: windowed, or the full-screen style set (borderless if none). */
static int attempt_fullscreen_toggle(void)
{
    int mode = sdl_displaymode_wanted;

    sdldebug("attempting to toggle fullscreen flag...");

    if (sdl_window == NULL)
        return(0);

    if (sdl_displaymode != DISPLAY_WINDOWED)
        mode = DISPLAY_WINDOWED;
    else if (mode == DISPLAY_WINDOWED)
        mode = DISPLAY_BORDERLESS;
    show_display_mode(mode);
    return(sdl_displaymode == mode);
} /* attempt_fullscreen_toggle */


    /*
     * The windib driver can't alert us to the keypad enter key, which
     *  Ken's code depends on heavily. It sends it as the same key as the
     *  regular return key. These users will have to hit SHIFT-ENTER,
     *  which we check for explicitly, and give the engine a keypad enter
     *  enter event.
     */
static __inline int handle_keypad_enter_hack(const SDL_Event *event)
{
    static int kp_enter_hack = 0;
    int retval = 0;

    if (event->key.keysym.sym == SDLK_RETURN)
    {
        if (event->key.state == SDL_PRESSED)
        {
            if (event->key.keysym.mod & KMOD_SHIFT)
            {
                kp_enter_hack = 1;
                lastkey = scancodes[SDL_SCANCODE_KP_ENTER];
                retval = 1;
            } /* if */
        } /* if */

        else  /* key released */
        {
            if (kp_enter_hack)
            {
                kp_enter_hack = 0;
                lastkey = scancodes[SDL_SCANCODE_KP_ENTER];
                retval = 1;
            } /* if */
        } /* if */
    } /* if */

    return(retval);
} /* handle_keypad_enter_hack */


static int sdl_key_filter(const SDL_Event *event)
{
    SDL_Scancode sc = event->key.keysym.scancode;
    int extended;

    /* SDL 1.2 never sent key repeat here; the engine expects none. */
    if (event->key.repeat)
        return(0);

    #if PLATFORM_MACOSX  /* Apple-Q */
    {
        static Uint32 cmdqticks = 0;
        if ( (event->key.keysym.sym == SDLK_q) &&
             (event->key.state == SDL_PRESSED) &&
             (event->key.keysym.mod & KMOD_GUI) )
        {
            Uint32 t = SDL_GetTicks();
            if (t - cmdqticks < 500)  /* 2 hits within .5 second? */
            {
                SDL_Quit();
                exit(0);   
            }
            cmdqticks = t;
        }
    }
    #endif


    if ( (event->key.keysym.sym == SDLK_g) &&
         (event->key.state == SDL_PRESSED) &&
         (event->key.keysym.mod & KMOD_CTRL) )
    {
        mouse_grabbed = ((mouse_grabbed) ? 0 : 1);
        set_mouse_grab(mouse_grabbed);
        return(0);
    } /* if */

    else if ( ( (event->key.keysym.sym == SDLK_RETURN) ||
                (event->key.keysym.sym == SDLK_KP_ENTER) ) &&
              (event->key.state == SDL_PRESSED) &&
              (event->key.keysym.mod & KMOD_ALT) )
    {
        attempt_fullscreen_toggle();
        return(0);
    } /* if */

    if (!handle_keypad_enter_hack(event))
        lastkey = scancodes[sc];

    if (lastkey == 0x0000)   /* No DOS equivalent defined. */
        return(0);

    extended = ((lastkey & 0xFF00) >> 8);
    if (extended != 0)
    {
        lastkey = extended;
        keyhandler();
        lastkey = (scancodes[sc] & 0xFF);
    } /* if */

    if (event->key.state == SDL_RELEASED)
        lastkey += 128;  /* +128 signifies that the key is released in DOS. */

    keyhandler();
    return(0);
} /* sdl_key_filter */


static int root_sdl_event_filter(const SDL_Event *event)
{
    switch (event->type)
    {
        case SDL_KEYUP:
        case SDL_KEYDOWN:
            return(sdl_key_filter(event));
        case SDL_JOYBALLMOTION:
        case SDL_MOUSEMOTION:
            return(sdl_mouse_motion_filter(event));
        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN:
	    return(sdl_mouse_button_filter(&event->button));
        case SDL_MOUSEWHEEL:
            return(sdl_mouse_wheel_filter(&event->wheel));
        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_EXPOSED)
                present_surface();
#ifdef USE_OPENGL
            else if ((event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) &&
                     (gl_drawing))
                gl_resize();
            else if ((event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) &&
                     (sdl_glcontext != NULL))
                present_surface();  /* an 8-bit mode, at the new size */
#endif
            return(0);
        case SDL_QUIT:
            /* !!! rcg TEMP */
            SDL_Quit();
            exit(0);   
    } /* switch */

    return(1);
} /* root_sdl_event_filter */


static void handle_events(void)
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
        root_sdl_event_filter(&event);
} /* handle_events */


/* bleh...public version... */
void _handle_events(void)
{
    handle_events();
} /* _handle_events */


static SDL_Joystick *joystick = NULL;
void _joystick_init(void)
{
    const char *envr = getenv(BUILD_SDLJOYSTICK);
    int favored = 0;
    int numsticks;
    int i;

    if (joystick != NULL)
    {
        sdldebug("Joystick appears to be already initialized.");
        sdldebug("...deinitializing for stick redetection...");
        _joystick_deinit();
    } /* if */

    if ((envr != NULL) && (strcmp(envr, "none") == 0))
    {
        sdldebug("Skipping joystick detection/initialization at user request");
        return;
    } /* if */

    sdldebug("Initializing SDL joystick subsystem...");
    sdldebug(" (export environment variable BUILD_SDLJOYSTICK=none to skip)");

    if (SDL_Init(SDL_INIT_JOYSTICK) != 0)
    {
        sdldebug("SDL_Init(SDL_INIT_JOYSTICK) failed: [%s].", SDL_GetError());
        return;
    } /* if */

    numsticks = SDL_NumJoysticks();
    sdldebug("SDL sees %d joystick%s.", numsticks, numsticks == 1 ? "" : "s");
    if (numsticks == 0)
        return;

    for (i = 0; i < numsticks; i++)
    {
        const char *stickname = SDL_JoystickNameForIndex(i);
        if (stickname == NULL)
            stickname = "(unnamed)";
        if ((envr != NULL) && (strcmp(envr, stickname) == 0))
            favored = i;

        sdldebug("Stick #%d: [%s]", i, stickname);
    } /* for */

    sdldebug("Using Stick #%d.", favored);
    if ((envr == NULL) && (numsticks > 1))
        sdldebug("Set BUILD_SDLJOYSTICK to one of the above names to change.");

    joystick = SDL_JoystickOpen(favored);
    if (joystick == NULL)
    {
        sdldebug("Joystick #%d failed to init: %s", favored, SDL_GetError());
        return;
    } /* if */

    sdldebug("Joystick initialized. %d axes, %d buttons, %d hats, %d balls.",
              SDL_JoystickNumAxes(joystick), SDL_JoystickNumButtons(joystick),
              SDL_JoystickNumHats(joystick), SDL_JoystickNumBalls(joystick));

    SDL_JoystickEventState(SDL_QUERY);
} /* _joystick_init */


void _joystick_deinit(void)
{
    if (joystick != NULL)
    {
        sdldebug("Closing joystick device...");
        SDL_JoystickClose(joystick);
        sdldebug("Joystick device closed. Deinitializing SDL subsystem...");
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
        sdldebug("SDL joystick subsystem deinitialized.");
        joystick = NULL;
    } /* if */
} /* _joystick_deinit */


int _joystick_update(void)
{
    if (joystick == NULL)
        return(0);

    SDL_JoystickUpdate();
    return(1);
} /* _joystick_update */


int _joystick_axis(int axis)
{
    if (joystick == NULL)
        return(0);

    return(SDL_JoystickGetAxis(joystick, axis));
} /* _joystick_axis */

int _joystick_hat(int hat)
{
    if (joystick == NULL)
    {   
        return(-1);
    }

    return(SDL_JoystickGetHat(joystick, hat));
} /* _joystick_axis */

int _joystick_button(int button)
{
    if (joystick == NULL)
        return(0);

    return(SDL_JoystickGetButton(joystick, button) != 0);
} /* _joystick_button */


unsigned char _readlastkeyhit(void)
{
    return(lastkey);
} /* _readlastkeyhit */


/* !!! I'd like this to be temporary. --ryan. */
#if (defined USE_I386_ASM)

#  if (defined PLATFORM_UNIX)
#    define PROT_R_W_X (PROT_READ | PROT_WRITE | PROT_EXEC)

#  elif (defined PLATFORM_WIN32)
#    ifndef PAGESIZE
#      define PAGESIZE 4096
#    endif
#    define PROT_R_W_X PAGE_EXECUTE_READWRITE

     static int mprotect(void *ptr, size_t len, int prot)
     {
         BOOL rc;
         DWORD old = 0;
         rc = VirtualProtect(ptr, len, prot, &old);
         return(rc == 0);
     }

#endif

#ifdef USE_I386_ASM
int mprotect_align(const void *addr, size_t len, int prot)
{
    int retval;
    uint32_t l = (uint32_t) addr;
    l -= (l % PAGESIZE);
    retval = mprotect((void *) l, len * 2, prot);
    assert(retval == 0);
    return(retval);
} /* mprotect_align */


void unprotect_ASM_pages(void)
{
    mprotect_align((const void *) asm_sethlinesizes, PAGESIZE, PROT_R_W_X);
    mprotect_align((const void *) asm_setpalookupaddress, PAGESIZE, PROT_R_W_X);
    mprotect_align((const void *) asm_setuphlineasm4, PAGESIZE, PROT_R_W_X);
    mprotect_align((const void *) asm_hlineasm4, PAGESIZE, PROT_R_W_X);
    mprotect_align((const void *) asm_setupvlineasm, PAGESIZE, PROT_R_W_X);
    mprotect_align((const void *) asm_vlineasm4, PAGESIZE, PROT_R_W_X);
    mprotect_align((const void *) asm_tvlineasm1, PAGESIZE, PROT_R_W_X);
    mprotect_align((const void *) asm_tvlineasm2, PAGESIZE, PROT_R_W_X);
    mprotect_align((const void *) asm_tspritevline, PAGESIZE, PROT_R_W_X);
    mprotect_align((const void *) asm_thline, PAGESIZE, PROT_R_W_X);
    mprotect_align((const void *) asm_prohlineasm4, PAGESIZE, PROT_R_W_X);
    mprotect_align((const void *) asm_stretchhline, PAGESIZE, PROT_R_W_X);
} /* unprotect_ASM_pages */
#endif


/*
  global _sethlinesizes


  global _prosethlinesizes
  global _setvlinebpl
  global _setpalookupaddress
  global _prosetpalookupaddress
  global _setuphlineasm4
  global _hlineasm4
  global _prohlineasm4
  global _setupvlineasm
  global _prosetupvlineasm
  global _setupmvlineasm
  global _setuptvlineasm
  global _prevlineasm1
  global _vlineasm1
  global _mvlineasm1
  global _fixtransluscence
  global _settransnormal
  global _settransreverse
  global _tvlineasm1
  global _vlineasm4
  global _provlineasm4
  global _mvlineasm4
  global _setupspritevline
  global _spritevline
  global _msetupspritevline
  global _mspritevline
  global _tsetupspritevline
  global _tspritevline
  global _msethlineshift
  global _mhline
  global _mhlineskipmodify
  global _tsethlineshift
  global _thline
  global _thlineskipmodify
  global _setuptvlineasm2
  global _tvlineasm2
  global _setupslopevlin2
  global _slopevlin2
  global _setupslopevlin
  global _slopevlin
  global _setuprhlineasm4
  global _rhlineasm4
  global _setuprmhlineasm4
  global _rmhlineasm4
  global _setupqrhlineasm4
  global _qrhlineasm4
  global _setupdrawslab
  global _drawslab
  global _stretchhline
  global _mmxoverlay
*/

#endif


static __inline void init_debugging(void)
{
    const char *envr = getenv(BUILD_SDLDEBUG); 

    debug_hall_of_mirrors = (getenv(BUILD_HALLOFMIRRORS) != NULL);

    if (_sdl_debug_file != NULL)
    {
        fclose(_sdl_debug_file);
        _sdl_debug_file = NULL;
    } /* if */

    if (envr != NULL)
    {
        if (strcmp(envr, "-") == 0)
	    _sdl_debug_file = stdout;
        else
            _sdl_debug_file = fopen(envr, "w");

        if (_sdl_debug_file == NULL)
            printf("BUILDSDL: -WARNING- Could not open debug file!\n");
        else
            setbuf(_sdl_debug_file, NULL);
    } /* if */
} /* init_debugging */


#if (!defined __DATE__)
#define __DATE__ "a long, long time ago"
#endif

static __inline void output_sdl_versions(void)
{
    SDL_version linked;
    const SDL_version *linked_ver = &linked;
    SDL_version compiled_ver;

    SDL_VERSION(&compiled_ver);
    SDL_GetVersion(&linked);

    sdldebug("SDL display driver for the BUILD engine initializing.");
    sdldebug("  sdl_driver.c by Ryan C. Gordon (icculus@clutteredmind.org).");
    sdldebug("Compiled %s against SDL version %d.%d.%d ...", __DATE__,
                compiled_ver.major, compiled_ver.minor, compiled_ver.patch);
    sdldebug("Linked SDL version is %d.%d.%d ...",
                linked_ver->major, linked_ver->minor, linked_ver->patch);
} /* output_sdl_versions */


static int in_vmware = 0;
static __inline void detect_vmware(void)
{
#if 1
    in_vmware = 0;  /* oh well. */
#else
    /* !!! need root access to touch i/o ports on Linux. */
    #if (!defined __linux__)
        in_vmware = (int) is_vmware_running();
    #endif
    sdldebug("vmWare %s running.", (in_vmware) ? "is" : "is not");
#endif
} /* detect_vmware */


/* lousy -ansi flag.  :) */
static char *string_dupe(const char *str)
{
    char *retval = malloc(strlen(str) + 1);
    if (retval != NULL)
        strcpy(retval, str);
    return(retval);
} /* string_dupe */


static void set_sdl_renderer(void)
{
    const char *envr = getenv(BUILD_RENDERER);

    if ((envr == NULL) || (strcmp(envr, ENVRSTR_RENDERER_SOFTWARE) == 0))
        renderer = RENDERER_SOFTWARE;

    else
    {
        fprintf(stderr,
                "BUILDSDL: \"%s\" in the %s environment var is not available.\n",
                envr, BUILD_RENDERER);
        _exit(1);
    } /* else */

#if defined(__APPLE__) && defined(USE_OPENGL)
    /* present the 8-bit modes (before any renderer 3) through SDL's GL
       renderer, not Metal: it presents faster with vsync off and its
       scaling keeps the palette's colours. A user's
       SDL_RENDER_DRIVER still wins. */
    SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "opengl", SDL_HINT_DEFAULT);
#endif

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)
    {
        fprintf(stderr, "BUILDSDL: SDL_Init() failed!\n");
        fprintf(stderr, "BUILDSDL: SDL_GetError() says \"%s\".\n", SDL_GetError());
        exit(1);
    } /* if */

} /* set_sdl_renderer */


static void init_renderer_names(void)
{
    memset((void *) renderer_name, '\0', sizeof (renderer_name));
    renderer_name[RENDERER_SOFTWARE] = "RENDERER_SOFTWARE";
} /* init_renderer_names */


void _platform_init(int argc, char **argv, const char *title, const char *icon)
{
    #if (defined PLATFORM_MACOSX)
    char buf[MAXPATHLEN];
    char realbuf[MAXPATHLEN];

	has_altivec = 0;  /* no PowerPC Macs anymore. */

    /* deal with Application Bundles on MacOS X... */
    if ((argv[0] != NULL) && (strchr(argv[0], '/') != NULL))
        strcpy(buf, argv[0]);
    else
    {
        if ((getcwd(buf, sizeof (buf)) == NULL) || (strcmp(buf, "/") == 0))
            buf[0] = '\0'; /* oh well. */
    } /* else */

    if (buf[0])
    {
        buf[sizeof (buf) - 1] = '\0';
        if (realpath(buf, realbuf) != NULL)
        {
            char *ptr = strstr(realbuf, "/Contents/MacOS/");
            if (ptr != NULL)
            {
                *ptr = '\0';  /* chop off bundle dirs... */
                chdir(realbuf);
            } /* if */
        } /* if */
    } /* if */
    #endif

    _argc = argc;
    _argv = argv;

    init_renderer_names();

    init_debugging();

    #if (defined USE_I386_ASM)
        unprotect_ASM_pages();
    #endif

    if (title == NULL)
        title = "BUILD";

    if (icon == NULL)
        icon = "BUILD";

    titlelong = string_dupe(title);
    titleshort = string_dupe(icon);

    if (getenv(BUILD_NOMOUSEGRAB) == NULL)
        mouse_grabbed = 1;
    else
        mouse_grabbed = 0;

    sdl_displaymode = (getenv(BUILD_WINDOWED) != NULL) ? DISPLAY_WINDOWED : sdl_displaymode_wanted;

    set_sdl_renderer();

    output_sdl_versions();
    output_driver_info();
    detect_vmware();

    memset(scancodes, '\0', sizeof (scancodes));
    /*
     * SDL2 scancodes are positional, like DOS scancodes, so non-QWERTY
     *  layouts (AZERTY, etc) map correctly without special cases.
     */
    scancodes[SDL_SCANCODE_ESCAPE]         = 0x01;
    scancodes[SDL_SCANCODE_1]              = 0x02;
    scancodes[SDL_SCANCODE_2]              = 0x03;
    scancodes[SDL_SCANCODE_3]              = 0x04;
    scancodes[SDL_SCANCODE_4]              = 0x05;
    scancodes[SDL_SCANCODE_5]              = 0x06;
    scancodes[SDL_SCANCODE_6]              = 0x07;
    scancodes[SDL_SCANCODE_7]              = 0x08;
    scancodes[SDL_SCANCODE_8]              = 0x09;
    scancodes[SDL_SCANCODE_9]              = 0x0A;
    scancodes[SDL_SCANCODE_0]              = 0x0B;
    scancodes[SDL_SCANCODE_MINUS]          = 0x0C;
    scancodes[SDL_SCANCODE_EQUALS]         = 0x0D;
    scancodes[SDL_SCANCODE_BACKSPACE]      = 0x0E;
    scancodes[SDL_SCANCODE_TAB]            = 0x0F;
    scancodes[SDL_SCANCODE_Q]              = 0x10;
    scancodes[SDL_SCANCODE_W]              = 0x11;
    scancodes[SDL_SCANCODE_E]              = 0x12;
    scancodes[SDL_SCANCODE_R]              = 0x13;
    scancodes[SDL_SCANCODE_T]              = 0x14;
    scancodes[SDL_SCANCODE_Y]              = 0x15;
    scancodes[SDL_SCANCODE_U]              = 0x16;
    scancodes[SDL_SCANCODE_I]              = 0x17;
    scancodes[SDL_SCANCODE_O]              = 0x18;
    scancodes[SDL_SCANCODE_P]              = 0x19;
    scancodes[SDL_SCANCODE_LEFTBRACKET]    = 0x1A;
    scancodes[SDL_SCANCODE_RIGHTBRACKET]   = 0x1B;
    scancodes[SDL_SCANCODE_RETURN]         = 0x1C;
    scancodes[SDL_SCANCODE_LCTRL]          = 0x1D;
    scancodes[SDL_SCANCODE_A]              = 0x1E;
    scancodes[SDL_SCANCODE_S]              = 0x1F;
    scancodes[SDL_SCANCODE_D]              = 0x20;
    scancodes[SDL_SCANCODE_F]              = 0x21;
    scancodes[SDL_SCANCODE_G]              = 0x22;
    scancodes[SDL_SCANCODE_H]              = 0x23;
    scancodes[SDL_SCANCODE_J]              = 0x24;
    scancodes[SDL_SCANCODE_K]              = 0x25;
    scancodes[SDL_SCANCODE_L]              = 0x26;
    scancodes[SDL_SCANCODE_SEMICOLON]      = 0x27;
    scancodes[SDL_SCANCODE_APOSTROPHE]     = 0x28;
    scancodes[SDL_SCANCODE_GRAVE]          = 0x29;
    scancodes[SDL_SCANCODE_LSHIFT]         = 0x2A;
    scancodes[SDL_SCANCODE_BACKSLASH]      = 0x2B;
    scancodes[SDL_SCANCODE_Z]              = 0x2C;
    scancodes[SDL_SCANCODE_X]              = 0x2D;
    scancodes[SDL_SCANCODE_C]              = 0x2E;
    scancodes[SDL_SCANCODE_V]              = 0x2F;
    scancodes[SDL_SCANCODE_B]              = 0x30;
    scancodes[SDL_SCANCODE_N]              = 0x31;
    scancodes[SDL_SCANCODE_M]              = 0x32;
    scancodes[SDL_SCANCODE_COMMA]          = 0x33;
    scancodes[SDL_SCANCODE_PERIOD]         = 0x34;
    scancodes[SDL_SCANCODE_SLASH]          = 0x35;
    scancodes[SDL_SCANCODE_RSHIFT]         = 0x36;
    scancodes[SDL_SCANCODE_KP_MULTIPLY]    = 0x37;
    scancodes[SDL_SCANCODE_LALT]           = 0x38;
    scancodes[SDL_SCANCODE_SPACE]          = 0x39;
    scancodes[SDL_SCANCODE_CAPSLOCK]       = 0x3A;
    scancodes[SDL_SCANCODE_F1]             = 0x3B;
    scancodes[SDL_SCANCODE_F2]             = 0x3C;
    scancodes[SDL_SCANCODE_F3]             = 0x3D;
    scancodes[SDL_SCANCODE_F4]             = 0x3E;
    scancodes[SDL_SCANCODE_F5]             = 0x3F;
    scancodes[SDL_SCANCODE_F6]             = 0x40;
    scancodes[SDL_SCANCODE_F7]             = 0x41;
    scancodes[SDL_SCANCODE_F8]             = 0x42;
    scancodes[SDL_SCANCODE_F9]             = 0x43;
    scancodes[SDL_SCANCODE_F10]            = 0x44;
    scancodes[SDL_SCANCODE_NUMLOCKCLEAR]   = 0x45;
    scancodes[SDL_SCANCODE_SCROLLLOCK]     = 0x46;
    scancodes[SDL_SCANCODE_KP_7]           = 0x47;
    scancodes[SDL_SCANCODE_KP_8]           = 0x48;
    scancodes[SDL_SCANCODE_KP_9]           = 0x49;
    scancodes[SDL_SCANCODE_KP_MINUS]       = 0x4A;
    scancodes[SDL_SCANCODE_KP_4]           = 0x4B;
    scancodes[SDL_SCANCODE_KP_5]           = 0x4C;
    scancodes[SDL_SCANCODE_KP_6]           = 0x4D;
    scancodes[SDL_SCANCODE_KP_PLUS]        = 0x4E;
    scancodes[SDL_SCANCODE_KP_1]           = 0x4F;
    scancodes[SDL_SCANCODE_KP_2]           = 0x50;
    scancodes[SDL_SCANCODE_KP_3]           = 0x51;
    scancodes[SDL_SCANCODE_KP_0]           = 0x52;
    scancodes[SDL_SCANCODE_KP_PERIOD]      = 0x53;
    scancodes[SDL_SCANCODE_F11]            = 0x57;
    scancodes[SDL_SCANCODE_F12]            = 0x58;
    scancodes[SDL_SCANCODE_PAUSE]          = 0x59;

    /* extended DOS scancodes... */
    scancodes[SDL_SCANCODE_KP_ENTER]       = 0xE01C;
    scancodes[SDL_SCANCODE_RCTRL]          = 0xE01D;
    scancodes[SDL_SCANCODE_KP_DIVIDE]      = 0xE035;
    scancodes[SDL_SCANCODE_PRINTSCREEN]    = 0xE037;
    scancodes[SDL_SCANCODE_SYSREQ]         = 0xE037;
    scancodes[SDL_SCANCODE_RALT]           = 0xE038;
    scancodes[SDL_SCANCODE_HOME]           = 0xE047;
    scancodes[SDL_SCANCODE_UP]             = 0xE048;
    scancodes[SDL_SCANCODE_PAGEUP]         = 0xE049;
    scancodes[SDL_SCANCODE_LEFT]           = 0xE04B;
    scancodes[SDL_SCANCODE_RIGHT]          = 0xE04D;
    scancodes[SDL_SCANCODE_END]            = 0xE04F;
    scancodes[SDL_SCANCODE_DOWN]           = 0xE050;
    scancodes[SDL_SCANCODE_PAGEDOWN]       = 0xE051;
    scancodes[SDL_SCANCODE_INSERT]         = 0xE052;
    scancodes[SDL_SCANCODE_DELETE]         = 0xE053;

    if (has_altivec)
        sdldebug("CPU feature: Altivec\n");
} /* _platform_init */


int setvesa(int32_t x, int32_t y)
{
    fprintf(stderr, "setvesa() called in SDL driver!\n");
    exit(23);
    return(0);
} /* setvesa */

#ifdef USE_OPENGL
/*
 * RGB of the frame (JFBuild's GL branch), top row first, in a malloc'd
 *  buffer, or NULL. It's read from the back buffer: a frame being drawn is
 *  there; otherwise the last swapped frame is drawn there again first, as
 *  the back buffer is undefined after a swap (with no last frame, NULL).
 *  The fade the swap puts over it is applied here the same way, so it's
 *  the picture on screen, as in 8-bit. With no last-frame texture (over
 *  GL_MAX_TEXTURE_SIZE) it reads the front buffer, fade and all. The size
 *  is the drawable's.
 */
static unsigned char *gl_screencapture(int *w, int *h)
{
    unsigned char *rgb, *top, *bottom, t;
    size_t rowbytes, i, n;
    int y, a = palfadedelta, front = 0, failed;

    *w = xdim;
    *h = ydim;
    rgb = polymost_capture_shader();  /* the GL 2.1 tier's frame, as shown */
    if (rgb != NULL)
        return(rgb);
    if (!framedrawn)
    {
        if (lastframe == 0)
        {
            if (!swapped)  /* the front buffer holds nothing yet. */
                return(NULL);
            front = 1;
        } /* if */
        else if (!lastframe_set)
            return(NULL);
        else
        {
            gl_draw_lastframe();
            framedrawn = 0;  /* that's the last frame again, not a new one. */
        } /* else */
    } /* if */

    rowbytes = (size_t) *w * 3;
    rgb = (unsigned char *) malloc(rowbytes * *h);
    if (rgb == NULL)
        return(NULL);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    if (front)
    {
        glReadBuffer(GL_FRONT);
        a = 0;  /* the swap drew the fade there already. */
    } /* if */
    while (glGetError() != GL_NO_ERROR) {}  /* only the read's own error counts. */
    glReadPixels(0, 0, *w, *h, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    failed = (glGetError() != GL_NO_ERROR);
    while (glGetError() != GL_NO_ERROR) {}
    glReadBuffer(GL_BACK);
    while (glGetError() != GL_NO_ERROR) {}  /* leave no error behind. */
    if (failed)
    {
        free(rgb);
        return(NULL);
    } /* if */

    for (y = 0; y < *h / 2; y++)  /* GL rows are bottom-up. */
    {
        top = rgb + y * rowbytes;
        bottom = rgb + (*h - 1 - y) * rowbytes;
        for (i = 0; i < rowbytes; i++)
            { t = top[i]; top[i] = bottom[i]; bottom[i] = t; }
    } /* for */

    if (a)  /* gl_draw_fade's blend: palfadergb at alpha palfadedelta. */
        for (i = 0, n = rowbytes * *h; i < n; i += 3)
        {
            rgb[i+0] = (rgb[i+0] * (255 - a) + palfadergb.r * a + 127) / 255;
            rgb[i+1] = (rgb[i+1] * (255 - a) + palfadergb.g * a + 127) / 255;
            rgb[i+2] = (rgb[i+2] * (255 - a) + palfadergb.b * a + 127) / 255;
        } /* for */

    return(rgb);
} /* gl_screencapture */
#endif


/* RGB of the 8-bit frame through the palette on screen (the faded one), as
 *  the SDL surface holds it, so the picture is what's presented. */
static unsigned char *classic_screencapture(int *w, int *h, char inverseit)
{
    const SDL_Color *pal = surface->format->palette->colors;
    unsigned char *rgb, *out;
    int32_t i, p, numbytes;
    int col;

    if (qsetmode == 200)
    {
        *w = xdim;
        *h = ydim;
    } /* if */
    else  /* the editor's 2D mode: 640 wide, qsetmode high, 16 colors. */
    {
        *w = 640;
        *h = qsetmode;
    } /* else */

    numbytes = *w * *h;
    rgb = (unsigned char *) malloc(numbytes * 3);
    if (rgb == NULL)
        return(NULL);

    for (p = 0, out = rgb; p < numbytes; p++)
    {
        if (qsetmode == 200)
            col = ((unsigned char *) frameplace)[p];
        else
        {
            col = readpixel((intptr_t) surface->pixels + p);  /* as drawpixel16 writes it */
            if ((inverseit == 1) && (((col&7) == 0) || ((col&7) == 7))) col ^= 15;
            i = (col & 15) * 3;
            *out++ = vgapal16[i+0] << 2;
            *out++ = vgapal16[i+1] << 2;
            *out++ = vgapal16[i+2] << 2;
            continue;
        } /* else */

        *out++ = pal[col].r;
        *out++ = pal[col].g;
        *out++ = pal[col].b;
    } /* for */

    return(rgb);
} /* classic_screencapture */


/* stb_image_write's output: the file, and whether every write worked. */
typedef struct
{
    int fd;
    int ok;
} capturefile;

static void screencapture_write(void *context, void *data, int size)
{
    capturefile *out = (capturefile *) context;
    if ((out->ok) && (write(out->fd, data, size) != size))
        out->ok = 0;
} /* screencapture_write */


/*
 * The format is the file name's extension: .jpg, .bmp, or else PNG.
 *  The NNNN before it is replaced by the first number not yet taken.
 */
int screencapture(char *_filename, char inverseit)
{
    unsigned char *rgb;
    int fil = -1;
    int w, h, written;
    capturefile out;
    int captcnt;
    size_t flen = strlen(_filename);
    char *filename = alloca(flen + 1);
    const char *ext;
    strcpy(filename, _filename);

    for (captcnt = 0; (captcnt <= 9999) && (fil == -1); captcnt++)
    {
        filename[flen-8] = ((captcnt/1000)%10)+48;
        filename[flen-7] = ((captcnt/100)%10)+48;
        filename[flen-6] = ((captcnt/10)%10)+48;
        filename[flen-5] = (captcnt%10)+48;
        fil = open(filename, O_BINARY|O_CREAT|O_EXCL|O_WRONLY,
                    S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    }

    if (fil == -1)
        return -1;

#ifdef USE_OPENGL
    if (gl_drawing)
        rgb = gl_screencapture(&w, &h);
    else
#endif
        rgb = classic_screencapture(&w, &h, inverseit);

    out.fd = fil;
    out.ok = 1;
    written = 0;
    ext = filename + flen - 3;
    if (rgb == NULL)
        out.ok = 0;
    else if (SDL_strcasecmp(ext, "jpg") == 0)
        written = stbi_write_jpg_to_func(screencapture_write, &out, w, h, 3, rgb, 90);
    else if (SDL_strcasecmp(ext, "bmp") == 0)
        written = stbi_write_bmp_to_func(screencapture_write, &out, w, h, 3, rgb);
    else
        written = stbi_write_png_to_func(screencapture_write, &out, w, h, 3, rgb, w * 3);

    free(rgb);
    if (close(fil) != 0)
        out.ok = 0;
    if ((!written) || (!out.ok))  /* no capture, or no whole file: none. */
    {
        remove(filename);
        return(-1);
    } /* if */
    return(0);
} /* screencapture */


void setvmode(int mode)
{
    int w = -1;
    int h = -1;

    if (mode == 0x3)  /* text mode. */
    {
        shutdown_video();
        return;
    } /* if */

    if (mode == 0x13)
    {
        w = 800;
        h = 600;
    } /* if */

    else
    {
        fprintf(stderr, "setvmode(0x%x) is unsupported in SDL driver.\n", mode);
        exit(13);
    } /* if */

    assert(w > 0);
    assert(h > 0);

    go_to_new_vid_mode(-1, w, h);
} /* setvmode */


int _setgamemode(char davidoption, int32_t daxdim, int32_t daydim)
{
#ifdef USE_OPENGL
    int32_t glw = daxdim, glh = daydim;
#endif

    exclusive_w = daxdim;  /* the size asked for, before the clamp */
    exclusive_h = daydim;

    /* 8-bit modes keep the original limit; MAXXDIM/MAXYDIM grew for OpenGL. */
    if (daxdim > 1600 || daydim > 1200)
    {
	    daxdim = 1600;
	    daydim = 1200;
    }

    if (in_egapalette)
        restore256_palette();

    go_to_new_vid_mode((int) davidoption, daxdim, daydim);
#ifdef USE_OPENGL
    /* but renderer 3's window gets the size asked for. */
    if ((glw != daxdim) || (glh != daydim))
    {
        gl_window_w = glw;
        gl_window_h = glh;
    } /* if */
#endif

    qsetmode = 200;
    last_render_ticks = SDL_GetTicks();
    return(0);
} /* setgamemode */


void qsetmode640350(void)
{
    assert(0);

    go_to_new_vid_mode(-1, 640, 350);
} /* qsetmode640350 */


void qsetmode640480(void)
{
    if (!in_egapalette)
    	set16color_palette();

    go_to_new_vid_mode(-1, 640, 480);
    pageoffset = 0;	/* Make sure it goes to the right place - DDOI */
    fillscreen16(0L,8L,640L*144L);
} /* qsetmode640480 */


static int get_dimensions_from_str(const char *str, int32_t *_w, int32_t *_h)
{
    char *xptr = NULL;
    char *ptr = NULL;
    int32_t w = -1;
    int32_t h = -1;

    if (str == NULL)
        return(0);

    xptr = strchr(str, 'x');
    if (xptr == NULL)
        return(0);

    w = (int32_t) strtol(str, &ptr, 10);
    if (ptr != xptr)
        return(0);

    xptr++;
    h = (int32_t) strtol(xptr, &ptr, 10);
    if ( (*xptr == '\0') || (*ptr != '\0') )
        return(0);

    if ((w <= 1) || (h <= 1))
        return(0);

    if (_w != NULL)
        *_w = w;

    if (_h != NULL)
        *_h = h;

    return(1);
} /* get_dimensions_from_str */


static __inline void get_max_screen_res(int32_t *max_w, int32_t *max_h)
{
    int32_t w = DEFAULT_MAXRESWIDTH;
    int32_t h = DEFAULT_MAXRESHEIGHT;
    const char *envr = getenv(BUILD_MAXSCREENRES);

    if (envr != NULL)
    {
        if (!get_dimensions_from_str(envr, &w, &h))
        {
            sdldebug("User's resolution ceiling [%s] is bogus!", envr);
            w = DEFAULT_MAXRESWIDTH;
            h = DEFAULT_MAXRESHEIGHT;
        } /* if */
    } /* if */

    if (max_w != NULL)
        *max_w = w;

    if (max_h != NULL)
        *max_h = h;
} /* get_max_screen_res */


static void add_vesa_mode(const char *typestr, int w, int h)
{
    sdldebug("Adding %s resolution (%dx%d).", typestr, w, h);
    validmode[validmodecnt] = validmodecnt;
    validmodexdim[validmodecnt] = w;
    validmodeydim[validmodecnt] = h;
    validmodecnt++;
} /* add_vesa_mode */


/* Let the user specify a specific mode via environment variable. */
static __inline void add_user_defined_resolution(void)
{
    int32_t w;
    int32_t h;
    const char *envr = getenv(BUILD_USERSCREENRES);

    if (envr == NULL)
        return;

    if (get_dimensions_from_str(envr, &w, &h))
        add_vesa_mode("user defined", w, h);
    else
        sdldebug("User defined resolution [%s] is bogus!", envr);
} /* add_user_defined_resolution */


static __inline void add_physical_resolutions(void)
{
    SDL_DisplayMode mode;
    int nummodes = SDL_GetNumDisplayModes(0);
    int i;

    if (nummodes < 1)
    {
        sdldebug("Couldn't get any physical resolutions.");
        return;
    } /* if */

    for (i = 0; i < nummodes; i++)
    {
        if (SDL_GetDisplayMode(0, i, &mode) == 0)
            add_vesa_mode("physical", mode.w, mode.h);
    } /* for */
} /* add_physical_resolutions */


static void remove_vesa_mode(int index, const char *reason)
{
    int i;

    assert(index < validmodecnt);
    sdldebug("Removing resolution #%d, %dx%d [%s].",
              index, validmodexdim[index], validmodeydim[index], reason);

    for (i = index; i < validmodecnt - 1; i++)
    {
        validmode[i] = validmode[i + 1];
        validmodexdim[i] = validmodexdim[i + 1];
        validmodeydim[i] = validmodeydim[i + 1];
    } /* for */

    validmodecnt--;
} /* remove_vesa_mode */


static __inline void cull_large_vesa_modes(void)
{
    int32_t max_w;
    int32_t max_h;
    int i;

    get_max_screen_res(&max_w, &max_h);
    sdldebug("Setting resolution ceiling to (%dx%d).", max_w, max_h);

    for (i = 0; i < validmodecnt; i++)
    {
        if ((validmodexdim[i] > max_w) || (validmodeydim[i] > max_h))
        {
            remove_vesa_mode(i, "above resolution ceiling");
            i--;  /* list shrinks. */
        } /* if */
    } /* for */
} /* cull_large_vesa_modes */


static __inline void cull_duplicate_vesa_modes(void)
{
    int i;
    int j;

    for (i = 0; i < validmodecnt; i++)
    {
        for (j = i + 1; j < validmodecnt; j++)
        {
            if ( (validmodexdim[i] == validmodexdim[j]) &&
                 (validmodeydim[i] == validmodeydim[j]) )
            {
                remove_vesa_mode(j, "duplicate");
                j--;  /* list shrinks. */
            } /* if */
        } /* for */
    } /* for */
} /* cull_duplicate_vesa_modes */


#define swap_macro(tmp, x, y) { tmp = x; x = y; y = tmp; }

/* be sure to call cull_duplicate_vesa_modes() before calling this. */
static __inline void sort_vesa_modelist(void)
{
    int i;
    int sorted;
    int32_t tmp;

    do
    {
        sorted = 1;
        for (i = 0; i < validmodecnt - 1; i++)
        {
            if ( (validmodexdim[i] >= validmodexdim[i+1]) &&
                 (validmodeydim[i] >= validmodeydim[i+1]) )
            {
                sorted = 0;
                swap_macro(tmp, validmode[i], validmode[i+1]);
                swap_macro(tmp, validmodexdim[i], validmodexdim[i+1]);
                swap_macro(tmp, validmodeydim[i], validmodeydim[i+1]);
            } /* if */
        } /* for */
    } while (!sorted);
} /* sort_vesa_modelist */


/* the engine assumes bytesperline == width, but SDL pads surface rows to 4. */
static __inline void cull_unaligned_vesa_modes(void)
{
    int i;

    for (i = 0; i < validmodecnt; i++)
    {
        if (validmodexdim[i] & 3)
        {
            remove_vesa_mode(i, "width not a multiple of 4");
            i--;  /* list shrinks. */
        } /* if */
    } /* for */
} /* cull_unaligned_vesa_modes */


static __inline void cleanup_vesa_modelist(void)
{
    cull_unaligned_vesa_modes();
    cull_large_vesa_modes();
    cull_duplicate_vesa_modes();
    sort_vesa_modelist();
} /* cleanup_vesa_modelist */


static __inline void output_vesa_modelist(void)
{
    char buffer[256];
    char numbuf[20];
    int i;

    if (!_sdl_debug_file)
        return;

    buffer[0] = '\0';

    for (i = 0; i < validmodecnt; i++)
    {
        sprintf(numbuf, " (%dx%d)",
                  (int32_t) validmodexdim[i], (int32_t) validmodeydim[i]);

        if ( (strlen(buffer) + strlen(numbuf)) >= (sizeof (buffer) - 1) )
            strcpy(buffer + (sizeof (buffer) - 5), " ...");
        else
            strcat(buffer, numbuf);
    } /* for */

    sdldebug("Final sorted modelist:%s", buffer);
} /* output_vesa_modelist */


void getvalidvesamodes(void)
{
    static int already_checked = 0;
    int i;
    int stdres[][2] = {
                        {320, 200}, {320, 240}, {640, 350},
			{640, 480}, {800, 600}, {1024, 768}
                      };

    if (already_checked)
        return;

    already_checked = 1;
   	validmodecnt = 0;
    vidoption = 1;  /* !!! tmp */

        /* fill in the standard resolutions... */
    for (i = 0; i < sizeof (stdres) / sizeof (stdres[0]); i++)
        add_vesa_mode("standard", stdres[i][0], stdres[i][1]);

         /* Anything the hardware can specifically do is added now... */
    add_physical_resolutions();

        /* Now add specific resolutions that the user wants... */
    add_user_defined_resolution();

        /* get rid of dupes and bogus resolutions... */
    cleanup_vesa_modelist();

        /* print it out for debugging purposes... */
    output_vesa_modelist();
} /* getvalidvesamodes */


int VBE_setPalette(int32_t start, int32_t num, char *palettebuffer)
/*
 * (From Ken's docs:)
 *   Set (num) palette palette entries starting at (start)
 *   palette entries are in a 4-byte format in this order:
 *       0: Blue (0-63)
 *       1: Green (0-63)
 *       2: Red (0-63)
 *       3: Reserved
 *
 * Naturally, the bytes are in the reverse order that SDL wants them...
 *  More importantly, SDL wants the color elements in a range from 0-255,
 *  so we do a conversion.
 */
{
    SDL_Color fmt_swap[256];
    SDL_Color *sdlp = &fmt_swap[start];
    char *p = palettebuffer;
    int i;

    assert( (start + num) <= (sizeof (fmt_swap) / sizeof (SDL_Color)) );

#ifdef USE_OPENGL
    if (gl_drawing)  /* 6-bit BGR0 to 8-bit, then retexture. */
    {
        palette_t newpal[256];

        for (i = 0; i < num; i++, p += 4)
        {
            newpal[i].b = (p[0] << 2) | (p[0] >> 4);
            newpal[i].g = (p[1] << 2) | (p[1] >> 4);
            newpal[i].r = (p[2] << 2) | (p[2] >> 4);
            newpal[i].f = 0;
        } /* for */
        /* the same palette again (a flash's end) needn't retexture. */
        if (memcmp(&curpalette[start], newpal, num * sizeof (palette_t)) != 0)
        {
            memcpy(&curpalette[start], newpal, num * sizeof (palette_t));
            if (gldrawpath != GLPATH_SHADER)  /* that one looks the palette up as it shows a frame */
            {
                gltexinvalidateall();
                polymost_buildshaderamp();
            } /* if */
        } /* if */
        return(1);
    } /* if */
#endif

    for (i = 0; i < num; i++)
    {
        sdlp->b = (Uint8) ((((float) *p++) / 63.0) * 255.0);
        sdlp->g = (Uint8) ((((float) *p++) / 63.0) * 255.0);
        sdlp->r = (Uint8) ((((float) *p++) / 63.0) * 255.0);
        sdlp->a = 255;
        p++;   /* This byte is unused in BUILD. */

        sdlp++;
    } /* for */

    if (surface == NULL)
        return(0);

    if (SDL_SetPaletteColors(surface->format->palette, &fmt_swap[start],
                             start, num) != 0)
        return(0);

    /* SDL 1.2 hardware palettes changed instantly; we must re-present. */
    present_surface();
    return(1);
} /* VBE_setPalette */


int VBE_getPalette(int32_t start, int32_t num, char *palettebuffer)
{
    SDL_Color *sdlp;
    char *p = palettebuffer + (start * 4);
    int i;

#ifdef USE_OPENGL
    if (gl_drawing)
    {
        for (i = start; i < start + num; i++)
        {
            *p++ = curpalette[i].b >> 2;
            *p++ = curpalette[i].g >> 2;
            *p++ = curpalette[i].r >> 2;
            *p++ = 0;
        } /* for */
        return(1);
    } /* if */
#endif

    sdlp = surface->format->palette->colors + start;

    for (i = 0; i < num; i++)
    {
        *p++ = (Uint8) ((((float) sdlp->b) / 255.0) * 63.0);
        *p++ = (Uint8) ((((float) sdlp->g) / 255.0) * 63.0);
        *p++ = (Uint8) ((((float) sdlp->r) / 255.0) * 63.0);
        *p++ = 0;   /* This byte is unused in BUILD. */
        sdlp++;
    } /* for */

    return(1);
} /* VBE_getPalette */


void _uninitengine(void)
{
   shutdown_video();
} /* _uninitengine */


void uninitvesa(void)
{
   shutdown_video();
} /* uninitvesa */


int setupmouse(void)
{
    int w, h;

    if (sdl_window == NULL)
        return(0);

    set_mouse_grab(mouse_grabbed);
    SDL_ShowCursor(0);

    mouse_area(&w, &h);
    mouse_x = w / 2;
    mouse_y = h / 2;
    mouse_relative_x = mouse_relative_y = 0;

        /*
         * this global usually gets set by BUILD, but it's a one-shot
         *  deal, and we may not have an SDL surface at that point. --ryan.
         */
    moustat = 1;

    return(1);
} /* setupmouse */


void readmousexy(short *x, short *y)
{
    if (x) *x = mouse_relative_x << 2;
    if (y) *y = mouse_relative_y << 2;

    mouse_relative_x = mouse_relative_y = 0;
} /* readmousexy */


void readmousebstatus(short *bstatus)
{
    if (bstatus)
        *bstatus = mouse_buttons;

    // special wheel treatment
    if(mouse_buttons&8) mouse_buttons ^= 8;
    if(mouse_buttons&16) mouse_buttons ^= 16;

} /* readmousebstatus */


static unsigned char mirrorcolor = 0;

void _updateScreenRect(int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (renderer == RENDERER_SOFTWARE)
        present_surface();  /* the whole frame is cheap to push nowadays. */
} /* _updatescreenrect */


void _nextpage(void)
{
    Uint32 ticks;

    handle_events();

#ifdef USE_OPENGL
    if (gl_drawing)
        gl_swap();
    else
#endif
    if (renderer == RENDERER_SOFTWARE)
    {
        if (qsetmode == 200)
            memcpy(surface->pixels, (const void *) frameplace, surface->w * surface->h);
        present_surface();
    } /* if */

    if ((debug_hall_of_mirrors) && (qsetmode == 200) && (frameplace))
    {
        memset((void *) frameplace, mirrorcolor, surface->w * surface->h);
        mirrorcolor++;
    } /* if */

    ticks = SDL_GetTicks();
    total_render_time = (ticks - last_render_ticks);
    if (total_render_time > 1000)
    {
        total_rendered_frames = 0;
        total_render_time = 1;
        last_render_ticks = ticks;
    } /* if */
    total_rendered_frames++;
} /* _nextpage */


unsigned char readpixel(intptr_t offset)
{
    return( *((unsigned char *) offset) );
} /* readpixel */

void drawpixel(intptr_t offset, unsigned char pixel)
{
    *((unsigned char *) offset) = pixel;
} /* drawpixel */


/* !!! These are incorrect. */
void drawpixels(int32_t offset, unsigned short pixels)
{
    Uint8 *surface_end;
    Uint16 *pos;

                printf("Blargh!\n");
                exit(91);

    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);

    surface_end = (((Uint8 *) surface->pixels) + (surface->w * surface->h)) - 2;
    pos = (Uint16 *) (((Uint8 *) surface->pixels) + offset);
    if ((pos >= (Uint16 *) surface->pixels) && (pos < (Uint16 *) surface_end))
        *pos = pixels;

    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
} /* drawpixels */


void drawpixelses(int32_t offset, unsigned int pixelses)
{
    Uint8 *surface_end;
    Uint32 *pos;

                printf("Blargh!\n");
                exit(91);

    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);

    surface_end = (((Uint8 *)surface->pixels) + (surface->w * surface->h)) - 2;
    pos = (Uint32 *) (((Uint8 *) surface->pixels) + offset);
    if ((pos >= (Uint32 *) surface->pixels) && (pos < (Uint32 *) surface_end))
        *pos = pixelses;

    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);
} /* drawpixelses */


/* Fix this up The Right Way (TM) - DDOI */
void setcolor16(int col)
{
	drawpixel_color = col;
}

void drawpixel16(int32_t offset)
{
    drawpixel(((intptr_t) surface->pixels + offset), drawpixel_color);
} /* drawpixel16 */


void fillscreen16(int32_t offset, int32_t color, int32_t blocksize)
{
    Uint8 *surface_end;
    Uint8 *wanted_end;
    Uint8 *pixels;

    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);

    pixels = get_framebuffer();

    /* Make this function pageoffset aware - DDOI */
    if (!pageoffset) { 
	    offset = offset << 3;
	    offset += 640*336;
    }

    surface_end = (pixels + (surface->w * surface->h)) - 1;
    wanted_end = (pixels + offset) + blocksize;

    if (offset < 0)
        offset = 0;

    if (wanted_end > surface_end)
        blocksize = (int32_t) (surface_end - (pixels + offset));

    memset(pixels + offset, (int) color, blocksize);

    if (SDL_MUSTLOCK(surface))
        SDL_UnlockSurface(surface);

    _nextpage();
} /* fillscreen16 */


/* Most of this line code is taken from Abrash's "Graphics Programming Blackbook".
Remember, sharing code is A Good Thing. AH */
static __inline void DrawHorizontalRun (char **ScreenPtr, int XAdvance, int RunLength, char Color)
{
    int i;
    char *WorkingScreenPtr = *ScreenPtr;

    for (i=0; i<RunLength; i++)
    {
        *WorkingScreenPtr = Color;
        WorkingScreenPtr += XAdvance;
    }
    WorkingScreenPtr += surface->w;
    *ScreenPtr = WorkingScreenPtr;
}

static __inline void DrawVerticalRun (char **ScreenPtr, int XAdvance, int RunLength, char Color)
{
    int i;
    char *WorkingScreenPtr = *ScreenPtr;

    for (i=0; i<RunLength; i++)
    {
      	*WorkingScreenPtr = Color;
    	WorkingScreenPtr += surface->w;
    }
    WorkingScreenPtr += XAdvance;
    *ScreenPtr = WorkingScreenPtr;
}

void drawline16(int32_t XStart, int32_t YStart, int32_t XEnd, int32_t YEnd, char Color)
{
    int Temp, AdjUp, AdjDown, ErrorTerm, XAdvance, XDelta, YDelta;
    int WholeStep, InitialPixelCount, FinalPixelCount, i, RunLength;
    char *ScreenPtr;
    int32_t dx, dy;

    if (SDL_MUSTLOCK(surface))
        SDL_LockSurface(surface);

	dx = XEnd-XStart; dy = YEnd-YStart;
	if (dx >= 0)
	{
		if ((XStart > 639) || (XEnd < 0)) return;
		if (XStart < 0) { if (dy) YStart += scale(0-XStart,dy,dx); XStart = 0; }
		if (XEnd > 639) { if (dy) YEnd += scale(639-XEnd,dy,dx); XEnd = 639; }
	}
	else
	{
		if ((XEnd > 639) || (XStart < 0)) return;
		if (XEnd < 0) { if (dy) YEnd += scale(0-XEnd,dy,dx); XEnd = 0; }
		if (XStart > 639) { if (dy) YStart += scale(639-XStart,dy,dx); XStart = 639; }
	}
	if (dy >= 0)
	{
		if ((YStart >= ydim16) || (YEnd < 0)) return;
		if (YStart < 0) { if (dx) XStart += scale(0-YStart,dx,dy); YStart = 0; }
		if (YEnd >= ydim16) { if (dx) XEnd += scale(ydim16-1-YEnd,dx,dy); YEnd = ydim16-1; }
	}
	else
	{
		if ((YEnd >= ydim16) || (YStart < 0)) return;
		if (YEnd < 0) { if (dx) XEnd += scale(0-YEnd,dx,dy); YEnd = 0; }
		if (YStart >= ydim16) { if (dx) XStart += scale(ydim16-1-YStart,dx,dy); YStart = ydim16-1; }
	}

	/* Make sure the status bar border draws correctly - DDOI */
	if (!pageoffset) { YStart += 336; YEnd += 336; }

    /* We'll always draw top to bottom */
    if (YStart > YEnd) {
        Temp = YStart;
        YStart = YEnd;
        YEnd = Temp;
        Temp = XStart;
        XStart = XEnd;
        XEnd = Temp;
    }

    /* Point to the bitmap address first pixel to draw */
    ScreenPtr = (char *) (get_framebuffer()) + XStart + (surface->w * YStart);

    /* Figure out whether we're going left or right, and how far we're going horizontally */
    if ((XDelta = XEnd - XStart) < 0)
    {
        XAdvance = (-1);
        XDelta = -XDelta;
    } else {
        XAdvance = 1;
    }

    /* Figure out how far we're going vertically */
    YDelta = YEnd - YStart;

    /* Special cases: Horizontal, vertical, and diagonal lines */
    if (XDelta == 0)
    {
        for (i=0; i <= YDelta; i++)
        {
            *ScreenPtr = Color;
            ScreenPtr += surface->w;
        }

        UNLOCK_SURFACE_AND_RETURN;
    }
    if (YDelta == 0)
    {
    	for (i=0; i <= XDelta; i++)
    	{
      	    *ScreenPtr = Color;
    	    ScreenPtr += XAdvance;
    	}
        UNLOCK_SURFACE_AND_RETURN;
    }
    if (XDelta == YDelta)
    {
    	for (i=0; i <= XDelta; i++)
        {
            *ScreenPtr = Color;
            ScreenPtr += XAdvance + surface->w;
        }
        UNLOCK_SURFACE_AND_RETURN;
    }

    /* Determine whether the line is X or Y major, and handle accordingly */
    if (XDelta >= YDelta) /* X major line */
    {
        WholeStep = XDelta / YDelta;
        AdjUp = (XDelta % YDelta) * 2;
        AdjDown = YDelta * 2;
        ErrorTerm = (XDelta % YDelta) - (YDelta * 2);

        InitialPixelCount = (WholeStep / 2) + 1;
        FinalPixelCount = InitialPixelCount;

        if ((AdjUp == 0) && ((WholeStep & 0x01) == 0)) InitialPixelCount--;
        if ((WholeStep & 0x01) != 0) ErrorTerm += YDelta;

        DrawHorizontalRun(&ScreenPtr, XAdvance, InitialPixelCount, Color);

        for (i=0; i<(YDelta-1); i++)
        {
            RunLength = WholeStep;
            if ((ErrorTerm += AdjUp) > 0)
            {
        	RunLength ++;
        	ErrorTerm -= AdjDown;
            }

            DrawHorizontalRun(&ScreenPtr, XAdvance, RunLength, Color);
         }

         DrawHorizontalRun(&ScreenPtr, XAdvance, FinalPixelCount, Color);

         UNLOCK_SURFACE_AND_RETURN;
    } else {	/* Y major line */
    	WholeStep = YDelta / XDelta;
    	AdjUp = (YDelta % XDelta) * 2;
    	AdjDown = XDelta * 2;
        ErrorTerm = (YDelta % XDelta) - (XDelta * 2);
        InitialPixelCount = (WholeStep / 2) + 1;
        FinalPixelCount = InitialPixelCount;

        if ((AdjUp == 0) && ((WholeStep & 0x01) == 0)) InitialPixelCount --;
        if ((WholeStep & 0x01) != 0) ErrorTerm += XDelta;

        DrawVerticalRun(&ScreenPtr, XAdvance, InitialPixelCount, Color);

        for (i=0; i<(XDelta-1); i++)
        {
            RunLength = WholeStep;
            if ((ErrorTerm += AdjUp) > 0)
            {
            	RunLength ++;
            	ErrorTerm -= AdjDown;
            }

            DrawVerticalRun(&ScreenPtr, XAdvance, RunLength, Color);
        }

        DrawVerticalRun(&ScreenPtr, XAdvance, FinalPixelCount, Color);
        UNLOCK_SURFACE_AND_RETURN;
     }
} /* drawline16 */


void clear2dscreen(void)
{
    SDL_Rect rect;

    rect.x = rect.y = 0;
    rect.w = surface->w;

	if (qsetmode == 350)
        rect.h = 350;
	else if (qsetmode == 480)
	{
		if (ydim16 <= 336)
            rect.h = 336;
        else
            rect.h = 480;
	} /* else if */

    SDL_FillRect(surface, &rect, 0);
} /* clear2dscreen */


void _idle(void)
{
    if (sdl_window != NULL)
        handle_events();
    SDL_Delay(1);
} /* _idle */

void *_getVideoBase(void)
{
    return((void *) surface->pixels);
}

void setactivepage(int32_t dapagenum)
{
	/* !!! Is this really still needed? - DDOI */
    /*fprintf(stderr, "%s, line %d; setactivepage(): STUB.\n", __FILE__, __LINE__);*/
} /* setactivepage */

void limitrate(void)
{
    /* this is a no-op in SDL. It was for buggy VGA cards in DOS. */
} /* limitrate */

static Uint64 timer_start, timer_calls;

static Uint32 _timer_catcher(Uint32 interval, void *bleh)
{
    /*
     * Fix: SDL timers tick in whole milliseconds and drift late, so one
     *  call per callback ran the clock slow. Call timerhandler() once for
     *  every 1/PLATFORM_TIMER_HZ second that has passed instead.
     */
    Uint64 due = (SDL_GetPerformanceCounter() - timer_start) * PLATFORM_TIMER_HZ /
                 SDL_GetPerformanceFrequency();

    /*
     * Catch up on drift, but not on a stall (a debugger, Ctrl-Z, App Nap):
     *  replaying all of it would fast-forward the game. Drop the backlog.
     */
    if (due - timer_calls > PLATFORM_TIMER_HZ/4)
        timer_calls = due - 1;

    while (timer_calls < due)
    {
        timer_calls++;
        timerhandler();
    }
    /*
     * SDL 1.2 rounded the old "return(1)" up to its 10ms granularity;
     *  SDL2 honors it literally, which would run the game 10x too fast.
     */
    return(interval);
} /* _timer_catcher */

void inittimer(void)
{
    SDL_ClearError();
    timer_start = SDL_GetPerformanceCounter();
    timer_calls = 0;
    primary_timer = SDL_AddTimer(1000 / PLATFORM_TIMER_HZ, _timer_catcher, NULL);
    if (primary_timer == 0)
    {
        fprintf(stderr, "BUILDSDL: -ERROR- Problem initializing primary timer!\n");
        fprintf(stderr, "BUILDSDL:  Reason: [%s]\n", SDL_GetError());
        SDL_Quit();
        exit(2);
    } /* if */
}

void uninittimer(void)
{
    if (primary_timer != 0)
    {
        SDL_RemoveTimer(primary_timer);
        primary_timer = 0;
    } /* if */
}

void initkeys(void)
{
    /* does nothing in SDL. Key input handling is set up elsewhere. */
    /* !!! why not here? */
}

void uninitkeys(void)
{
    /* does nothing in SDL. Key input handling is set up elsewhere. */
}

void set16color_palette(void)
{
	/* Backup old palette */
	/*VBE_getPalette (0, 16, (char *)&backup_palette);*/
	memcpy (&backup_palette, &palette, 16*3);

	/* Set new palette */
	/*VBE_setPalette (0, 16, (char *)&egapalette);*/
	memcpy (&palette, &egapalette, 16*3);
	in_egapalette = 1;
} /* set16color_palette */

void restore256_palette(void)
{
	/*VBE_setPalette (0, 16, (char *)&backup_palette);*/
	memcpy (&palette, &backup_palette, 16*3);
	in_egapalette = 0;
} /* restore256_palette */

uint32_t getticks(void)
{
    return(SDL_GetTicks());
} /* getticks */

/* end of sdl_driver.c ... */

