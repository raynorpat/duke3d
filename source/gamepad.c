/* SDL's named controller controls, independent of legacy joystick mappings. */
#include "duke3d.h"
#include "gamepad.h"

uint32_t GAMEPAD_ButtonState1, GAMEPAD_ButtonState2;

static SDL_GameController *controller;
static int initialized, was_menu = -1;
static uint64_t suppressed;
static int axes[4];
static unsigned previous_keys;
static Uint32 repeat_at[4];

static int stick(int value)
{
    const int deadzone = 8000;
    if (value > deadzone)
        return (value - deadzone) * 32767 / (32767 - deadzone);
    if (value < -deadzone)
        return (value + deadzone) * 32767 / (32768 - deadzone);
    return 0;
}

static void open_controller(void)
{
    const char *wanted = getenv("BUILD_SDLJOYSTICK");
    int i;
    for (i = 0; i < SDL_NumJoysticks(); i++) {
        if (!SDL_IsGameController(i))
            continue;
        if (wanted != NULL && strcmp(wanted, SDL_GameControllerNameForIndex(i)) != 0)
            continue;
        controller = SDL_GameControllerOpen(i);
        if (controller != NULL)
            break;
    }
}

void GAMEPAD_Init(void)
{
    const char *wanted = getenv("BUILD_SDLJOYSTICK");
    if (initialized || (wanted != NULL && strcmp(wanted, "none") == 0))
        return;
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0) {
        initialized = 1;
        open_controller();
    }
}

void GAMEPAD_Shutdown(void)
{
    if (controller != NULL)
        SDL_GameControllerClose(controller);
    controller = NULL;
    if (initialized)
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    initialized = 0;
    GAMEPAD_ButtonState1 = GAMEPAD_ButtonState2 = 0;
    suppressed = previous_keys = 0;
    was_menu = -1;
    memset(axes, 0, sizeof(axes));
    memset(repeat_at, 0, sizeof(repeat_at));
    memset((void *)KB_GamepadKeyDown, 0, sizeof(KB_GamepadKeyDown));
}

int GAMEPAD_Connected(void)
{
    return controller != NULL;
}

void GAMEPAD_ClearButton(int action)
{
    suppressed |= UINT64_C(1) << action;
    if (action < 32)
        GAMEPAD_ButtonState1 &= ~(UINT32_C(1) << action);
    else
        GAMEPAD_ButtonState2 &= ~(UINT32_C(1) << (action - 32));
}

void GAMEPAD_Poll(void)
{
    static const int actions[] = {
        gamefunc_Jump, gamefunc_Crouch, gamefunc_Open, gamefunc_Inventory,
        gamefunc_Map, -1, -1, gamefunc_Run, gamefunc_Crouch,
        gamefunc_Previous_Weapon, gamefunc_Next_Weapon,
        gamefunc_MedKit, gamefunc_Jetpack, gamefunc_Inventory_Left, gamefunc_Inventory_Right
    };
    static const int keys[] = {
        sc_UpArrow, sc_DownArrow, sc_LeftArrow, sc_RightArrow, sc_Enter, sc_Escape
    };
    uint64_t held = 0;
    unsigned buttons = 0, menu_keys = 0;
    int i, menu;
    Uint32 now;

    if (!initialized)
        return;
    SDL_GameControllerUpdate();
    if (controller != NULL && !SDL_GameControllerGetAttached(controller)) {
        SDL_GameControllerClose(controller);
        controller = NULL;
        GAMEPAD_ButtonState1 = GAMEPAD_ButtonState2 = 0;
        suppressed = previous_keys = 0;
        was_menu = -1;
        memset(axes, 0, sizeof(axes));
        memset((void *)KB_GamepadKeyDown, 0, sizeof(KB_GamepadKeyDown));
    }
    if (controller == NULL)
        open_controller();
    if (controller == NULL)
        return;

    for (i = 0; i < (int)(sizeof(actions)/sizeof(actions[0])); i++) {
        if (SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)i)) {
            buttons |= 1u << i;
            if (actions[i] >= 0)
                held |= UINT64_C(1) << actions[i];
        }
    }
    if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 8000)
        held |= UINT64_C(1) << gamefunc_Fire;
    if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 8000)
        held |= UINT64_C(1) << gamefunc_Quick_Kick;
    for (i = 0; i < 4; i++)
        axes[i] = stick(SDL_GameControllerGetAxis(controller, (SDL_GameControllerAxis)i));

    menu = (ps[myconnectindex].gm & MODE_MENU) || !(ps[myconnectindex].gm & MODE_GAME);
    if (menu) {
        if ((buttons & (1u << SDL_CONTROLLER_BUTTON_DPAD_UP)) || axes[1] < -16000) menu_keys |= 1u;
        if ((buttons & (1u << SDL_CONTROLLER_BUTTON_DPAD_DOWN)) || axes[1] > 16000) menu_keys |= 2u;
        if ((buttons & (1u << SDL_CONTROLLER_BUTTON_DPAD_LEFT)) || axes[0] < -16000) menu_keys |= 4u;
        if ((buttons & (1u << SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) || axes[0] > 16000) menu_keys |= 8u;
        if (buttons & (1u << SDL_CONTROLLER_BUTTON_A)) menu_keys |= 16u;
        if (buttons & (1u << SDL_CONTROLLER_BUTTON_B)) menu_keys |= 32u;
    }
    if (buttons & (1u << SDL_CONTROLLER_BUTTON_START)) menu_keys |= 32u;
    if (was_menu >= 0 && menu != was_menu) {
        suppressed |= held;
        previous_keys = menu_keys;
        memset((void *)KB_GamepadKeyDown, 0, sizeof(KB_GamepadKeyDown));
    }
    was_menu = menu;
    suppressed &= held;
    if (menu) {
        suppressed |= held;
        memset(axes, 0, sizeof(axes));
        held = 0;
    }
    held &= ~suppressed;
    GAMEPAD_ButtonState1 = (uint32_t)held;
    GAMEPAD_ButtonState2 = (uint32_t)(held >> 32);

    now = SDL_GetTicks();
    for (i = 0; i < 6; i++) {
        unsigned bit = 1u << i;
        if (!(menu_keys & bit)) {
            KB_GamepadKeyDown[keys[i]] = false;
            continue;
        }
        if (!(previous_keys & bit)) {
            KB_QueueGamepadKey(keys[i]);
            if (i < 4) repeat_at[i] = now + 350;
        } else if (i < 4 && (Sint32)(now - repeat_at[i]) >= 0) {
            KB_QueueGamepadKey(keys[i]);
            repeat_at[i] = now + 100;
        }
    }
    previous_keys = menu_keys;
}

void GAMEPAD_AddInput(int running, int32_t *forward, int32_t *strafe,
                     int32_t *turn, int32_t *look)
{
    int speed = running ? 80 : 40;
    *forward -= axes[1] * speed / 32767;
    *strafe -= axes[0] * speed / 32767;
    *turn += axes[2] * 45 / 32767;
    *look -= axes[3] * 32 / 32767;
}
