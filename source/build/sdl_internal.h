/* Private interfaces between the SDL platform modules. */
#ifndef BUILD_SDL_INTERNAL_H
#define BUILD_SDL_INTERNAL_H
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
#include "../videoopts.h"

extern SDL_Window *sdl_window;
extern SDL_Surface *surface;
extern int sdl_mouse_grabbed;
extern int sdl_input_focused;
extern void (*sdl_input_poll)(void), (*sdl_input_reset)(void);
extern void (*sdl_input_event)(const SDL_Event *);
extern int sdl_displaymode, sdl_displaymode_wanted;
extern int sdl_debug_hall_of_mirrors;
extern char *sdl_titlelong;
extern FILE *sdl_debug_file;

#ifdef DC
#define sdl_debug(fmt,args...) printf("BUILDSDL: " fmt "\n",##args)
#else
void sdl_debug(const char *fmt, ...);
#endif
void sdl_sys_init_debugging(void);
void sdl_sys_detect_vmware(void);
void sdl_window_init_renderer_names(void);
void sdl_window_init_renderer(void);
void sdl_window_log_driver(void);
void sdl_window_present(void);
int sdl_window_toggle_fullscreen(void);
void sdl_window_mouse_area(int *w, int *h);
void sdl_input_init(void);
int sdl_key_filter(const SDL_Event *event);
int sdl_mouse_motion_filter(const SDL_Event *event);
int sdl_mouse_button_filter(const SDL_MouseButtonEvent *event);
int sdl_mouse_wheel_filter(const SDL_MouseWheelEvent *event);
#ifdef USE_OPENGL
extern SDL_GLContext sdl_glcontext;
extern int sdl_gl_drawing;
void sdl_window_gl_resize(void);
void sdl_window_gl_mouse8(SDL_MouseMotionEvent *motion);
#endif
#endif
