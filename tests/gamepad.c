#include "profile_test_support.h"
/* Exercises the actual SDL GameController path with a virtual device. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"

#if SDL_VERSION_ATLEAST(2, 0, 14)
static int failures;

static void check(const char *what, int got, int want)
{
    if (got != want) {
        printf("FAIL %s: got %d, want %d\n", what, got, want);
        failures++;
    }
}

static void pollpad(void)
{
    ControlInfo info;
    SDL_PumpEvents();
    CONTROL_PollDevices();
    CONTROL_GetInput(&info);
    CONTROL_EndInput();
}

int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    game_init_release_defaults();
    SDL_Joystick *pad;
    int device, action = gamefunc_Jump;
    char guid[33], mapping[1024];

    _platform_init(argc, argv, "gamepad", "gamepad");
    SDL_setenv("BUILD_SDLJOYSTICK", "Test Gamepad", 1);
    KB_Startup();
    CONFIG_SetDefaults();
    CONTROL_Startup(ControllerType, NULL, TICRATE);
    /* Detection must also work when the device is attached after startup. */
    device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 15, 0);
    if (device < 0) {
        printf("virtual gamepad: %s\n", SDL_GetError());
        return 1;
    }
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(device), guid, sizeof(guid));
    snprintf(mapping, sizeof(mapping),
        "%s,Test Gamepad,a:b0,b:b1,x:b2,y:b3,back:b4,start:b6,"
        "leftstick:b7,rightstick:b8,leftshoulder:b9,rightshoulder:b10,"
        "dpup:b11,dpdown:b12,dpleft:b13,dpright:b14,"
        "leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,", guid);
    SDL_GameControllerAddMapping(mapping);
    pad = SDL_JoystickOpen(device);
    if (pad == NULL)
        return 1;
    SDL_JoystickSetVirtualAxis(pad, 4, -32768);
    SDL_JoystickSetVirtualAxis(pad, 5, -32768);
    ps[0].gm = MODE_GAME;
    pollpad();
    check("resting pad does not jump", BUTTON(action), 0);
    SDL_JoystickSetVirtualButton(pad, 0, 1);
    pollpad();
    check("A jumps without changing controller type", BUTTON(action), 1);
    CONTROL_ClearButton(action);
    pollpad();
    check("cleared action stays cleared while held", BUTTON(action), 0);
    SDL_JoystickSetVirtualButton(pad, 0, 0);
    pollpad();
    SDL_JoystickSetVirtualButton(pad, 0, 1);
    pollpad();
    check("released action can be pressed again", BUTTON(action), 1);

    CONTROL_MapKey(action, sc_Space, 0);
    CONTROL_UpdateKeyboardState(sc_Space, 1);
    SDL_JoystickSetVirtualButton(pad, 0, 0);
    pollpad();
    check("releasing pad does not release held keyboard action", BUTTON(action), 1);
    CONTROL_UpdateKeyboardState(sc_Space, 0);
    check("keyboard release clears action", BUTTON(action), 0);

    SDL_JoystickSetVirtualAxis(pad, 5, 32767);
    pollpad();
    action = gamefunc_Fire;
    check("right trigger fires", BUTTON(action), 1);
    SDL_JoystickSetVirtualAxis(pad, 5, -32768);
    pollpad();
    check("right trigger releases", BUTTON(action), 0);
    {
        static const struct { int button, action; } bindings[] = {
            { 1, gamefunc_Crouch }, { 2, gamefunc_Open },
            { 3, gamefunc_Inventory }, { 4, gamefunc_Map },
            { 7, gamefunc_Run }, { 8, gamefunc_Crouch },
            { 9, gamefunc_Previous_Weapon }, { 10, gamefunc_Next_Weapon },
            { 11, gamefunc_MedKit }, { 12, gamefunc_Jetpack },
            { 13, gamefunc_Inventory_Left }, { 14, gamefunc_Inventory_Right }
        };
        unsigned i;
        for (i = 0; i < sizeof(bindings)/sizeof(bindings[0]); i++) {
            action = bindings[i].action;
            SDL_JoystickSetVirtualButton(pad, bindings[i].button, 1);
            pollpad();
            check("controller button activates action", BUTTON(action), 1);
            SDL_JoystickSetVirtualButton(pad, bindings[i].button, 0);
            pollpad();
            check("controller button releases action", BUTTON(action), 0);
        }
    }

    initengine();
    /* The input transform needs only these cardinal values from tables.dat. */
    sintable[0] = 0;
    sintable[512] = 16384;
    sintable[1536] = -16384;
    SDL_JoystickSetVirtualAxis(pad, 0, 1000);
    SDL_JoystickSetVirtualAxis(pad, 1, -1000);
    SDL_JoystickSetVirtualAxis(pad, 2, 1000);
    SDL_JoystickSetVirtualAxis(pad, 3, 1000);
    CONTROL_PollDevices();
    getinput(0);
    check("stick drift does not move", loc.fvel != 0 || loc.svel != 0, 0);
    check("stick drift does not look", loc.avel != 0 || loc.horz != 0, 0);
    SDL_JoystickSetVirtualAxis(pad, 0, 32767);
    SDL_JoystickSetVirtualAxis(pad, 1, -32768);
    SDL_JoystickSetVirtualAxis(pad, 2, 32767);
    SDL_JoystickSetVirtualAxis(pad, 3, 32767);
    SDL_PumpEvents();
    CONTROL_PollDevices();
    getinput(0);
    check("left stick moves forward", loc.fvel > 0, 1);
    check("left stick strafes right", loc.svel > 0, 1);
    check("right stick turns right", loc.avel > 0, 1);
    check("right stick looks down", loc.horz < 0, 1);
    /* A saved mouse-look-off preference must not disable the right stick. */
    myaimmode = 0;
    CONTROL_PollDevices();
    getinput(0);
