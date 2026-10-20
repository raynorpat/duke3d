/*
 * Draws HUD rotatesprites and the textured map view of E1L1 with the
 *  classic renderer at a given 8-bit mode, headless, and checks both frames
 *  against known checksums.
 *
 *  usage: render_classic2d <DUKE3D.GRP> <out-prefix> <W> <H> [expected-hud] [expected-map]
 *
 * Writes <out-prefix>_hud.bmp and <out-prefix>_map.bmp. Without an expected
 *  checksum (or with an empty one) it just prints it. When either is given,
 *  the mode must come out at exactly W x H.
 *  Run with SDL_VIDEODRIVER=dummy.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "build.h"
#include "display.h"
#include "engine.h"
#include "cache1d.h"
#include "SDL.h"

extern SDL_Surface *surface;  /* sdl_driver.c */

/* Callbacks the game normally provides to the engine. */
void timerhandler(void) { }
void keyhandler(void) { }
void faketimerhandler(void) { }

/* names.h */
#define APLAYER 1405
#define BOTTOMSTATUSBAR 2462
#define DIGITALNUM 2472
#define THREEDEE 2498
#define CROSSHAIR 2523
#define FIRSTGUN 2524

/* Like premap.c genspriteremaps(): the palette swaps from lookup.dat. */
static int load_lookups(void)
{
    char remap[256];
    unsigned char numl = 0;
    signed char look_pos;
    int32_t fp, j;

    if ((fp = kopen4load("lookup.dat", 0)) == -1)
        return(0);
    kread(fp, &numl, 1);
    for (j = 0; j < numl; j++)
    {
        kread(fp, &look_pos, 1);
        kread(fp, remap, 256);
        makepalookup((int32_t) look_pos, remap, 0, 0, 0, 1);
    }
    kclose(fp);
    palette[765] = palette[766] = palette[767] = 0;
    return(1);
}

/* Animated tiles would make the frame depend on the clock. */
static int still_tile(short picnum)
{
    if (picanm[picnum] & 192)
    {
        fprintf(stderr, "FAIL: tile %d is animated\n", (int) picnum);
        return(0);
    }
    return(1);
}

/* Saves the frame and returns its FNV-1a checksum. */
static uint32_t frame(const char *prefix, const char *name)
{
    char path[1024];
    uint32_t hash = 2166136261u;
    const unsigned char *pix;
    int32_t i;

    nextpage();
    snprintf(path, sizeof (path), "%s_%s.bmp", prefix, name);
    if (SDL_SaveBMP(surface, path) != 0)
        fprintf(stderr, "WARN: SDL_SaveBMP(%s): %s\n", path, SDL_GetError());
    pix = (const unsigned char *) surface->pixels;
    for (i = 0; i < xdim * ydim; i++)
    {
        hash ^= pix[i];
        hash *= 16777619u;
    }
    return(hash);
}

/* The HUD sprites with dastat as given, in the setview window or full screen. */
static void draw_hud(int32_t x, int32_t y, char stat)
{
    int32_t cx1 = windowx1, cy1 = windowy1, cx2 = windowx2, cy2 = windowy2;

    if (stat & 8)
        cx1 = 0, cy1 = 0, cx2 = xdim-1, cy2 = ydim-1;
    rotatesprite(x<<16, y<<16, 16384L, 0, FIRSTGUN, 0, 0, stat, cx1, cy1, cx2, cy2);
    rotatesprite((x+40)<<16, (y+10)<<16, 32768L, 256, CROSSHAIR, 8, 0, stat, cx1, cy1, cx2, cy2);
    rotatesprite((x+60)<<16, (y+30)<<16, 65536L, 0, DIGITALNUM+3, 0, 2, stat, cx1, cy1, cx2, cy2);
}

