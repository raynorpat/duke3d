#include "profile_test_support.h"
/* Legacy joystick axes and buttons use the same frame sampler as gamepads. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"

#if SDL_VERSION_ATLEAST(2, 0, 14)
static int failures;
static void check(const char *name, int ok)
{
    if (!ok) { printf("FAIL %s\n", name); failures++; }
}
int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,"1.5")) return 1;
    game_init_release_defaults();
    SDL_Joystick *stick;
    int device;
    ControlInfo a, b;
    _platform_init(argc, argv, "joystick_polling", "joystick_polling");
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_FLIGHT_STICK, 2, 2, 1);
    if (device < 0) return 1;
    stick = SDL_JoystickOpen(device);
    SDL_setenv("BUILD_SDLJOYSTICK", SDL_JoystickName(stick), 1);
    CONFIG_SetDefaults(); CONTROL_ClearAssignments();
    CONTROL_JoystickEnabled = 1;
    CONTROL_MapAnalogAxis(0, analog_turning);
    CONTROL_SetAnalogAxisScale(0, 1.0f);
    CONTROL_SetAnalogAxisDeadzone(0, 8000);
    CONTROL_MapJoyButton(gamefunc_Jump, 0, false);
    CONTROL_MapJoyHat(gamefunc_Open, 0, 0);
    CONTROL_Startup(controltype_keyboardandjoystick, NULL, TICRATE);
    SDL_JoystickSetVirtualAxis(stick, 0, 16000);
    SDL_JoystickSetVirtualButton(stick, 0, 1);
    SDL_JoystickSetVirtualHat(stick, 0, SDL_HAT_UP);
    _handle_events();
    check("legacy joystick is not handled as gamepad", !GAMEPAD_Connected());
    check("legacy button sampled at frame rate", BUTTON(gamefunc_Jump));
    check("legacy hat sampled at frame rate", BUTTON(gamefunc_Open));
    SDL_JoystickSetVirtualButton(stick, 0, 0); _handle_events();
    check("short legacy press survives until tick", BUTTON(gamefunc_Jump));
    CONTROL_GetInput(&a); CONTROL_GetInput(&b);
    check("legacy axis snapshot survives consumption", a.dyaw == 16000 && b.dyaw == a.dyaw);
    CONTROL_EndInput();
    check("legacy press consumed", !BUTTON(gamefunc_Jump));
    SDL_JoystickSetVirtualButton(stick, 0, 1); SDL_JoystickUpdate();
    SDL_JoystickSetVirtualButton(stick, 0, 0); SDL_JoystickUpdate();
    _handle_events();
    check("batched legacy press survives event pump", BUTTON(gamefunc_Jump));
    CONTROL_EndInput();
    check("batched legacy press consumed once", !BUTTON(gamefunc_Jump));
    ps[0].gm = MODE_GAME; ps[0].newowner = -1; ps[0].horiz = 100;
    SDL_JoystickSetVirtualAxis(stick, 0, 0); CONTROL_ResetInput(); _handle_events();
    CONTROL_MapKey(gamefunc_Strafe, sc_LeftAlt, 0);
    CONTROL_UpdateKeyboardState(sc_LeftAlt, 1);
    SDL_JoystickSetVirtualAxis(stick, 0, 16000); _handle_events();
    SDL_Delay(20); _handle_events();
    CONTROL_GetInput(&a);
    check("legacy strafe modifier retains turning axis", a.dyaw == 16000);
    {
        int32 yaw, pitch;
        CONTROL_GetView(&yaw, &pitch);
        check("legacy strafe modifier does not turn camera", yaw == 0);
    }
    CONTROL_UpdateKeyboardState(sc_LeftAlt, 0); CONTROL_EndInput();
    ps[0].gm = 0; CONTROL_PollDevices();
    SDL_JoystickSetVirtualAxis(stick, 0, 1000); _handle_events();
    CONTROL_GetInput(&a); check("legacy deadzone preserved", a.dyaw == 0);
    SDL_JoystickDetachVirtual(device); _handle_events(); CONTROL_EndInput();
    CONTROL_GetInput(&a);
    check("legacy disconnect clears axis", a.dyaw == 0);
    check("legacy disconnect clears hat", !BUTTON(gamefunc_Open));
    SDL_JoystickClose(stick);
    device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_FLIGHT_STICK, 2, 2, 1);
    stick = SDL_JoystickOpen(device);
    SDL_JoystickSetVirtualAxis(stick, 0, -16000); _handle_events();
    CONTROL_GetInput(&a);
    check("legacy reconnect restores sampled axes", a.dyaw == -16000);
    SDL_JoystickDetachVirtual(device);
    CONTROL_Shutdown(); SDL_JoystickClose(stick);
    return failures != 0;
}
#else
int main(int argc, char **argv) {
    if (!test_select_profile(&argc,argv,"1.5")) return 1;
    game_init_release_defaults(); return 77; }
#endif
