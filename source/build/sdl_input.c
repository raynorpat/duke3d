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

/* Keyboard, mouse and joystick input. */
#include "sdl_internal.h"

static int32_t mouse_x = 0;
static int32_t mouse_y = 0;
static int32_t mouse_relative_x = 0;
static int32_t mouse_relative_y = 0;
static short mouse_buttons = 0;
int sdl_mouse_grabbed = 0;
static unsigned int lastkey = 0;


static unsigned int scancodes[SDL_NUM_SCANCODES];


static SDL_Joystick *joystick = NULL;


void (*sdl_input_poll)(void), (*sdl_input_reset)(void);
void (*sdl_input_event)(const SDL_Event *);
int sdl_input_focused = 1;
struct mousewheel { int count; struct mousewheel *next; };
static struct mousewheel *wheel_head, *wheel_tail;
static double wheel_fraction;

/*
================
setinputcallback

Install the game's frame-poll, context-reset and event-latch callbacks.
================
*/
void setinputcallback(void (*poll)(void), void (*reset)(void),
                      void (*event)(const SDL_Event *))
{
    sdl_input_poll = poll; sdl_input_reset = reset; sdl_input_event = event;
}

/*
================
readmousewheel

Consume one signed step from the oldest queued wheel event.
Retain the remaining count so repeated and reversed steps keep their order.
================
*/
int readmousewheel(void)
{
    struct mousewheel *w = wheel_head;
    int direction;
    if (w == NULL) return 0;
    direction = w->count > 0 ? 1 : -1;
    w->count -= direction;
    if (w->count == 0) {
        wheel_head = w->next;
        if (wheel_head == NULL) wheel_tail = NULL;
        SDL_free(w);
    }
    return direction;
}

/*
================
clearmouseinput

Clear motion, legacy button bits and precise/queued wheel state together.
================
*/
void clearmouseinput(void)
{
    mouse_relative_x = mouse_relative_y = mouse_buttons = 0;
    wheel_fraction = 0;
    while (wheel_head != NULL) {
        struct mousewheel *next = wheel_head->next;
        SDL_free(wheel_head);
        wheel_head = next;
    }
    wheel_tail = NULL;
}

/*
================
readmousexy32

Consume both relative mouse axes at Build's fourfold scale using full-width outputs.
================
*/
void readmousexy32(int32_t *x, int32_t *y)
{
    if (x) *x = mouse_relative_x * 4;
    if (y) *y = mouse_relative_y * 4;
    mouse_relative_x = mouse_relative_y = 0;
}

/*
================
_joystick_instance

Return the active SDL joystick instance ID, or -1 when no device is open.
================
*/
int _joystick_instance(void)
{
    return joystick != NULL ? SDL_JoystickInstanceID(joystick) : -1;
}

/*
================
set_mouse_grab

Request relative mouse mode and window capture from SDL.
The caller owns the stored grab-policy flag.
================
*/
static void set_mouse_grab(int grab)
{
    SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE);
    if (sdl_window != NULL)
        SDL_SetWindowGrab(sdl_window, grab ? SDL_TRUE : SDL_FALSE);
} /* set_mouse_grab */


/*
================
sdl_mouse_button_filter

Translate SDL button numbering into Build's left/right/middle bit order.
Release clears the mapped bit; wheel pulses are handled separately.
================
*/
int sdl_mouse_button_filter(SDL_MouseButtonEvent const *event)
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
    if (!sdl_input_focused || button > SDL_BUTTON_RIGHT)
        return(0);

    if (button == SDL_BUTTON_RIGHT)
        button = SDL_BUTTON_MIDDLE;
    else if (button == SDL_BUTTON_MIDDLE)
        button = SDL_BUTTON_RIGHT;

    if (((const SDL_MouseButtonEvent*)event)->state)
        mouse_buttons |= 1<<(button-1);
    else if (button != 4 && button != 5)
        mouse_buttons &= ~(1<<(button-1));

#if 0
    Uint8 bmask = SDL_GetMouseState(NULL, NULL);
    mouse_buttons = 0;
    if (bmask & SDL_BUTTON_LMASK) mouse_buttons |= 1;
    if (bmask & SDL_BUTTON_RMASK) mouse_buttons |= 2;
    if (bmask & SDL_BUTTON_MMASK) mouse_buttons |= 4;
#endif

    return(0);
}