if (game_profile->gamever >= GAMEVER_13D) {
    check("gamepad sends aim mode with mouse look off", (loc.bits >> 23) & 1, 1);
}

    SDL_JoystickSetVirtualAxis(pad, 0, 0);
    SDL_JoystickSetVirtualAxis(pad, 1, 0);
    SDL_JoystickSetVirtualAxis(pad, 2, 0);
    SDL_JoystickSetVirtualAxis(pad, 3, 0);
    SDL_JoystickSetVirtualButton(pad, 6, 1);
    pollpad();
    check("Start opens menu", KB_KeyPressed(sc_Escape), 1);
    KB_ClearKeysDown();
    ps[0].gm = MODE_MENU;
    pollpad();
    check("held Start does not immediately close menu", KB_KeyPressed(sc_Escape), 0);
    SDL_JoystickSetVirtualButton(pad, 6, 0);
    SDL_JoystickSetVirtualButton(pad, 12, 1);
    pollpad();
    check("D-pad down navigates menu", KB_KeyPressed(sc_DownArrow), 1);
    KB_ClearKeyDown(sc_DownArrow);
    pollpad();
    check("held direction waits before repeating", KB_KeyPressed(sc_DownArrow), 0);
    SDL_Delay(360);
    pollpad();
    check("held direction repeats in menu", KB_KeyPressed(sc_DownArrow), 1);
    SDL_JoystickSetVirtualButton(pad, 12, 0);
    KB_KeyDown[sc_DownArrow] = 1;
    SDL_JoystickSetVirtualButton(pad, 0, 1);
    pollpad();
    check("pad release preserves physical menu key", KB_KeyPressed(sc_DownArrow), 1);
    KB_KeyDown[sc_DownArrow] = 0;
    check("unconsumed direction clears on pad release", KB_KeyPressed(sc_DownArrow), 0);
    check("A confirms menu", KB_KeyPressed(sc_Enter), 1);
    KB_ClearKeyDown(sc_Enter);
    pollpad();
    check("held A does not repeatedly confirm", KB_KeyPressed(sc_Enter), 0);
    action = gamefunc_Jump;
    check("menu A does not jump", BUTTON(action), 0);
    ps[0].gm = MODE_GAME;
    pollpad();
    check("held menu confirm does not jump on resume", BUTTON(action), 0);
    SDL_JoystickSetVirtualButton(pad, 0, 0);
    pollpad();
    SDL_JoystickSetVirtualButton(pad, 0, 1);
    pollpad();
    check("jump works after menu confirm releases", BUTTON(action), 1);

    SDL_JoystickDetachVirtual(device);
    pollpad();
    check("disconnect releases controller action", BUTTON(action), 0);
    CONTROL_PollDevices();
    getinput(0);
    check("disconnect does not leave movement", loc.fvel != 0 || loc.svel != 0, 0);
