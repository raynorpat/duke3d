//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------

#include "duke3d.h"
#include "control.h"
#include "mouse.h"
#include "joystick.h"
#include "inputaim.h"

//***************************************************************************
// FIXME  These will need to be removed once the buildengine directory
//        structure is figured out and we move to unified SDL codebase
void _joystick_init(void);
void _joystick_deinit(void);
int _joystick_update(void);
int _joystick_axis(int axis);
int _joystick_button(int button);
int _joystick_hat(int hat);
int _joystick_instance(void);
//#include "build/display.h"
//***************************************************************************

//***************************************************************************
//
// GLOBALS
//
//***************************************************************************

boolean  CONTROL_RudderEnabled;
boolean  CONTROL_MousePresent;
boolean  CONTROL_JoysPresent[ MaxJoys ];
boolean  CONTROL_MouseEnabled;
boolean  CONTROL_JoystickEnabled;
byte     CONTROL_JoystickPort;
uint32   CONTROL_ButtonState1;
uint32   CONTROL_ButtonHeldState1;
uint32   CONTROL_ButtonState2;
uint32   CONTROL_ButtonHeldState2;

uint32   CONTROL_JoyHatState[MAXJOYHATS];

static short mouseButtons = 0;
static short mouse_wheel_buttons;

static short joyHats[MAXJOYHATS];


//***************************************************************************
//
// FUNCTIONS
//
//***************************************************************************

struct _KeyMapping
{
	boolean key_active;
	kb_scancode key1;
	kb_scancode key2;
	
	/* other mappings go here */
} KeyMapping[MAXGAMEBUTTONS];

static int32 MouseMapping[MAXMOUSEBUTTONS];

// Joystick/Gamepad bindings
static int32 JoyAxisMapping[MAXJOYAXES];
static int32 JoyHatMapping[MAXJOYHATS][8];
static int32 JoyButtonMapping[MAXJOYBUTTONS];
static float JoyAnalogScale[MAXJOYAXES];
static int32 JoyAnalogDeadzone[MAXJOYAXES];

static int32 mousePositionX, mousePositionY;
static int32 mouseRelativeX, mouseRelativeY;

static InputAim local_aim, submitted_aim;
static struct { int32 sequence; InputAim delta; int base_turn, base_look; } look_fifo[MOVEFIFOSIZ];
static InputAim packet_look;
static int packet_base_turn, packet_base_look;
static int immediate_look, was_active = -1, previous_aimmode = -1;
static Uint64 last_poll;
static int32_t turn_rate, pitch_rate;

int CONTROL_ImmediateLook(void) { return immediate_look; }

/*
================
turnscale

Return the release's nominal command-to-angle scale used for look rates.
================
*/
static int turnscale(void)
{
if (game_profile->gamever < GAMEVER_11) {
    return TICSPERFRAME;
} else {
if (game_profile->gamever < GAMEVER_13D) {
    return 1;
} else {
    return 2;
}
}
}

/*
================
turn_delta

Evaluate this release's actual turn transform, including underwater scaling.
Quantization must use the same result as player simulation.
================
*/
static int turn_delta(int turn)
{
if (game_profile->gamever < GAMEVER_11) {
    return (turn + (turn >> 3)) * TICSPERFRAME;
} else {
if (game_profile->gamever < GAMEVER_13D) {
    return turn;
} else {
    int delta = turn * 2;
    int sect = ps[myconnectindex].cursectnum;
    if (sect >= 0 && sect < numsectors && sector[sect].lotag == 2)
        delta -= delta >> 3;
    return delta;
}
}
}

/*
================
CONTROL_GetView

Return the combined Q16 look held locally and awaiting FIFO acknowledgement.
================
*/
void CONTROL_GetView(int32 *yaw, int32 *horizon)
{
    *yaw = (int32)(local_aim.yaw + submitted_aim.yaw);
    *horizon = (int32)(local_aim.horizon + submitted_aim.horizon);
}

