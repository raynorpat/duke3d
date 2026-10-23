/* Device sampling must not consume gameplay input. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"
#include "mouse.h"
#include <math.h>

static int failures;
static void check(const char *name, int got, int want)
{
    if (got != want) { printf("FAIL %s: %d != %d\n", name, got, want); failures++; }
}
static void event(int type, int x, int y)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e)); e.type = type;
    if (type == SDL_MOUSEMOTION) { e.motion.xrel = x; e.motion.yrel = y; }
    else if (type == SDL_MOUSEWHEEL) {
        e.wheel.y = y;
#if SDL_VERSION_ATLEAST(2, 0, 18)
        e.wheel.preciseY = (float)y;
#endif
    }
    else { e.button.button = x; e.button.state = type == SDL_MOUSEBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED; }
    SDL_PushEvent(&e); _handle_events();
}
int main(int argc, char **argv)
{
    ControlInfo a, b;
    int fire = gamefunc_Fire;
    _platform_init(argc, argv, "input_polling", "input_polling");
    if (_setgamemode(1, 320, 200) || !initmouse()) return 1;
    MOUSE_Init(); CONFIG_SetDefaults(); CONTROL_Startup(ControllerType, NULL, TICRATE);
    CONTROL_MapButton(gamefunc_Fire, 0, false);
    CONTROL_MapButton(gamefunc_Previous_Weapon, 3, false);
    CONTROL_MapButton(gamefunc_Next_Weapon, 4, false);
    CONTROL_ResetInput();
    event(SDL_MOUSEMOTION, 1, -1);
    CONTROL_PollDevices(); CONTROL_PollDevices();
    CONTROL_GetInput(&a); CONTROL_GetInput(&b);
    check("diagonal yaw", a.dyaw > 0, 1); check("diagonal pitch", a.dpitch < 0, 1);
    check("consumed once", b.dyaw || b.dpitch, 0);
    event(SDL_MOUSEMOTION, 12000, 0); CONTROL_GetInput(&a);
    check("high DPI motion does not wrap at 16 bits", a.dyaw > 0, 1);
    event(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, 0);
    event(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, 0);
    check("short click survives until tick", BUTTON(fire), 1);
    CONTROL_EndInput(); check("short click consumed", BUTTON(fire), 0);
    {
        SDL_Event e;
        memset(&e, 0, sizeof(e)); e.type = SDL_MOUSEBUTTONDOWN;
        e.button.button = SDL_BUTTON_LEFT; e.button.state = SDL_PRESSED;
        SDL_PushEvent(&e);
        e.type = SDL_MOUSEBUTTONUP; e.button.state = SDL_RELEASED;
        SDL_PushEvent(&e); _handle_events();
        check("batched short mouse press survives", BUTTON(fire), 1);
        CONTROL_EndInput(); check("batched short press consumed", BUTTON(fire), 0);
    }
    CONTROL_MapKey(fire, sc_X, sc_Y);
    CONTROL_UpdateKeyboardState(sc_X, 1); CONTROL_UpdateKeyboardState(sc_Y, 1);
    CONTROL_UpdateKeyboardState(sc_X, 0); CONTROL_EndInput();
    check("second keyboard binding remains held", BUTTON(fire), 1);
    event(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT, 0);
    event(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT, 0); CONTROL_EndInput();
    check("mouse release preserves keyboard", BUTTON(fire), 1);
    CONTROL_UpdateKeyboardState(sc_Y, 0); CONTROL_EndInput();
    event(SDL_MOUSEWHEEL, 0, 3); event(SDL_MOUSEWHEEL, 0, -2);
    check("wheel 1", CONTROL_GetWheelDirection(), -1);
    check("wheel 2", CONTROL_GetWheelDirection(), -1);
    check("wheel 3", CONTROL_GetWheelDirection(), -1);
    check("wheel reversal 1", CONTROL_GetWheelDirection(), 1);
    check("wheel reversal 2", CONTROL_GetWheelDirection(), 1);
    check("wheel exhausted", CONTROL_GetWheelDirection(), 0);
    event(SDL_MOUSEMOTION, 2, 2); event(SDL_MOUSEWHEEL, 0, 1);
    CONTROL_ResetInput(); CONTROL_GetInput(&a);
    check("reset clears movement", a.dyaw || a.dpitch || a.dz, 0);
    check("reset clears wheel", CONTROL_GetWheelDirection(), 0);
    ps[0].gm = MODE_GAME; ps[0].newowner = -1; ps[0].horiz = 100;
    CONTROL_ResetInput(); CONTROL_PollDevices();
    CONTROL_SetMouseSensitivity(1024);
    {
        int32 yaw, pitch, before;
        event(SDL_MOUSEMOTION, 1, 1);
        CONTROL_GetView(&yaw, &pitch);
        check("frame mouse yaw is fractional and immediate", yaw > 0 && yaw < 65536, 1);
        check("frame mouse pitch is fractional and immediate", pitch < 0 && pitch > -65536, 1);
        before = yaw;
        getinput(0); CONTROL_GetView(&yaw, &pitch);
        check("tick preserves unquantized view", yaw, before);
        event(SDL_MOUSEMOTION, -1, -1); CONTROL_GetView(&yaw, &pitch);
        check("tiny opposite yaw cancels", yaw, 0);
        check("tiny opposite pitch cancels", abs(pitch) < 2, 1);
        CONTROL_ResetInput();
        CONTROL_SetMouseSensitivity(32768);
        event(SDL_MOUSEMOTION, 1, 0); CONTROL_GetView(&yaw, &pitch); before = yaw;
        getinput(0); CONTROL_GetView(&yaw, &pitch);
        check("submitted input stays visible before simulation", yaw, before);
        CONTROL_AcknowledgeInput(movefifoend[0]); CONTROL_GetView(&yaw, &pitch);
        check("acknowledgment removes simulated turn once", yaw, before - loc.avel *
#if DUKE3D_GAMEVER < GAMEVER_11
            TICSPERFRAME
#elif DUKE3D_GAMEVER < GAMEVER_13D
            1
#else
            2
#endif
            * 65536);
        before = yaw; CONTROL_AcknowledgeInput(movefifoend[0]); CONTROL_GetView(&yaw, &pitch);
        check("prediction replay cannot acknowledge twice", yaw, before);
        event(SDL_MOUSEMOTION, 1, 0);
        ps[0].gm |= MODE_MENU; CONTROL_PollDevices();
        CONTROL_GetView(&yaw, &pitch);
        check("menu clears pending camera movement", yaw || pitch, 0);
        ps[0].gm = MODE_GAME; CONTROL_PollDevices();
        CONTROL_GetView(&yaw, &pitch);
        check("resume has no camera jump", yaw || pitch, 0);
        {
            SDL_Event e;
            event(SDL_MOUSEMOTION, 1, 1);
            memset(&e, 0, sizeof(e)); e.type = SDL_WINDOWEVENT;
            e.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
            SDL_PushEvent(&e); _handle_events();
            CONTROL_GetView(&yaw, &pitch);
            check("focus loss clears camera movement", yaw || pitch, 0);
            event(SDL_MOUSEMOTION, 5, 5); CONTROL_GetView(&yaw, &pitch);
            check("background mouse motion is ignored", yaw || pitch, 0);
            e.window.event = SDL_WINDOWEVENT_FOCUS_GAINED;
            SDL_PushEvent(&e); _handle_events();
            CONTROL_GetView(&yaw, &pitch);
            check("focus return has no camera jump", yaw || pitch, 0);
        }
    }
    {
        double angles[2];
        int i;
        for (i = 0; i < 2; i++) {
            int32 yaw, pitch;
            ps[0].horiz = i == 0 ? 100 : 250;
            CONTROL_ResetInput(); CONTROL_PollDevices();
            CONTROL_SetMouseSensitivity(32768);
            event(SDL_MOUSEMOTION, 0, 8); CONTROL_GetView(&yaw, &pitch);
            /* A square-pixel 320x200 view has a 160-pixel focal length. */
            angles[i] = atan((ps[0].horiz + pitch/65536.0 - 100) / 160.0) -
                atan((ps[0].horiz - 100) / 160.0);
        }
        check("spherical pitch matches renderer projection", fabs(angles[0] - angles[1]) < 0.000001, 1);
        ps[0].horiz = 100;
    }
    {
        int packet, tick;
        for (packet = 2; packet <= 4; packet *= 2) {
            int32 yaw, pitch, desired, desired_pitch, applied = 0, applied_pitch = 0;
            CONTROL_ResetInput(); CONTROL_PollDevices();
            CONTROL_SetMouseSensitivity(1024);
            numplayers = ud.multimode = 1; movesperpacket = packet;
            myconnectindex = connecthead = 0; connectpoint2[0] = -1;
            ready2send = 1; movefifoend[0] = 0;
            avgfvel = avgsvel = avgavel = avghorz = avgbits = 0;
            event(SDL_MOUSEMOTION, 512, 64);
            CONTROL_GetView(&desired, &desired_pitch);
            for (tick = 0; tick < packet; tick++) {
                totalclock = ototalclock + TICSPERFRAME;
                faketimerhandler();
                applied += inputfifo[tick][0].avel *
#if DUKE3D_GAMEVER < GAMEVER_11
                    TICSPERFRAME
#elif DUKE3D_GAMEVER < GAMEVER_13D
                    1
#else
                    2
#endif
                    * 65536;
                applied_pitch += (inputfifo[tick][0].horz >> 1) * 65536;
                CONTROL_AcknowledgeInput(tick);
                CONTROL_GetView(&yaw, &pitch);
                check("packetized camera accounts for actual FIFO turn", yaw + applied, desired);
                check("packetized camera accounts for actual FIFO pitch", pitch + applied_pitch, desired_pitch);
            }
        }
        movesperpacket = 1; ready2send = 0; CONTROL_ResetInput();
    }
    {
        struct player_struct *p = &ps[0];
        int i;
        memset(p, 0, sizeof(*p));
        p->newowner = -1; p->weapon_pos = -4; p->last_weapon = 0;
        sprite[0].xrepeat = 48; sprite[0].extra = 100;
        for (i = 0; i < 4; i++) { p->gotweapon[i] = 1; p->ammo_amount[i] = 10; }
        p->curr_weapon = 1; SoundToggle = 0;
        sync[0].bits = 14 << 8; cheatkeys(0);
        check("cycle during lowering", p->curr_weapon, 2);
        check("keep lowering progress", p->weapon_pos, -4);
        check("keep displayed old weapon", p->last_weapon, 0);
        cheatkeys(0); check("successive cycle steps count", p->curr_weapon, 3);
        sync[0].bits = 13 << 8; cheatkeys(0);
        check("cycle reversal", p->curr_weapon, 2);
        p->ammo_amount[1] = 0; cheatkeys(0);
        check("skip empty weapon", p->curr_weapon, 0);
        cheatkeys(0); check("wrap past unavailable weapons", p->curr_weapon, 3);
        p->weapon_pos = 6; sync[0].bits = 14 << 8; cheatkeys(0);
        check("cycle during raising", p->curr_weapon, 0);
    }
    CONTROL_Shutdown(); _uninitengine();
    return failures != 0;
}
