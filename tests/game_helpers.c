/*
 * Checks game helper functions against the original DOS libraries'
 *  results (MACT386.LIB). Needs no game data.
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

    if (failures) {
        printf("%d failures\n", failures);
        return 1;
    }
    printf("game_helpers: all match\n");
    return 0;
}