/*
================
CONTROL_ConsumeLook

Fit accumulated look into the available command range alongside other input.
Retain the unrepresented remainder and track the submitted FIFO contribution.
================
*/
void CONTROL_ConsumeLook(int32 *turn, int32 *look, int maxturn, int maxlook)
{
    int slot = movefifoend[myconnectindex] & (MOVEFIFOSIZ - 1);
    int base_turn = min(max(*turn, -maxturn), maxturn);
    int base_look = min(max(*look, -maxlook), maxlook);
    int32 t, h;
    int delta, step = local_aim.yaw >= 0 ? 1 : -1;
    int64_t desired = local_aim.yaw;
    INPUT_ConsumeLook(&local_aim, turnscale(), &t, &h);
    /* Quantize against the release's actual turn transform, without overshoot. */
    *turn = base_turn;
    while (*turn + step >= -maxturn && *turn + step <= maxturn) {
        int64_t next = (int64_t)(turn_delta(*turn + step) - turn_delta(base_turn)) * 65536;
        if ((step > 0 && next > desired) || (step < 0 && next < desired)) break;
        *turn += step;
    }
    *look = min(max(base_look + h, -maxlook), maxlook);
    /* The serialized horizon command represents pairs of input units.
     * Return any clipped or odd remainder to the Q16 accumulator. */
    *look -= (*look - base_look) % 2;
    local_aim.horizon += (int64_t)(h - (*look - base_look)) * 32768;
    delta = turn_delta(*turn) - turn_delta(base_turn);
    local_aim.yaw = desired - (int64_t)delta * 65536;
    look_fifo[slot].base_turn = base_turn;
    look_fifo[slot].base_look = base_look;
    look_fifo[slot].sequence = movefifoend[myconnectindex];
    look_fifo[slot].delta.yaw = (int64_t)delta * 65536;
    look_fifo[slot].delta.horizon = (int64_t)(*look - base_look) * 32768;
    /* Move represented motion into the outstanding-command total so it stays
     * visible until the corresponding FIFO entry has been acknowledged. */
    submitted_aim.yaw += look_fifo[slot].delta.yaw;
    submitted_aim.horizon += look_fifo[slot].delta.horizon;
}

/* Record the actual averaged/copied FIFO entry, rather than raw getinput. */
/*
================
CONTROL_QueueInput

Reconcile local look with the command actually copied or averaged into the FIFO.
Refund motion lost to packet quantization instead of dropping its remainder.
================
*/
void CONTROL_QueueInput(int32 sequence, int turn, int look)
{
    int slot = sequence & (MOVEFIFOSIZ - 1);
    int previous = (sequence - 1) & (MOVEFIFOSIZ - 1);
    InputAim represented, refund;
    if (look_fifo[slot].sequence != sequence) {
        look_fifo[slot].base_turn = turn;
        look_fifo[slot].base_look = look;
        look_fifo[slot].delta.yaw = look_fifo[slot].delta.horizon = 0;
    }
    packet_base_turn += look_fifo[slot].base_turn;
    packet_base_look += look_fifo[slot].base_look;
    packet_look.yaw += look_fifo[slot].delta.yaw;
    packet_look.horizon += look_fifo[slot].delta.horizon;
    if (sequence & (movesperpacket - 1)) {
        look_fifo[slot].base_turn = look_fifo[previous].base_turn;
        look_fifo[slot].base_look = look_fifo[previous].base_look;
    } else {
        look_fifo[slot].base_turn = packet_base_turn / movesperpacket;
        look_fifo[slot].base_look = packet_base_look / movesperpacket;
    }
    represented.yaw = (int64_t)(turn_delta(turn) - turn_delta(look_fifo[slot].base_turn)) * 65536;
    represented.horizon = (int64_t)((look >> 1) - (look_fifo[slot].base_look >> 1)) * 65536;
    if (!(sequence & (movesperpacket - 1))) {
        /* One averaged packet stands for movesperpacket ticks. Preserve the
         * difference between accumulated look and that repeated representation. */
        refund.yaw = packet_look.yaw - represented.yaw * movesperpacket;
        refund.horizon = packet_look.horizon - represented.horizon * movesperpacket;
        local_aim.yaw += refund.yaw; local_aim.horizon += refund.horizon;
        submitted_aim.yaw -= refund.yaw; submitted_aim.horizon -= refund.horizon;
        packet_look.yaw = packet_look.horizon = 0;
        packet_base_turn = packet_base_look = 0;
    }
    look_fifo[slot].sequence = sequence;
    look_fifo[slot].delta = represented;
}

/*
================
CONTROL_AcknowledgeInput

Retire the look contribution for this exact FIFO sequence after consumption.
Ignore stale or already acknowledged ring slots.
================
*/
void CONTROL_AcknowledgeInput(int32 sequence)
{
    int slot = sequence & (MOVEFIFOSIZ - 1);
    if (look_fifo[slot].sequence != sequence) return;
    submitted_aim.yaw -= look_fifo[slot].delta.yaw;
    submitted_aim.horizon -= look_fifo[slot].delta.horizon;
    look_fifo[slot].sequence = -1;
}

