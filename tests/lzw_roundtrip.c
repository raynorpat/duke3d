/*
 * Writes buffers with the engine's dfwrite() and reads them back with
 *  kdfread(), so the LZW compressor and decompressor have to agree. Needs
 *  no game data.
 *
 * With an argument, it also reads the start of that DOS save, which must
 *  have been written by the DOS executable of this build's version: the
 *  header and the map counts only come out right if kdfread() decompresses
 *  the way that executable compressed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "build/build.h"
#include "build/cache1d.h"

#define TMPNAME "lzw_roundtrip.tmp"

/* The engine calls these into the game. */
void timerhandler(void) { }
void keyhandler(void) { }
void faketimerhandler(void) { }

static int failures;

static uint32_t rng = 12345;
static unsigned char rnd(void) { rng = rng * 1103515245 + 12345; return (unsigned char)(rng >> 16); }

static void roundtrip(const char *what, unsigned char *data, size_t dasizeof, size_t count)
{
    size_t n = dasizeof * count;
    unsigned char *back = malloc(n);
    FILE *f = fopen(TMPNAME, "wb");
    int32_t fil;

    dfwrite(data, dasizeof, count, f);
    fclose(f);
    memset(back, 0xcc, n);
    fil = kopen4load(TMPNAME, 0);
    if (fil == -1) {
        printf("FAIL %s: can't reopen %s\n", what, TMPNAME);
        failures++;
    } else {
        kdfread(back, dasizeof, count, fil);
        kclose(fil);
        if (memcmp(data, back, n) != 0) {
            size_t i = 0;
            while (data[i] == back[i]) i++;
            printf("FAIL %s: byte %lu differs (%d, wrote %d)\n", what, (unsigned long)i, back[i], data[i]);
            failures++;
        }
    }
    free(back);
    remove(TMPNAME);
}

static void check_dos_save(const char *path)
{
    int32_t fil = kopen4load(path, 0);
    int32_t version, vol, lev, skill, i;
    char name[20];
    short numwalls, numsectors;
    static unsigned char thumb[160 * 100], walls[32 * 8192];

    if (fil == -1) {
        printf("FAIL: can't open %s\n", path);
        failures++;
        return;
    }
    kdfread(&version, 4, 1, fil);
    kdfread(name, 19, 1, fil);
    name[19] = 0;
    kdfread(&vol, 4, 1, fil);
    kdfread(&lev, 4, 1, fil);
    kdfread(&skill, 4, 1, fil);
    kdfread(thumb, 160, 100, fil);
    kdfread(&numwalls, 2, 1, fil);
    kdfread(walls, 32, 8192, fil);
    kdfread(&numsectors, 2, 1, fil);
    kclose(fil);

    printf("DOS save: version %d, name \"%s\", E%dL%d, skill %d, %d walls, %d sectors\n",
           version, name, vol + 1, lev + 1, skill, numwalls, numsectors);
    for (i = 0; name[i]; i++)
        if (name[i] < 32 || name[i] > 126) {
            printf("FAIL: the save name isn't text\n");
            failures++;
            break;
        }
    if (name[0] == 0 || vol < 0 || vol > 3 || lev < 0 || lev > 10 || skill < 0 || skill > 4 ||
        numwalls <= 0 || numwalls > MAXWALLS || numsectors <= 0 || numsectors > MAXSECTORS) {
        printf("FAIL: the save header or map counts are out of range\n");
        failures++;
    }
}

int main(int argc, char **argv)
{
    static unsigned char buf[100000];
    size_t i;

    initcache((intptr_t)malloc(4 << 20), 4 << 20);

    memset(buf, 0, sizeof(buf));
    roundtrip("zeros", buf, 1, 40000);
    for (i = 0; i < sizeof(buf); i++) buf[i] = rnd();
    roundtrip("noise", buf, 1, sizeof(buf));
    for (i = 0; i < sizeof(buf); i++) buf[i] = "Duke Nukem 3D "[i % 14];
    roundtrip("text", buf, 1, 30000);
    for (i = 0; i < 44 * 2000; i++) buf[i] = (unsigned char)((i % 44) < 12 ? i / 44 : rnd() & 3);
    roundtrip("records", buf, 44, 2000);
    roundtrip("one long", buf, 4, 1);
    roundtrip("odd size", buf + 3, 19, 1);

    if (argc > 1)
        check_dos_save(argv[1]);

    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("lzw_roundtrip: ok\n");
    return 0;
}
