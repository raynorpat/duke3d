#include "profile_test_support.h"
/* Fresh controls work without a config; saved player choices take precedence. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"
#include "mouse.h"

static int failures;

static void check(const char *what, int got, int want)
{
    if (got != want) {
        printf("FAIL %s: got %d, want %d\n", what, got, want);
        failures++;
    }
}

int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,"1.5")) return 1;
    game_init_release_defaults();
    static const struct { int key, function; } bindings[] = {
        { sc_W, gamefunc_Move_Forward }, { sc_S, gamefunc_Move_Backward },
        { sc_A, gamefunc_Strafe_Left }, { sc_D, gamefunc_Strafe_Right },
        { sc_Space, gamefunc_Jump }, { sc_E, gamefunc_Open },
        { sc_LeftControl, gamefunc_Crouch },
        { sc_UpArrow, gamefunc_Move_Forward },
        { sc_DownArrow, gamefunc_Move_Backward }
    };
    char home[256], cfg[512], *slash;
    const char *tmp = getenv("TMPDIR");
    FILE *f;
    unsigned i;
    int j;

    _platform_init(argc, argv, "input_defaults", "input_defaults");
    if (_setgamemode(1, 320, 200) != 0 || !initmouse())
        return 1;
    MOUSE_Init();
    snprintf(home, sizeof(home), "%s/duke3d_input_XXXXXX",
             tmp != NULL && tmp[0] != '\0' ? tmp : "/tmp");
    if (mkdtemp(home) == NULL)
        return 1;
    setenv("HOME", home, 1);
    if (setup_homedir() != 0)
        return 1;
    GetPathFromEnvironment(cfg, sizeof(cfg), "duke3d.cfg");
    CONFIG_GetSetupFilename();
    CONFIG_ReadSetup();
    CONTROL_Startup(ControllerType, NULL, TICRATE);
    check("fresh config uses keyboard and mouse", ControllerType, controltype_keyboardandmouse);
    check("fresh config has usable mouse sensitivity", CONTROL_GetMouseSensitivity() > 0, 1);
    check("fresh config enables mouse look", myaimmode, 1);
    check("fresh config enables player mouse aim", ps[0].aim_mode, 1);
    check("fresh config uses toggle mouse aim", MouseAiming, 0);
    check("fresh config enables crosshair", ud.crosshair, 1);
    check("mouse aim toggle is unbound", CONTROL_GetMappingName(gamefunc_Mouse_Aiming) == NULL ||
        strcmp(CONTROL_GetMappingName(gamefunc_Mouse_Aiming), "None") == 0, 1);
    ud.last_level = -1;
    resetplayerstats(0);
    check("new level keeps configured mouse look", ps[0].aim_mode, 1);
    ps[0].horiz = 100;
    for (i = 0; i < sizeof(bindings)/sizeof(bindings[0]); i++) {
        CONTROL_UpdateKeyboardState(bindings[i].key, 1);
        for (j = 0; j < NUMGAMEFUNCTIONS; j++)
            check("default key activates only its action", BUTTON(j), j == bindings[i].function);
        CONTROL_UpdateKeyboardState(bindings[i].key, 0);
        CONTROL_EndInput();
        check("default key releases its action", BUTTON(bindings[i].function), 0);
    }
    {
        SDL_Event ev;
        memset(&ev, 0, sizeof(ev));
        ev.type = SDL_MOUSEMOTION;
        ev.motion.xrel = 2;
        ev.motion.yrel = 2;
        SDL_PushEvent(&ev);
        _handle_events();
        getinput(0);
        check("mouse right turns right", loc.avel > 0, 1);
        check("mouse down looks down", loc.horz < 0, 1);
        check("mouse look does not move forward", loc.fvel, 0);
        check("mouse look does not strafe", loc.svel, 0);

        ev.type = SDL_MOUSEBUTTONDOWN;
        ev.button.button = SDL_BUTTON_LEFT;
        ev.button.state = SDL_PRESSED;
        SDL_PushEvent(&ev);
        _handle_events();
        getinput(0);
        check("left mouse fires", (loc.bits >> 2) & 1, 1);
        ev.type = SDL_MOUSEBUTTONUP;
        ev.button.state = SDL_RELEASED;
        SDL_PushEvent(&ev);
        _handle_events();
        getinput(0);
        check("left mouse releases fire", (loc.bits >> 2) & 1, 0);
    }
    CONFIG_WriteSetup();
    CONFIG_ReadSetup();
    check("mouse look survives config round trip", myaimmode, 1);
    check("crosshair survives config round trip", ud.crosshair, 1);
    check("mouse sensitivity survives config round trip", CONTROL_GetMouseSensitivity() > 0, 1);
    CONFIG_WriteSetup();

    f = fopen(cfg, "w");
    if (f == NULL)
        return 1;
    fputs("[Misc]\nCrosshairs = 0\n"
          "[Controls]\nControllerType = 1\nMouseSensitivity = 8192\n"
          "MouseAimingFlipped = 0\n"
          "GameMouseAiming = 0\nAimingFlag = 0\nMouseAiming = 1\n"
          "[KeyDefinitions]\nMove_Forward = \"X\" \"\"\n", f);
    fclose(f);
    CONFIG_ReadSetup();
    check("saved config keeps crosshair off", ud.crosshair, 0);
    check("saved config keeps mouse look off", myaimmode, 0);
    check("saved config keeps player mouse aim off", ps[0].aim_mode, 0);
    check("saved config keeps hold mouse aim", MouseAiming, 1);
    check("saved config keeps mouse inversion", ud.mouseflip, 0);
    check("saved config keeps mouse sensitivity", CONTROL_GetMouseSensitivity(), 8192);
    j = gamefunc_Move_Forward;
    CONTROL_UpdateKeyboardState(sc_X, 1);
    check("saved config keeps custom forward key", BUTTON(j), 1);
    CONTROL_UpdateKeyboardState(sc_X, 0);
    CONTROL_EndInput();
    CONTROL_UpdateKeyboardState(sc_W, 1);
    check("saved config replaces default forward key", BUTTON(j), 0);
    CONTROL_UpdateKeyboardState(sc_W, 0);
    CONFIG_WriteSetup();

    remove(cfg);
    while ((slash = strrchr(cfg, '/')) != NULL) {
        *slash = 0;
        if (rmdir(cfg) != 0 || strcmp(cfg, home) == 0)
            break;
    }
    _uninitengine();
    return failures != 0;
}