static uint64_t keyboard_buttons, mouse_buttons_held, joy_buttons, pressed_buttons;
static uint64_t previous_buttons, suppressed_buttons;
static unsigned char key_down[256];
static ControlInfo mouse_pending, joystick_sample;

static uint64_t actionbit(int action)
{
    return action >= 0 && action < MAXGAMEBUTTONS ? UINT64_C(1) << action : 0;
}

/*
================
publish_buttons

Combine device-held actions with latched presses, excluding suppressed actions.
Keep suppression only while an action remains physically held.
================
*/
static void publish_buttons(void)
{
    uint64_t held = keyboard_buttons | mouse_buttons_held | joy_buttons |
        GAMEPAD_ButtonState1 | ((uint64_t)GAMEPAD_ButtonState2 << 32);
    suppressed_buttons &= held;
    /* Keep down/up taps visible until a command consumes them, even when
     * no device is still held at the end of the frame. */
    pressed_buttons |= held & ~previous_buttons & ~suppressed_buttons;
    previous_buttons = held;
    held = (held | pressed_buttons) & ~suppressed_buttons;
    CONTROL_ButtonState1 = (uint32_t)held;
    CONTROL_ButtonState2 = (uint32_t)(held >> 32);
}

/*
================
CONTROL_UpdateKeyboardState

Rebuild held keyboard actions from the key map after a scancode transition.
================
*/
void CONTROL_UpdateKeyboardState(int key, int pressed)
{
    int i;
    if (key < 0 || key >= 256) return;
    key_down[key] = pressed != 0;
    keyboard_buttons = 0;
    for (i = 0; i < MAXGAMEBUTTONS; i++)
        if (KeyMapping[i].key_active &&
            ((KeyMapping[i].key1 && key_down[KeyMapping[i].key1]) ||
             (KeyMapping[i].key2 && key_down[KeyMapping[i].key2])))
            keyboard_buttons |= actionbit(i);
    publish_buttons();
}

/*
================
CONTROL_EndInput

Retire latched presses at the end of command consumption and republish held state.
================
*/
void CONTROL_EndInput(void)
{
    pressed_buttons = 0;
    publish_buttons();
}

/*
================
CONTROL_ResetInput

Clear pending device motion, fractional look, FIFO accounting and button state.
Reset the poll clock so resumed input does not integrate the paused interval.
================
*/
void CONTROL_ResetInput(void)
{
    int i;
    local_aim.yaw = local_aim.horizon = 0;
    submitted_aim.yaw = submitted_aim.horizon = 0;
    packet_look.yaw = packet_look.horizon = 0;
    packet_base_turn = packet_base_look = 0;
    memset(look_fifo, 0, sizeof(look_fifo));
    for (i = 0; i < MOVEFIFOSIZ; i++) look_fifo[i].sequence = -1;
    last_poll = 0;
    turn_rate = pitch_rate = 0;
    GAMEPAD_ResetInput();
    KB_ClearKeysDown();
    immediate_look = 0;
    mouseRelativeX = mouseRelativeY = mouse_wheel_buttons = 0;
    keyboard_buttons = mouse_buttons_held = joy_buttons = pressed_buttons = 0;
    previous_buttons = suppressed_buttons = 0;
    memset(key_down, 0, sizeof(key_down));
    memset(&mouse_pending, 0, sizeof(mouse_pending));
    memset(&joystick_sample, 0, sizeof(joystick_sample));
    clearmouseinput();
    mouseButtons = 0;
    publish_buttons();
}

/*
================
CONTROL_GetWheelDirection

Consume one ordered wheel step and map weapon cycling to -1 or +1.
Other mapped actions are latched as presses and return zero.
================
*/
int CONTROL_GetWheelDirection(void)
{
    int direction = readmousewheel();
    int action = MouseMapping[direction > 0 ? 3 : 4];
    if (direction == 0) return 0;
    if (action == gamefunc_Previous_Weapon) return -1;
    if (action == gamefunc_Next_Weapon) return 1;
    pressed_buttons |= actionbit(action);
    publish_buttons();
    return 0;
}

