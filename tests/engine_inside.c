/*
 * Checks inside() against the point-in-sector test of each engine revision.
 *  The sector is a diamond at the edge of the map, where the cross product
 *  x1*y2-x2*y1 overflows 32 bits. The 1.0 and 1.1 engines (before 19960320)
 *  compare the two products, each wrapped to 32 bits, and report some points
 *  outside the sector as inside; the later engines subtract the products and
 *  get those right. Points that don't overflow come out the same in every
 *  revision.
 */
#include <stdio.h>
#include <stdint.h>
#include "build/build.h"

static int failures;

/* The engine calls these into the game. */
void timerhandler(void) { }
void keyhandler(void) { }
void faketimerhandler(void) { }

static void check(const char *what, int got, int want)
{
    if (got != want) {
        printf("FAIL %s: got %d, want %d\n", what, got, want);
        failures++;
    }
}

/* Fills sector s with a diamond of the given radius, from wall w. */
static void diamond(int s, int w, int32_t r)
{
    static const int dx[4] = { -1, 0, 1, 0 }, dy[4] = { 0, -1, 0, 1 };
    int i;

    for (i = 0; i < 4; i++) {
        wall[w+i].x = dx[i]*r; wall[w+i].y = dy[i]*r; wall[w+i].point2 = w + ((i+1) & 3);
    }
    sector[s].wallptr = w; sector[s].wallnum = 4;
}

int main(void)
{
    diamond(0, 0, 1000);
    diamond(1, 4, 131072);
    numsectors = 2;

    check("centre", inside(0, 0, 0), 1);
    check("inside off centre", inside(300, -200, 0), 1);
    check("inside near edge", inside(-900, 50, 0), 1);
    check("outside corner", inside(800, 800, 0), 0);
    check("outside right", inside(1500, 0, 0), 0);
    check("big, centre", inside(0, 0, 1), 1);
    check("big, outside corner", inside(100000, 100000, 1), 0);
    check("bad sector", inside(0, 0, 2), -1);

    /* Outside the diamond, with x1*y2 near 2^34. */
#if BUILD_ENGINEREV < 19960320L
    check("outside, products overflow", inside(-130000, 50210, 1), 1);
    check("outside, products overflow 2", inside(-122081, -57916, 1), 1);
#else
    check("outside, products overflow", inside(-130000, 50210, 1), 0);
    check("outside, products overflow 2", inside(-122081, -57916, 1), 0);
#endif

    if (failures) return 1;
    printf("inside: ok\n");
    return 0;
}