/*
================
sdl_mouse_wheel_filter

Queue ordered wheel steps for game input and latch legacy direction bits.
Preserve precise-wheel fractions between events.
================
*/
int sdl_mouse_wheel_filter(SDL_MouseWheelEvent const *event)
{
    Sint32 y = event->y;
    struct mousewheel *w;
    if (!sdl_input_focused) return 0;
    if (event->direction == SDL_MOUSEWHEEL_FLIPPED) y = -y;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    if (event->preciseY != 0.0f) {
        double delta = event->preciseY;
        if (event->direction == SDL_MOUSEWHEEL_FLIPPED) delta = -delta;
        wheel_fraction += delta;
        y = (Sint32)wheel_fraction;
        wheel_fraction -= y;
    }
#endif
    if (y == 0) return 0;
    if (y > 0) mouse_buttons |= 8;
    else mouse_buttons |= 16;
    if (sdl_input_poll == NULL) return 0;
    w = SDL_malloc(sizeof(*w));
    if (w == NULL) return 0;
    w->count = y; w->next = NULL;
    if (wheel_tail != NULL) wheel_tail->next = w;
    else wheel_head = w;
    wheel_tail = w;
    return 0;
} /* sdl_mouse_wheel_filter */


/*
================
sdl_mouse_motion_filter

Accumulate mouse or trackball motion and constrain Build's cursor to the active area.
Indexed frames shown through GL first rescale window coordinates to surface pixels.
================
*/
int sdl_mouse_motion_filter(SDL_Event const *event)
{
    int w, h;
#ifdef USE_OPENGL
    SDL_Event scaled;
#endif

    if (sdl_window == NULL || !sdl_input_focused)
        return(0);

#ifdef USE_OPENGL
    if ((event->type == SDL_MOUSEMOTION) && (sdl_glcontext != NULL) &&
        (!sdl_gl_drawing) && (surface != NULL))
    {
        scaled = *event;
        sdl_window_gl_mouse8(&scaled.motion);
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
        if (sdl_mouse_grabbed)
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

    sdl_window_mouse_area(&w, &h);
   	if (mouse_x < 0) mouse_x = 0;
   	if (mouse_x > w) mouse_x = w;
   	if (mouse_y < 0) mouse_y = 0;
   	if (mouse_y > h) mouse_y = h;

    return(0);
} /* sdl_mouse_motion_filter */


    /*
     * The windib driver can't alert us to the keypad enter key, which
     *  Ken's code depends on heavily. It sends it as the same key as the
     *  regular return key. These users will have to hit SHIFT-ENTER,
     *  which we check for explicitly, and give the engine a keypad enter
     *  enter event.
     */
/*
================
handle_keypad_enter_hack

Translate Shift-Return into keypad Enter for the legacy keyboard interface.
Remember the substituted press so its release uses the same key.
================
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


/*
================
sdl_key_filter

Translate positional SDL keys into Build make/break scancodes.
Handle platform shortcuts first and emit extended prefixes before their key bytes.
================
*/
int sdl_key_filter(const SDL_Event *event)
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
        sdl_mouse_grabbed = ((sdl_mouse_grabbed) ? 0 : 1);
        set_mouse_grab(sdl_mouse_grabbed);
        return(0);
    } /* if */

    else if ( ( (event->key.keysym.sym == SDLK_RETURN) ||
                (event->key.keysym.sym == SDLK_KP_ENTER) ) &&
              (event->key.state == SDL_PRESSED) &&
              (event->key.keysym.mod & KMOD_ALT) )
    {
        sdl_window_toggle_fullscreen();
        return(0);
    } /* if */

    if (!handle_keypad_enter_hack(event))
        lastkey = scancodes[sc];

    if (lastkey == 0x0000)   /* No DOS equivalent defined. */
        return(0);

    /* Extended keys are two bytes in the old keyboard interface.
     * The high byte is delivered as a separate keyhandler call. */
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


/*
================
_joystick_init

Open the joystick selected by name, or the first device by default.
The environment can disable detection; a repeated call closes the old device first.
================
*/
void _joystick_init(void)
{
    const char *envr = getenv(BUILD_SDLJOYSTICK);
    int favored = 0;
    int numsticks;
    int i;

    if (joystick != NULL)
    {
        sdl_debug("Joystick appears to be already initialized.");
        sdl_debug("...deinitializing for stick redetection...");
        _joystick_deinit();
    } /* if */

    if ((envr != NULL) && (strcmp(envr, "none") == 0))
    {
        sdl_debug("Skipping joystick detection/initialization at user request");
        return;
    } /* if */

    sdl_debug("Initializing SDL joystick subsystem...");
    sdl_debug(" (export environment variable BUILD_SDLJOYSTICK=none to skip)");

    if (SDL_Init(SDL_INIT_JOYSTICK) != 0)
    {
        sdl_debug("SDL_Init(SDL_INIT_JOYSTICK) failed: [%s].", SDL_GetError());
        return;
    } /* if */

    numsticks = SDL_NumJoysticks();
    sdl_debug("SDL sees %d joystick%s.", numsticks, numsticks == 1 ? "" : "s");
    if (numsticks == 0)
        return;

    for (i = 0; i < numsticks; i++)
    {
        const char *stickname = SDL_JoystickNameForIndex(i);
        if (stickname == NULL)
            stickname = "(unnamed)";
        if ((envr != NULL) && (strcmp(envr, stickname) == 0))
            favored = i;

        sdl_debug("Stick #%d: [%s]", i, stickname);
    } /* for */

    sdl_debug("Using Stick #%d.", favored);
    if ((envr == NULL) && (numsticks > 1))
        sdl_debug("Set BUILD_SDLJOYSTICK to one of the above names to change.");

    joystick = SDL_JoystickOpen(favored);
    if (joystick == NULL)
    {
        sdl_debug("Joystick #%d failed to init: %s", favored, SDL_GetError());
        return;
    } /* if */

    sdl_debug("Joystick initialized. %d axes, %d buttons, %d hats, %d balls.",
              SDL_JoystickNumAxes(joystick), SDL_JoystickNumButtons(joystick),
              SDL_JoystickNumHats(joystick), SDL_JoystickNumBalls(joystick));

    SDL_JoystickEventState(SDL_QUERY);
} /* _joystick_init */