// added.  --ryan.
const char *CONTROL_GetMappingName(int32 which)
{
	if (!KeyMapping[which].key_active)
        return NULL;

	return(KB_ScanCodeToString(KeyMapping[which].key1));
}


void CONTROL_MapKey( int32 which, kb_scancode key1, kb_scancode key2 )
{
	KeyMapping[which].key_active = true;
	KeyMapping[which].key1 = key1;
	KeyMapping[which].key2 = key2;
}

void CONTROL_MapButton
        (
        int32 whichfunction,
        int32 whichbutton,
        boolean doubleclicked
        )
{
    if(doubleclicked)
	return; // TODO

    if(whichbutton < 0 || whichbutton >= MAXMOUSEBUTTONS)
	    return;

    MouseMapping[whichbutton] = whichfunction;
}

void CONTROL_MapJoyButton(int32 whichfunction, int32 whichbutton, boolean doubleclicked)
{
    if(whichbutton < 0 || whichbutton >= MAXJOYBUTTONS)
    {
        return;
    }

    if(doubleclicked)
       return; // TODO

    JoyButtonMapping[whichbutton] = whichfunction;
}

void CONTROL_MapJoyHat(int32 whichfunction, int32 whichhat, int32 whichvalue)
{
    if(whichhat < 0 || whichhat >= MAXJOYHATS)
    {
        return;
    }

    JoyHatMapping[whichhat][whichvalue] = whichfunction;
}

void CONTROL_DefineFlag( int32 which, boolean toggle )
{
	// STUBBED("CONTROL_DefineFlag");
}

boolean CONTROL_FlagActive( int32 which )
{
	STUBBED("CONTROL_FlagActive");
	return false;
}

/*
================
CONTROL_ClearAssignments

Clear device mappings and discard pending input associated with the old map.
================
*/
void CONTROL_ClearAssignments( void )
{
    memset(KeyMapping, 0, sizeof(KeyMapping));
    memset(MouseMapping, -1, sizeof(MouseMapping));
    memset(JoyButtonMapping, -1, sizeof(JoyButtonMapping));
    memset(JoyHatMapping, -1, sizeof(JoyHatMapping));
    memset(JoyAxisMapping, -1, sizeof(JoyAxisMapping));
    CONTROL_ResetInput();
}

void CONTROL_GetUserInput( UserInput *info )
{
	STUBBED("CONTROL_GetUserInput");
}

/*
================
prepare_input

Select immediate-look eligibility and reset accumulated input on context
or aiming-mode transitions before accepting fresh events.
================
*/
static int prepare_input(void)
{
    int active;
    active = (ps[myconnectindex].gm & MODE_GAME) &&
        !(ps[myconnectindex].gm & (MODE_MENU | MODE_TYPE | MODE_EOL | MODE_RESTART)) &&
        !ud.pause_on && !ud.show_help && !(ud.scrollmode && ud.overhead_on) && ud.recstat != 2 &&
        ps[myconnectindex].newowner < 0;
    if ((was_active >= 0 && active != was_active) ||
        (previous_aimmode >= 0 && myaimmode != previous_aimmode)) CONTROL_ResetInput();
    previous_aimmode = myaimmode;
    was_active = active;
    immediate_look = active;
    return active;
}

/*
================
control_event

Latch short device presses that may begin and end between simulation ticks.
A NULL event prepares the input context before the SDL queue is drained.
================
*/
static void control_event(const SDL_Event *event)
{
    int action = -1, button, j;
    if (event == NULL) { prepare_input(); return; }
    action = GAMEPAD_EventAction(event);
    if (event->type == SDL_MOUSEBUTTONDOWN) {
        button = event->button.button;
        if (button == SDL_BUTTON_LEFT) action = MouseMapping[0];
        else if (button == SDL_BUTTON_RIGHT) action = MouseMapping[1];
        else if (button == SDL_BUTTON_MIDDLE) action = MouseMapping[2];
    } else if (CONTROL_JoystickEnabled && !GAMEPAD_Connected()) {
        if (event->type == SDL_JOYBUTTONDOWN && event->jbutton.which == _joystick_instance() &&
            event->jbutton.button < MAXJOYBUTTONS)
            action = JoyButtonMapping[event->jbutton.button];
        else if (event->type == SDL_JOYHATMOTION && event->jhat.which == _joystick_instance() &&
                 event->jhat.hat < MAXJOYHATS)
            for (j = 0; j < 8; j++)
                if (event->jhat.value & (1 << j))
                    pressed_buttons |= actionbit(JoyHatMapping[event->jhat.hat][j]);
    }
    pressed_buttons |= actionbit(action) & ~suppressed_buttons;
    publish_buttons();
}