if (game_profile->gamever >= GAMEVER_13D) {
    check("disconnect restores mouse aim preference", (loc.bits >> 23) & 1, 0);
}
    SDL_JoystickClose(pad);
    device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 15, 0);
    pad = device >= 0 ? SDL_JoystickOpen(device) : NULL;
    if (pad == NULL)
        return 1;
    SDL_JoystickSetVirtualButton(pad, 0, 1);
    pollpad();
    check("reconnected controller works", BUTTON(action), 1);
    ps[0].newowner = -1; ps[0].horiz = 100;
    CONTROL_ResetInput(); _handle_events();
    SDL_JoystickSetVirtualButton(pad, 0, 0); _handle_events();
    SDL_JoystickSetVirtualButton(pad, 0, 1); _handle_events();
    SDL_JoystickSetVirtualButton(pad, 0, 0); _handle_events();
    check("short gamepad press survives frame polling", BUTTON(action), 1);
    getinput(0);
    check("tick sees short gamepad press", loc.bits & 1, 1);
    check("short gamepad press consumed once", BUTTON(action), 0);
    SDL_JoystickSetVirtualButton(pad, 0, 1); SDL_JoystickUpdate();
    SDL_JoystickSetVirtualButton(pad, 0, 0); SDL_JoystickUpdate();
    _handle_events();
    check("batched gamepad press survives event pump", BUTTON(action), 1);
    getinput(0);
    check("batched gamepad press consumed once", BUTTON(action), 0);
    SDL_JoystickSetVirtualAxis(pad, 5, 32767); SDL_JoystickUpdate();
    SDL_JoystickSetVirtualAxis(pad, 5, -32768); SDL_JoystickUpdate();
    _handle_events();
    action = gamefunc_Fire;
    check("batched trigger press survives event pump", BUTTON(action), 1);
    getinput(0);
    check("batched trigger press consumed once", BUTTON(action), 0);
    SDL_JoystickSetVirtualAxis(pad, 2, 32767); _handle_events();
    SDL_Delay(40); _handle_events();
    {
        int32 yaw, pitch;
        CONTROL_GetView(&yaw, &pitch);
        check("gamepad look reaches camera before tick", yaw > 0, 1);
        getinput(0);
        check("sampled gamepad look reaches tick", loc.avel > 0, 1);
        CONTROL_GetView(&yaw, &pitch);
        {
            int32 before = yaw;
            movefifoend[0]++;
            getinput(0);
            CONTROL_GetView(&yaw, &pitch);
            check("tick does not resample gamepad rotation", yaw, before);
            check("quantization cannot reverse held gamepad turn", loc.avel >= 0, 1);
        }
    }
    SDL_JoystickDetachVirtual(device);
    CONTROL_Shutdown();
    SDL_JoystickClose(pad);
    uninitengine();
    return failures != 0;
}
#else
int main(int argc, char **argv) {
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    game_init_release_defaults(); return 77; }
#endif
