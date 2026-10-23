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

/* Platform startup and event dispatch. */
#include "sdl_internal.h"

int _argc = 0;
char **_argv = NULL;


char *sdl_titlelong = NULL;
static char *titleshort = NULL;


#if (!defined __DATE__)
#define __DATE__ "a long, long time ago"
#endif


/*
================
root_sdl_event_filter

Route queued SDL events to the input and window owners.
Keep delivery synchronous: Build's key handler consumes each generated scancode immediately.
================
*/
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
                sdl_window_present();
#ifdef USE_OPENGL
            else if ((event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) &&
                     (sdl_gl_drawing))
                sdl_window_gl_resize();
            else if ((event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) &&
                     (sdl_glcontext != NULL))
                sdl_window_present();  /* an 8-bit mode, at the new size */
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


/* Public entry to the private event pump. */
void _handle_events(void)
{
    handle_events();
} /* _handle_events */


static __inline void output_sdl_versions(void)
{
    SDL_version linked;
    const SDL_version *linked_ver = &linked;
    SDL_version compiled_ver;

    SDL_VERSION(&compiled_ver);
    SDL_GetVersion(&linked);

    sdl_debug("SDL display driver for the BUILD engine initializing.");
    sdl_debug("  sdl_driver.c by Ryan C. Gordon (icculus@clutteredmind.org).");
    sdl_debug("Compiled %s against SDL version %d.%d.%d ...", __DATE__,
                compiled_ver.major, compiled_ver.minor, compiled_ver.patch);
    sdl_debug("Linked SDL version is %d.%d.%d ...",
                linked_ver->major, linked_ver->minor, linked_ver->patch);
} /* output_sdl_versions */


/* lousy -ansi flag.  :) */
static char *string_dupe(const char *str)
{
    char *retval = malloc(strlen(str) + 1);
    if (retval != NULL)
        strcpy(retval, str);
    return(retval);
} /* string_dupe */


/*
================
_platform_init

Initialize platform paths, SDL options and the positional keyboard map.
Keep option setup before SDL initialization and scancode setup before event pumping.
================
*/
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

    sdl_window_init_renderer_names();

    sdl_sys_init_debugging();


    if (title == NULL)
        title = "BUILD";

    if (icon == NULL)
        icon = "BUILD";

    sdl_titlelong = string_dupe(title);
    titleshort = string_dupe(icon);

    if (getenv(BUILD_NOMOUSEGRAB) == NULL)
        sdl_mouse_grabbed = 1;
    else
        sdl_mouse_grabbed = 0;

    sdl_displaymode = (getenv(BUILD_WINDOWED) != NULL) ? DISPLAY_WINDOWED : sdl_displaymode_wanted;

    sdl_window_init_renderer();

    output_sdl_versions();
    sdl_window_log_driver();
    sdl_sys_detect_vmware();

    sdl_input_init();

    if (has_altivec)
        sdl_debug("CPU feature: Altivec\n");
} /* _platform_init */