/*
================
CONTROL_PollDevices

Accumulate mouse deltas and sample controller/joystick state at frame cadence.
Integrate the prior look rate over elapsed time before installing the new sample.
================
*/
void CONTROL_PollDevices(void)
{
    ControlInfo sample, *info = &sample;
    int32 sens = CONTROL_GetMouseSensitivity() >> 9;
    int32 mx, my;
    int i, j, active;
    int32 padturn, padpitch;
    int basehorizon;
    Uint64 now = SDL_GetPerformanceCounter();
    int64_t elapsed;
    active = prepare_input();
    /* The first poll after a reset has zero elapsed time; later intervals
     * are measured by SDL's performance counter in microseconds. */
    elapsed = last_poll ? (now - last_poll) * 1000000 / SDL_GetPerformanceFrequency() : 0;
    last_poll = now;
    basehorizon = (numplayers > 1 ? myhoriz : ps[myconnectindex].horiz) +
        (int)(submitted_aim.horizon / 65536);
    if (active) {
        INPUT_AddRate(&local_aim,
            (int64_t)turn_rate * turnscale() * TICRATE / TICSPERFRAME,
            (int64_t)pitch_rate * TICRATE / (2 * TICSPERFRAME),
            (uint32_t)elapsed, basehorizon, getviewhorizscale());
    }
    GAMEPAD_Poll();
    memset(info, '\0', sizeof (ControlInfo));

    mx = my = 0;

    MOUSE_GetDelta(&mx,&my);

    info->dyaw = mx * sens;

       switch(ControllerType)
       {
       case controltype_keyboardandjoystick:
               {
               }
               break;
       case controltype_joystickandmouse:
               {
                       // Mouse should use pitch instead of forward movement.
                       info->dpitch = my * sens*2;
               }
               break;
       default:
               {
            // If mouse aim is active
                       if( myaimmode )
                       {  
                               info->dpitch = my * sens*2;
            }
            else
            {
    info->dz = my * sens*2;
            }
               }
               break;
       }

    /* Immediate camera motion takes the mouse look components out of the
     * legacy pending sample so tick consumption cannot apply them twice. */
    if (active) {
        int64_t yaw = 0, pitch = 0;
        if (!BUTTON(gamefunc_Strafe)) {
if (game_profile->gamever < GAMEVER_11) {
            yaw = (int64_t)mx * CONTROL_GetMouseSensitivity() * 65536 * turnscale() / (512 * 256);
} else {
            yaw = (int64_t)mx * CONTROL_GetMouseSensitivity() * 65536 * turnscale() / (512 * 64);
}
            info->dyaw = 0;
        }
        if (myaimmode || ControllerType == controltype_joystickandmouse) {
            pitch = (int64_t)my * CONTROL_GetMouseSensitivity() * 65536 / (512 * 186);
            if (ud.mouseflip) pitch = -pitch;
            info->dpitch = 0;
        }
        INPUT_AddLook(&local_aim, yaw, pitch, basehorizon, getviewhorizscale());
    }
    mouse_pending.dyaw += info->dyaw;
    mouse_pending.dpitch += info->dpitch;
    mouse_pending.dz += info->dz;
    memset(info, 0, sizeof(*info));
    mouse_buttons_held = joy_buttons = 0;
    for (i = 0; i < MAXMOUSEBUTTONS; i++)
        if (i != 3 && i != 4 && (mouseButtons & (1 << i)))
            mouse_buttons_held |= actionbit(MouseMapping[i]);

    // update stick state.
    if ((CONTROL_JoystickEnabled) && !GAMEPAD_Connected() && (_joystick_update()))
    {
               // Check the hats
               JOYSTICK_UpdateHats();

               // TODO: make this NOT use the BUTTON() system for storing the hat input. (requires much game code changing)
               for(i=0; i<MAXJOYHATS; i++)
               {

                       for(j=0; j<8; j++)
                       {
                               if (joyHats[i] & (1 << j))
                                   joy_buttons |= actionbit(JoyHatMapping[i][j]);
                       }

               }
               

        for(i=0; i<MAXJOYAXES;i++)
        {
            switch(JoyAxisMapping[i])
            {
                case analog_turning:
                    {
                     info->dyaw +=  (int32)((float)CONTROL_FilterDeadzone
                                    (
                                        _joystick_axis(i),
                                        JoyAnalogDeadzone[i]
                                    ) 
                                        * JoyAnalogScale[i]
                                    );
                    }
                   break;
                case analog_strafing:
                    {
                        info->dx += (int32)((float)CONTROL_FilterDeadzone
                                    (
                                        _joystick_axis(i), 
                                        JoyAnalogDeadzone[i]
                                    )
                                        * JoyAnalogScale[i]
                                    );
                                    //printf("Joy %d = %d\n", i, info->dx);
                    }
                   break;
                case analog_lookingupanddown:
                        info->dpitch += (int32)((float)CONTROL_FilterDeadzone
                                    (
                                        _joystick_axis(i), 
                                        JoyAnalogDeadzone[i]
                                    )
                                        * JoyAnalogScale[i]
                                    );
                   break;
                case analog_elevation: //STUB
                   break;
                case analog_rolling: //STUB
                   break;
                case analog_moving:
                    {
                        info->dz += (int32)((float)CONTROL_FilterDeadzone
                                    (
                                        _joystick_axis(i), 
                                        JoyAnalogDeadzone[i]
                                    )
                                        * JoyAnalogScale[i]
                                    );
                    }
                   break;
                default:
                    break;
            }
        }
        for(i=0; i<MAXJOYBUTTONS;++i)
        {
            if(_joystick_button(i))
            {
                joy_buttons |= actionbit(JoyButtonMapping[i]);
            }
        }
    }
    GAMEPAD_GetLook(&padturn, &padpitch);
    turn_rate = padturn;
    if (!BUTTON(gamefunc_Strafe))
if (game_profile->gamever < GAMEVER_11) {
        turn_rate += (int32_t)((int64_t)sample.dyaw * 65536 / 256);
} else {
        turn_rate += (int32_t)((int64_t)sample.dyaw * 65536 / 64);
}
    pitch_rate = padpitch + (int32_t)((int64_t)sample.dpitch * 65536 / 186 * (ud.mouseflip ? -1 : 1));
    if (active) {
        if (!BUTTON(gamefunc_Strafe)) sample.dyaw = 0;
        sample.dpitch = 0;
    }
    joystick_sample = sample;
    publish_buttons();
}

