/*
 * Checks game helper functions against the original DOS libraries'
 *  results (MACT386.LIB), and in 1.3D builds the episode and skill names
 *  the 1.3D executable has built in. Needs no game data.
 */
#define SDL_MAIN_HANDLED  /* game.c's main() is renamed; this one is ours. */
#include <sys/stat.h>
#include "duke3d.h"
#if PLATFORM_WIN32
#include <direct.h>
#define mkdir(d, m) _mkdir(d)
#define rmdir _rmdir
#endif

static int failures;

static void check(const char *what, int got, int want)
{
    if (got != want) {
        printf("FAIL %s: got %d, want %d\n", what, got, want);
        failures++;
    }
}

static void check_str(const char *what, const char *got, const char *want)
{
    if (strcmp(got, want) != 0) {
        printf("FAIL %s: got \"%s\", want \"%s\"\n", what, got, want);
        failures++;
    }
}

int main(void)
{
    /* On an axis the original returns the exact length, not the estimate. */
    check("FindDistance2D(1000,0)", FindDistance2D(1000, 0), 1000);
    check("FindDistance2D(0,-1000)", FindDistance2D(0, -1000), 1000);
    check("FindDistance2D(-7,0)", FindDistance2D(-7, 0), 7);
    check("FindDistance2D(0,0)", FindDistance2D(0, 0), 0);
    /* Off the axes it is the estimate: 1000 - 31 - 7 + 0 + 0 for (1000,1). */
    check("FindDistance2D(1000,1)", FindDistance2D(1000, 1), 962);
    check("FindDistance2D(3000,4000)", FindDistance2D(3000, 4000), 5039);

#if DUKE3D_GAMEVER < GAMEVER_14
    /* 1.3D CONs have no definevolumename or defineskillname. */
    check_str("volume_names[0]", volume_names[0], "L.A. MELTDOWN");
    check_str("volume_names[1]", volume_names[1], "LUNAR APOCALYPSE");
    check_str("volume_names[2]", volume_names[2], "SHRAPNEL CITY");
    check_str("skill_names[0]", skill_names[0], "PIECE OF CAKE");
    check_str("skill_names[1]", skill_names[1], "LET'S ROCK");
    check_str("skill_names[2]", skill_names[2], "COME GET SOME");
    check_str("skill_names[3]", skill_names[3], "DAMN I'M GOOD");

    /* 1.3D saves and loads locally even in a network game: a negative slot
       (the menu's multiplayer request) must not queue a request that 1.3D
       has no handler for. A directory named game0.sav makes the save's fopen
       fail, and there is no game1.sav, so neither call touches game state. */
    mkdir("game0.sav", 0755);
    multiflag = 0;
    saveplayer(-1);
    check("multiflag after saveplayer(-1)", multiflag, 0);
    multiflag = 0;
    loadplayer(-2);
    check("multiflag after loadplayer(-2)", multiflag, 0);
    rmdir("game0.sav");
#endif

    /* The GRP check: a known release GRP that isn't this build's release
       gets a warning; this build's own GRP, a modded one and an unknown one
       don't. CRC-32 is the standard (zlib) one. */
    check("grpcrc32(\"123456789\")", (int)grpcrc32(0, (const unsigned char *)"123456789", 9), (int)0xCBF43926);
    check("unknown GRP", grpmismatch(123456, 0x12345678) != NULL, 0);
    check("modded GRP the size of a release", grpmismatch(26524524, 0x12345678) != NULL, 0);
#if DUKE3D_GAMEVER < GAMEVER_14
#ifdef VOLUMEONE
    check("1.3D shareware GRP", grpmismatch(11035779, 0x983AD923) != NULL, 0);
    check("1.3D full GRP", grpmismatch(26524524, 0xBBC9CE44) != NULL, 1);
#else
    check("1.3D full GRP", grpmismatch(26524524, 0xBBC9CE44) != NULL, 0);
    check("1.3D shareware GRP", grpmismatch(11035779, 0x983AD923) != NULL, 1);
#endif
    check("Atomic GRP", grpmismatch(44356548, 0xFD3DCFF1) != NULL, 1);
    check("1.4 GRP", grpmismatch(44348015, 0xF514A6AC) != NULL, 1);
#elif DUKE3D_GAMEVER < GAMEVER_15
    check("1.4 GRP", grpmismatch(44348015, 0xF514A6AC) != NULL, 0);
    check("Atomic GRP", grpmismatch(44356548, 0xFD3DCFF1) != NULL, 1);
#else
    check("Atomic GRP", grpmismatch(44356548, 0xFD3DCFF1) != NULL, 0);
    check("1.4 GRP", grpmismatch(44348015, 0xF514A6AC) != NULL, 1);
    check("1.3D full GRP", grpmismatch(26524524, 0xBBC9CE44) != NULL, 1);
#endif
    check("1.1 shareware GRP", grpmismatch(10442980, 0x912E1E8D) != NULL, 1);

    if (failures) {
        printf("%d failures\n", failures);
        return 1;
    }
    printf("game_helpers: all match\n");
    return 0;
}