int main(int argc, char **argv)
{
    int32_t posx, posy, posz, w, h;
    short ang, sectnum;
    char stat;
    uint32_t hud, map;
    int32_t i;
    int fail = 0, check;

    if (argc < 5)
    {
        fprintf(stderr, "usage: %s <DUKE3D.GRP> <out-prefix> <W> <H> [expected-hud] [expected-map]\n", argv[0]);
        return 2;
    }
    w = atoi(argv[3]);
    h = atoi(argv[4]);
    check = ((argc > 5) && argv[5][0]) || ((argc > 6) && argv[6][0]);

    if (initgroupfile(argv[1]) == -1)
    {
        fprintf(stderr, "FAIL: initgroupfile(%s)\n", argv[1]);
        return 1;
    }
    initengine();
    if (palookup[0] == NULL)
    {
        fprintf(stderr, "FAIL: palette.dat not loaded\n");
        return 1;
    }
    if (loadpics("tiles000.art") != 0)
    {
        fprintf(stderr, "FAIL: loadpics\n");
        return 1;
    }
    if (loadboard("E1L1.MAP", &posx, &posy, &posz, &ang, &sectnum) == -1)
    {
        fprintf(stderr, "FAIL: loadboard(E1L1.MAP)\n");
        return 1;
    }

    /* Like game.c spawn(): hide the editor-only marker sprites and players. */
    for (i = 0; i < MAXSPRITES; i++)
        if ((sprite[i].statnum < MAXSTATUS) &&
            (((sprite[i].picnum >= 1) && (sprite[i].picnum <= 10)) ||
             (sprite[i].picnum == APLAYER)))
            sprite[i].cstat |= 32768;
    if (!load_lookups())
    {
        fprintf(stderr, "FAIL: lookup.dat\n");
        return 1;
    }
    if (!still_tile(BOTTOMSTATUSBAR) || !still_tile(FIRSTGUN) ||
        !still_tile(CROSSHAIR) || !still_tile(DIGITALNUM+3) || !still_tile(THREEDEE))
        return 1;

    /* A translucent floor sprite just ahead of the player start. */
    i = insertsprite(sectnum, 0);
    sprite[i].x = posx + (sintable[(ang+512)&2047]>>6);
    sprite[i].y = posy + (sintable[ang&2047]>>6);
    sprite[i].z = sector[sectnum].floorz;
    sprite[i].cstat = 32|2;
    sprite[i].picnum = THREEDEE;
    sprite[i].shade = 0;
    sprite[i].pal = 0;
    sprite[i].xrepeat = sprite[i].yrepeat = 32;
    sprite[i].xoffset = sprite[i].yoffset = 0;
    sprite[i].ang = ang;

    _platform_init(argc, argv, "render_classic2d", "render");
    if (_setgamemode(1, w, h) != 0)
    {
        fprintf(stderr, "FAIL: _setgamemode(%dx%d)\n", (int) w, (int) h);
        return 1;
    }
    printf("mode: %dx%d (asked for %dx%d)\n", (int) xdim, (int) ydim, (int) w, (int) h);
    if (check && ((xdim != w) || (ydim != h)))
    {
        fprintf(stderr, "FAIL: got %dx%d, not %dx%d\n", (int) xdim, (int) ydim, (int) w, (int) h);
        return 1;
    }
    setbrightness(0, palette);

    /* HUD: a smaller setview window, as with a reduced screen size, so
     *  scaling to the window (2) and to the full screen (2|8) differ.
     *  Each dastat is passed as a literal and through a char variable. */
    setview(0, 0, xdim-1, ydim-1);
    clearview(0L);
    setview(xdim>>3, ydim>>3, xdim-1-(xdim>>3), ydim-1-(ydim>>2));
    clearview(16L);
    rotatesprite(0, (200-34)<<16, 65536L, 0, BOTTOMSTATUSBAR, 4, 0, 2|8|16|64,
                 0, 0, xdim-1, ydim-1);

    draw_hud(10, 10, 2);
    stat = 2; draw_hud(10, 60, stat);
    draw_hud(90, 10, 2|8);
    stat = 2|8; draw_hud(90, 60, stat);
    draw_hud(170, 10, 2|8|16|64|128);
    stat = 2|8|16|64|128; draw_hud(170, 60, stat);
    draw_hud(250, 10, 2|4|64);
    stat = 2|4|64; draw_hud(250, 60, stat);
    hud = frame(argv[2], "hud");

    /* Map view: full screen, every sector shown, at the player start. */
    setview(0, 0, xdim-1, ydim-1);
    clearview(0L);
    memset(show2dsector, 0xff, sizeof (show2dsector));
    drawmapview(posx, posy, 2048, ang);
    map = frame(argv[2], "map");

    printf("hud=%08x\nmap=%08x\n", (unsigned) hud, (unsigned) map);
    uninitengine();

    if ((argc > 5) && argv[5][0] && (strtoul(argv[5], NULL, 16) != hud))
    {
        fprintf(stderr, "FAIL: hud checksum %08x, expected %s\n", (unsigned) hud, argv[5]);
        fail = 1;
    }
    if ((argc > 6) && argv[6][0] && (strtoul(argv[6], NULL, 16) != map))
    {
        fprintf(stderr, "FAIL: map checksum %08x, expected %s\n", (unsigned) map, argv[6]);
        fail = 1;
    }
    return(fail);
}