/*
================
CONTROL_GetInput

Consume pending mouse motion together with the latest joystick sample.
Sampling remains separate so multiple simulation ticks do not poll devices again.
================
*/
void CONTROL_GetInput(ControlInfo *info)
{
    *info = joystick_sample;
    info->dyaw += mouse_pending.dyaw;
    info->dpitch += mouse_pending.dpitch;
    info->dz += mouse_pending.dz;
    memset(&mouse_pending, 0, sizeof(mouse_pending));
}


/*
================
CONTROL_ClearButton

Suppress an action until release and remove its pending press.
A held device must not immediately reassert a consumed one-shot action.
================
*/
void CONTROL_ClearButton( int32 whichbutton )
{
    uint64_t bit = actionbit(whichbutton);
    suppressed_buttons |= bit;
    pressed_buttons &= ~bit;
    GAMEPAD_ClearButton(whichbutton);
    publish_buttons();
}

void CONTROL_ClearUserInput( UserInput *info )
{
	STUBBED("CONTROL_ClearUserInput");
}

void CONTROL_WaitRelease( void )
{
	STUBBED("CONTROL_WaitRelease");
}

void CONTROL_Ack( void )
{
	STUBBED("CONTROL_Ack");
}

void CONTROL_CenterJoystick
   (
   void ( *CenterCenter )( void ),
   void ( *UpperLeft )( void ),
   void ( *LowerRight )( void ),
   void ( *CenterThrottle )( void ),
   void ( *CenterRudder )( void )
   )
{
	STUBBED("CONTROL_CenterJoystick");
}

static int32 mouseSensitivity = 0;
int32 CONTROL_GetMouseSensitivity( void )
{
    return mouseSensitivity;
}

void CONTROL_SetMouseSensitivity( int32 newsensitivity )
{
	mouseSensitivity = newsensitivity;
}

void CONTROL_Startup
   (
   controltype which,
   int32 ( *TimeFunction )( void ),
   int32 ticspersecond
   )
{
    int i;

    _joystick_init();
    GAMEPAD_Init();
    CONTROL_ResetInput();
    setinputcallback(CONTROL_PollDevices, CONTROL_ResetInput, control_event);

    for (i=0; i < MAXJOYHATS; i++)
    {
	    joyHats[i] = 0;
    }
}

