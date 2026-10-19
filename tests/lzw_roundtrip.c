/*
 * Writes buffers with the engine's dfwrite() and reads them back with
 *  kdfread(), so the LZW compressor and decompressor have to agree. Needs
 *  no game data.
 *
 * It also feeds kdfread() malformed blocks (bad lengths, string counts and
 *  codes), which must come back without crashing or hanging.
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
#if !PLATFORM_WIN32
#include <unistd.h>
#endif
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

/* Writes one block (a length, then the bytes) followed by tail bytes of junk,
   and reads it back with kdfread. Surviving is the test. */
static void hostile(const char *what, short leng, const unsigned char *block, size_t n, size_t tail)
{
    static unsigned char out[65536];
    FILE *f = fopen(TMPNAME, "wb");
    int32_t fil;
    size_t i;

    fwrite(&leng, 2, 1, f);
    fwrite(block, 1, n, f);
    for (i = 0; i < tail; i++) fputc(rnd(), f);
    fclose(f);
    fil = kopen4load(TMPNAME, 0);
    kdfread(out, 1, sizeof(out), fil);
    kdfread(out, 4, 4096, fil);
    kclose(fil);
    remove(TMPNAME);
    printf("hostile %s: survived\n", what);
}

static void hostile_blocks(void)
{
    static unsigned char b[40000];
    size_t i;

    for (i = 0; i < sizeof(b); i++) b[i] = rnd();
    hostile("negative length", -2, b, 64, 8 << 20);
    hostile("length past the buffer", 32767, b, 32767, 0);

    b[0] = 100; b[1] = 0; b[2] = 0xff; b[3] = 0x7f;   /* 100 bytes from 32767 strings */
    hostile("too many strings", 4000, b, 4000, 0);

    b[0] = 0x80; b[1] = 0x3f; b[2] = 0; b[3] = 0;     /* uncompressed, says 16256 bytes */
    hostile("uncompressed block past the buffer", 20000, b, 20000, 0);

    memset(b, 0, 64);
    b[0] = 100; b[1] = 0; b[2] = 0x2c; b[3] = 0x01;   /* 300 strings */
    b[5] = 0x01;                                       /* first code 256: itself */
    hostile("code that refers to itself", 64, b, 64, 0);
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

#if !PLATFORM_WIN32
    alarm(20);  /* a decoder caught in a loop fails here instead of hanging */
#endif
    hostile_blocks();

    if (argc > 1)
        check_dos_save(argv[1]);

    if (failures) {
        printf("%d failure(s)\n", failures);
        return 1;
    }
    printf("lzw_roundtrip: ok\n");
    return 0;
}
