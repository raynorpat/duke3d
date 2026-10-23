#include "profile_test_support.h"
/*
 * Checks how many pixels the engine's C column drawers (build/draw.c) write.
 *  The original A.ASM loops end with "sub ecx,1 / jnc", so a count of n
 *  draws n+1 pixels, and the engine's callers pass the inclusive end of the
 *  column. A drawer that writes n pixels loses the bottom row of everything
 *  it draws.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* As defined in build/draw.c. */
void setvlinebpl(int32_t i1);
void setupvlineasm(int32_t i1);
void setupmvlineasm(int32_t i1);
int32_t vlineasm1(int32_t vince, intptr_t palookupoffse, int32_t i3, int32_t vplce, intptr_t bufplce, intptr_t i6);
int32_t mvlineasm1(int32_t vince, intptr_t palookupoffse, int32_t i3, int32_t vplce, intptr_t bufplce, intptr_t i6);
void setupdrawslab(int32_t dabpl, intptr_t pal);
void drawslab(int32_t dx, int32_t v, int32_t dy, int32_t vi, intptr_t vptr, intptr_t p);

/* The engine calls these into the game. */
void timerhandler(void) { }
void keyhandler(void) { }
void faketimerhandler(void) { }

static int failures;

static void check(const char *what, unsigned char *dest, int count)
{
    int i;

    for (i = 0; i <= count; i++)
        if (dest[i] != 100 + i) {
            printf("FAIL %s: row %d is %d, want %d (count %d draws %d rows)\n",
                   what, i, dest[i], 100 + i, count, count + 1);
            failures++;
            return;
        }
    if (dest[count + 1] != 0) {
        printf("FAIL %s: row %d was drawn too\n", what, count + 1);
        failures++;
    }
}

/* drawslab (classic voxels) as a.c had it before its palette lookup moved
 * out of the x loop: the output must stay byte for byte the same. */
static void drawslab_before(int32_t bpl, const unsigned char *pal, int32_t dx, int32_t v,
                            int32_t dy, int32_t vi, intptr_t vptr, intptr_t p)
{
    int32_t x;
    unsigned char *pp = (unsigned char *)p;

    while (dy > 0)
    {
        for(x=0;x<dx;x++) *(pp+x) = pal[(int32_t)(*(unsigned char *)((v>>16)+vptr))];
        pp += bpl; v += vi; dy--;
    }
}

static void check_drawslab(void)
{
    static unsigned char pal[256], src[256], want[64 * 72], got[64 * 72];
    uint32_t seed = 12345;
    int i, n;

#define RND() (seed = seed * 1103515245u + 12345u, (int32_t)(seed >> 8))
    for (i = 0; i < 256; i++) { pal[i] = (unsigned char)RND(); src[i] = (unsigned char)RND(); }
    for (n = 0; n < 5000; n++) {
        int32_t bpl = 40 + RND() % 24, dx = RND() % 41, dy = RND() % 64 - 3;
        /* Up to 2 texels a row from the middle: v>>16 stays in src. */
        int32_t vi = (RND() % 0x40000) - 0x20000, v = (128 << 16) + (RND() & 0xffff);
        for (i = 0; i < (int)sizeof(want); i++) want[i] = got[i] = (unsigned char)RND();
        drawslab_before(bpl, pal, dx, v, dy, vi, (intptr_t)src, (intptr_t)(want + 4));
        setupdrawslab(bpl, (intptr_t)pal);
        drawslab(dx, v, dy, vi, (intptr_t)src, (intptr_t)(got + 4));
        if (memcmp(want, got, sizeof(want))) {
            printf("FAIL drawslab: case %d (dx %d dy %d bpl %d) differs from before\n",
                   n, (int)dx, (int)dy, (int)bpl);
            failures++;
            return;
        }
    }
#undef RND
}

int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    static unsigned char pal[256], tile[16], dest[16];
    int i;

    for (i = 0; i < 256; i++) pal[i] = (unsigned char)(100 + i);
    for (i = 0; i < 16; i++) tile[i] = (unsigned char)i;
    setvlinebpl(1);

    /* A 5-row column: texel i lands on row i. */
    setupvlineasm(24);
    memset(dest, 0, sizeof(dest));
    vlineasm1(1 << 24, (intptr_t)pal, 4, 0, (intptr_t)tile, (intptr_t)dest);
    check("vlineasm1", dest, 4);

    setupmvlineasm(24);
    memset(dest, 0, sizeof(dest));
    mvlineasm1(1 << 24, (intptr_t)pal, 4, 0, (intptr_t)tile, (intptr_t)dest);
    check("mvlineasm1", dest, 4);

    check_drawslab();

    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("vline_counts: ok\n");
    return 0;
}