void CONTROL_Shutdown( void )
{
    setinputcallback(NULL, NULL, NULL);
    CONTROL_ResetInput();
    GAMEPAD_Shutdown();
    _joystick_deinit();
}


void CONTROL_MapAnalogAxis
   (
   int32 whichaxis,
   int32 whichanalog
   )
{
    if(whichaxis < MAXJOYAXES)
    {
        JoyAxisMapping[whichaxis] = whichanalog;
    }
}


void CONTROL_MapDigitalAxis
   (
   int32 whichaxis,
   int32 whichfunction,
   int32 direction
   )
{
	STUBBED("CONTROL_MapDigitalAxis");
}

void CONTROL_SetAnalogAxisScale
   (
   int32 whichaxis,
   float axisscale
   )
{
    if(whichaxis < MAXJOYAXES)
    {
        // Set it... make sure we don't let them set it to 0.. div by 0 is bad.
        JoyAnalogScale[whichaxis] = (axisscale == 0) ? 1.0f : axisscale;
    }
}

void CONTROL_SetAnalogAxisDeadzone
   (
   int32 whichaxis,
   int32 axisdeadzone
   )
{
    if(whichaxis < MAXJOYAXES)
    {
        // Set it... 
        JoyAnalogDeadzone[whichaxis] = axisdeadzone;
    }
}

/*
================
CONTROL_FilterDeadzone

Return zero strictly inside the symmetric deadzone.
Values outside it retain their original magnitude.
================
*/
int32 CONTROL_FilterDeadzone
   (
   int32 axisvalue,
   int32 axisdeadzone
   )
{
    if((axisvalue < axisdeadzone) && (axisvalue > -axisdeadzone))
    {
        return 0;
    }

    return axisvalue;
}

/*
================
CONTROL_GetFilteredAxisValue

Apply the configured deadzone and scale to one legacy joystick axis.
================
*/
int32 CONTROL_GetFilteredAxisValue(int32 axis)
{
return (int32)((float)CONTROL_FilterDeadzone
                                    (
                                        _joystick_axis(axis), 
                                        JoyAnalogDeadzone[axis]
                                    )
                                        * JoyAnalogScale[axis]
                                    );
}

void CONTROL_PrintAxes( void )
{
	STUBBED("CONTROL_PrintAxes");
}

boolean MOUSE_Init( void )
{
	memset(MouseMapping,-1,sizeof(MouseMapping));
	return true;
}

void    MOUSE_Shutdown( void )
{
	STUBBED("MOUSE_Shutdown");
}

void    MOUSE_ShowCursor( void )
{
	STUBBED("MOUSE_ShowCursor");
}

void    MOUSE_HideCursor( void )
{
	STUBBED("MOUSE_HideCursor");
}



/*
================
updateMouse

Collect full-width Build mouse deltas and retain wheel pulses for button readers.
================
*/
static void updateMouse(void)
{
    // this is in buildengine.
    int32 x, y;
    getmousevalues32(&x, &y, &mouseButtons);
    mouse_wheel_buttons |= mouseButtons & 24;
    mouseButtons |= mouse_wheel_buttons;

    mouseRelativeX += x;
    mouseRelativeY += y;
    mousePositionX += x;
    mousePositionY += y;
}

/*
================
MOUSE_GetButtons

Return the current mouse buttons and consume the legacy wheel direction bits.
================
*/
int32   MOUSE_GetButtons( void )
{
    int32 buttons;
    updateMouse();
    buttons = mouseButtons;
    mouse_wheel_buttons = 0;
    mouseButtons &= ~24;
    return buttons;
}

void    MOUSE_GetPosition( int32*x, int32*y  )
{
    if (x) *x = mousePositionX;
    if (y) *y = mousePositionY;
}

/*
================
MOUSE_GetDelta

Collect and consume both accumulated relative mouse axes.
================
*/
void    MOUSE_GetDelta( int32*x, int32*y  )
{
    updateMouse();

    if (x) *x = mouseRelativeX;
    if (y) *y = mouseRelativeY;

    mouseRelativeX = mouseRelativeY = 0;
}

void JOYSTICK_UpdateHats()
{
       int i;

       for(i=0; i<MAXJOYHATS; i++)
       {
               joyHats[i] = _joystick_hat(i);
       }
}

