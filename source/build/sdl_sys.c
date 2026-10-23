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

/* Timing, idle processing and diagnostic output. */
#include "sdl_internal.h"

static SDL_TimerID primary_timer = 0;


int sdl_debug_hall_of_mirrors = 0;


FILE *sdl_debug_file = NULL;

#ifdef DC
#define sdl_debug(fmt,args...) printf("BUILDSDL: " fmt "\n",##args)
#else
/*
================
__out_sdldebug

Write and flush one subsystem-prefixed diagnostic line.
The caller has already checked that the debug stream exists.
================
*/
static __inline void __out_sdldebug(const char *subsystem,
                                  const char *fmt, va_list ap)
{
    fprintf(sdl_debug_file, "%s: ", subsystem);
    vfprintf(sdl_debug_file, fmt, ap);
    fprintf(sdl_debug_file, "\n");
    fflush(sdl_debug_file);
} /* __out_sdldebug */


/*
================
sdl_debug

Write an SDL backend diagnostic only when a debug stream is configured.
Keep varargs handling here so callers share the same prefix and flushing policy.
================
*/
void sdl_debug(const char *fmt, ...)
{
    va_list ap;

    if (sdl_debug_file)
    {
        va_start(ap, fmt);
        __out_sdldebug("BUILDSDL", fmt, ap);
        va_end(ap);
    } /* if */
} /* sdldebug */
#endif


static int in_vmware = 0;


static Uint64 timer_start, timer_calls;


/*
================
sdl_sys_init_debugging

Read the debug-file and hall-of-mirrors environment options at platform startup.
Replace any previous stream; a filename of minus selects stdout.
================
*/
void sdl_sys_init_debugging(void)
{
    const char *envr = getenv(BUILD_SDLDEBUG);

    sdl_debug_hall_of_mirrors = (getenv(BUILD_HALLOFMIRRORS) != NULL);

    if (sdl_debug_file != NULL)
    {
        fclose(sdl_debug_file);
        sdl_debug_file = NULL;
    } /* if */

    if (envr != NULL)
    {
        if (strcmp(envr, "-") == 0)
	    sdl_debug_file = stdout;
        else
            sdl_debug_file = fopen(envr, "w");

        if (sdl_debug_file == NULL)
            printf("BUILDSDL: -WARNING- Could not open debug file!\n");
        else
            setbuf(sdl_debug_file, NULL);
    } /* if */
} /* init_debugging */


/*
================
sdl_sys_detect_vmware

Retain the legacy detection hook with its active path clearing the VMware flag.
The privileged probe remains in the disabled compatibility branch.
================
*/
void sdl_sys_detect_vmware(void)
{
#if 1
    in_vmware = 0;  /* oh well. */
#else
    /* !!! need root access to touch i/o ports on Linux. */
    #if (!defined __linux__)
        in_vmware = (int) is_vmware_running();
    #endif
    sdl_debug("vmWare %s running.", (in_vmware) ? "is" : "is not");
#endif
} /* detect_vmware */


/*
================
_idle

Pump platform events while a window exists, then yield briefly to SDL.
Wait loops use this to keep input and window events moving.
================
*/
void _idle(void)
{
    if (sdl_window != NULL)
        _handle_events();
    SDL_Delay(1);
} /* _idle */


/*
================
_timer_catcher

Advance the Build clock at a fixed rate using elapsed performance-counter time.
Drop a long-stall backlog instead of fast-forwarding the game on resume.
================
*/
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


/*
================
inittimer

Start the SDL callback with its performance-counter epoch and logical count reset.
The callback derives Build ticks from elapsed time rather than callback frequency.
================
*/
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


/*
================
uninittimer

Remove the registered primary timer when present and clear its handle.
Repeated shutdown calls have no timer left to remove.
================
*/
void uninittimer(void)
{
    if (primary_timer != 0)
    {
        SDL_RemoveTimer(primary_timer);
        primary_timer = 0;
    } /* if */
}


/*
================
getticks

Return SDL's millisecond clock, separate from the fixed-rate Build game clock.
The unsigned counter wraps; elapsed-time callers subtract unsigned values.
================
*/
uint32_t getticks(void)
{
    return(SDL_GetTicks());
} /* getticks */
