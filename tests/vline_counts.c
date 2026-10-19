/*
 * Checks how many pixels the engine's C column drawers (build/a.c) write.
 *  The original A.ASM loops end with "sub ecx,1 / jnc", so a count of n
 *  draws n+1 pixels, and the engine's callers pass the inclusive end of the
 *  column. A drawer that writes n pixels loses the bottom row of everything
 *  it draws.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* As defined in build/a.c. */
void setvlinebpl(int32_t i1);
void setupvlineasm(int32_t i1);
void setupmvlineasm(int32_t i1);
int32_t vlineasm1(int32_t vince, intptr_t palookupoffse, int32_t i3, int32_t vplce, intptr_t bufplce, intptr_t i6);
int32_t mvlineasm1(int32_t vince, intptr_t palookupoffse, int32_t i3, int32_t vplce, intptr_t bufplce, intptr_t i6);

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

int main(void)
{
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

    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("vline_counts: ok\n");
    return 0;
}
