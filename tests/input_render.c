#include "profile_test_support.h"
/* The renderer must retain orientation below one Build angle/horizon unit. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"

extern int32_t cosglobalang, singlobalang, globalhoriz;
int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,"1.5")) return 1;
    game_init_release_defaults();
    int32_t sine, horizon;
    int w, f;
    _platform_init(argc, argv, "input_render", "input_render");
    initengine();
    if (_setgamemode(1, 1024, 768)) return 1;
    initspritelists();
    numsectors = 1; sector[0].wallnum = 0;
    sintable[0] = 0; sintable[1] = 50;
    sintable[512] = 16384; sintable[513] = 16383;
    setviewanglefraction(0, 0);
    drawrooms(0, 0, 0, 0, 100, MAXSECTORS);
    sine = singlobalang; horizon = globalhoriz;
    setviewanglefraction(32768, 32768);
    drawrooms(0, 0, 0, 0, 100, MAXSECTORS);
    if (singlobalang == sine || globalhoriz == horizon) {
        puts("FAIL fractional yaw/pitch was truncated before rasterization");
        return 1;
    }
    setviewanglefraction(0, 0);
    drawrooms(0, 0, 0, 0, 100, MAXSECTORS);
    if (singlobalang != sine || globalhoriz != horizon) {
        puts("FAIL integer renderer behavior changed");
        return 1;
    }
    /* At equal height, Hor+ keeps the horizon at the same screen row.
     * Integer and fractional look must both use the vertical focal length. */
    for (w = 640; w <= 1600; w += 320) {
        if (_setgamemode(1, w, 480)) return 1;
        for (f = 0; f < 2; f++) {
            setviewanglefraction(0, f ? 32768 : 0);
            drawrooms(0, 0, 0, 0, 130, MAXSECTORS);
            if (globalhoriz != (f ? 313 : 311)) {
                printf("FAIL Hor+ horizon at %dx480: %d\n", w, (int)globalhoriz);
                return 1;
            }
        }
    }
    uninitengine();
    return 0;
}
