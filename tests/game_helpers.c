/*
 * Checks game helper functions against the original DOS libraries'
 *  results (MACT386.LIB), and in 1.3D builds the episode and skill names
 *  the 1.3D executable has built in. Needs no game data.
 */
#define SDL_MAIN_HANDLED  /* game.c's main() is renamed; this one is ours. */
#include "duke3d.h"

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
#endif

    if (failures) {
        printf("%d failures\n", failures);
        return 1;
    }
    printf("game_helpers: all match\n");
    return 0;
}