/*
================
_joystick_deinit

Close the open joystick and release its SDL subsystem.
Clear the pointer so later polling follows the disconnected paths.
================
*/
void _joystick_deinit(void)
{
    if (joystick != NULL)
    {
        sdl_debug("Closing joystick device...");
        SDL_JoystickClose(joystick);
        sdl_debug("Joystick device closed. Deinitializing SDL subsystem...");
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
        sdl_debug("SDL joystick subsystem deinitialized.");
        joystick = NULL;
    } /* if */
} /* _joystick_deinit */


/*
================
_joystick_update

Refresh SDL's joystick state, reopening a device when needed.
Return zero if no attached device is available.
================
*/
int _joystick_update(void)
{
    if (joystick == NULL && SDL_NumJoysticks() > 0) _joystick_init();
    if (joystick == NULL) return 0;
    if (!SDL_JoystickGetAttached(joystick)) {
        _joystick_deinit();
        return 0;
    }

    SDL_JoystickUpdate();
    return(1);
} /* _joystick_update */


/*
================
_joystick_axis

Read an unscaled signed SDL axis value, or zero with no open joystick.
================
*/
int _joystick_axis(int axis)
{
    if (joystick == NULL)
        return(0);

    return(SDL_JoystickGetAxis(joystick, axis));
} /* _joystick_axis */


/*
================
_joystick_hat

Read SDL's hat-direction bits; return -1 when no joystick is open.
A connected hat centered at rest is represented by SDL's zero value.
================
*/
int _joystick_hat(int hat)
{
    if (joystick == NULL)
    {
        return(-1);
    }

    return(SDL_JoystickGetHat(joystick, hat));
}

/*
================
_joystick_button

Read one joystick button as a pressed/not-pressed value.
A missing joystick reports not pressed.
================
*/
int _joystick_button(int button)
{
    if (joystick == NULL)
        return(0);

    return(SDL_JoystickGetButton(joystick, button) != 0);
} /* _joystick_button */


/*
================
_readlastkeyhit

Peek at the last byte delivered through the Build keyboard handler.
Reading it does not consume or clear the stored byte.
================
*/
unsigned char _readlastkeyhit(void)
{
    return(lastkey);
} /* _readlastkeyhit */


/*
================
setupmouse

Apply capture/cursor policy and reset Build's motion origin to the view center.
Return zero until a window exists; this does not warp the SDL cursor.
================
*/
int setupmouse(void)
{
    int w, h;

    if (sdl_window == NULL)
        return(0);

    set_mouse_grab(sdl_mouse_grabbed);
    SDL_ShowCursor(0);

    sdl_window_mouse_area(&w, &h);
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


/*
================
readmousexy

Consume accumulated relative motion in Build's four-units-per-pixel interface.
Consume both axes through readmousexy32, then clamp to the legacy short range.
================
*/
void readmousexy(short *x, short *y)
{
    int32_t dx, dy;
    readmousexy32(&dx, &dy);
    if (x) *x = (short)min(max(dx, -32768), 32767);
    if (y) *y = (short)min(max(dy, -32768), 32767);
} /* readmousexy */


/*
================
readmousebstatus

Return held button bits together with any latched wheel directions.
Clear only the wheel pulses after reading; held buttons remain set.
================
*/
void readmousebstatus(short *bstatus)
{
    if (bstatus)
        *bstatus = mouse_buttons;

    // special wheel treatment
    if(mouse_buttons&8) mouse_buttons ^= 8;
    if(mouse_buttons&16) mouse_buttons ^= 16;

} /* readmousebstatus */


void initkeys(void)
{
    /* does nothing in SDL. Key input handling is set up elsewhere. */
    /* !!! why not here? */
}


void uninitkeys(void)
{
    /* does nothing in SDL. Key input handling is set up elsewhere. */
}

/* Build the positional SDL-to-Build scancode map before pumping events. */
void sdl_input_init(void)
{
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

}
