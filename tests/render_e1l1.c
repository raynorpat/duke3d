/*
 * Renders the start view of E1L1 from a real DUKE3D.GRP, headless, and
 *  checks the frame against a known checksum.
 *
 *  usage: render_e1l1 <DUKE3D.GRP> <out.bmp> [expected checksum, hex] [renderer] [tile]
 *
 * Without an expected checksum (or with an empty one) it just prints the
 *  checksum and exits 0. The renderer is a setrendermode() number, 0 if
 *  not given. With "tile", the view goes through a tile 256 tall, as in
 *  game.c's low detail mode, and must look like renderer 0's.
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

#define LA 89  /* names.h: the Los Angeles skyline sky */
#define APLAYER 1405  /* names.h: a player start */

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

/* Like premap.c setupbackdrop(LA), with parallaxtype 0 as in resetplayerstats. */
static void setup_la_backdrop(void)
{
    int i;
    for (i = 0; i < MAXPSKYTILES; i++) pskyoff[i] = 0;
    parallaxtype = 0;
    parallaxyscale = 16384+1024;
    pskyoff[0]=1; pskyoff[1]=2; pskyoff[2]=1; pskyoff[3]=3;
    pskyoff[4]=4; pskyoff[5]=0; pskyoff[6]=2; pskyoff[7]=3;
    pskybits = 3;
}

/*
 * game.c's low detail and screen tilt: the view drawn into a tile whose
 *  height (the screen's width) is a power of two, put on screen turned 90
 *  degrees with rotatesprite.
 */
#define VIEWTILE (MAXTILES-2)
static void draw_through_tile(int32_t x, int32_t y, int32_t z, short a, short sect)
{
    static unsigned char tilebuf[100*256];

    walock[VIEWTILE] = 255;
    waloff[VIEWTILE] = (intptr_t) tilebuf;
    setviewtotile(VIEWTILE, 100, 256);
    drawrooms(x, y, z, a, 100, sect);
    drawmasks();
    setviewback();
    picanm[VIEWTILE] &= 0xff0000ff;
    rotatesprite(160<<16, 100<<16, 131072, 512, VIEWTILE, 0, 0, 4+2+64,
                 windowx1, windowy1, windowx2, windowy2);
}

int main(int argc, char **argv)
{
    int32_t posx, posy, posz;
    short ang, sectnum;
    uint32_t hash = 2166136261u;
    const unsigned char *pix;
    int32_t i, same;

    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <DUKE3D.GRP> <out.bmp> [expected checksum] [renderer]\n", argv[0]);
        return 2;
    }

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
    printf("start: x=%d y=%d z=%d ang=%d sect=%d\n",
           (int) posx, (int) posy, (int) posz, (int) ang, (int) sectnum);

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
    setup_la_backdrop();

    _platform_init(argc, argv, "render_e1l1", "render");
    if (_setgamemode(1, 320, 200) != 0)
    {
        fprintf(stderr, "FAIL: _setgamemode(320x200)\n");
        return 1;
    }
    setbrightness(0, palette);
    if ((argc > 5) && (strcmp(argv[5], "tile") == 0))
    {
        /* through a tile: the renderer's picture against renderer 0's. */
        unsigned char *classic = (unsigned char *) malloc(xdim * ydim);

        draw_through_tile(posx, posy, posz, ang, sectnum);
        memcpy(classic, (void *) frameplace, xdim * ydim);
        setrendermode(atoi(argv[4]));
        clearview(0);
        draw_through_tile(posx, posy, posz, ang, sectnum);
        nextpage();
        for (i = 0, same = 0; i < xdim * ydim; i++)
            same += (classic[i] == ((unsigned char *) surface->pixels)[i]);
        printf("through a tile: %d%% of pixels as in renderer 0\n", (int) (same * 100 / (xdim * ydim)));
        free(classic);
        /* Renderer 2 samples the tile a little differently (58% the same);
         * a tile addressed with the wrong power of two is garbage (16%). */
        if (same * 100 < xdim * ydim * 40)
        {
            fprintf(stderr, "FAIL: the view through a tile isn't renderer 0's\n");
            return 1;
        }
    }
    else
    {
        if (argc > 4)
            setrendermode(atoi(argv[4]));
        drawrooms(posx, posy, posz, ang, 100, sectnum);
        drawmasks();
        nextpage();
    }

    if (SDL_SaveBMP(surface, argv[2]) != 0)
    {
        fprintf(stderr, "FAIL: SDL_SaveBMP(%s): %s\n", argv[2], SDL_GetError());
        return 1;
    }

    /* FNV-1a over the 8-bit frame. */
    pix = (const unsigned char *) surface->pixels;
    for (i = 0; i < xdim * ydim; i++)
    {
        hash ^= pix[i];
        hash *= 16777619u;
    }
    printf("checksum: %08x\n", (unsigned) hash);

    uninitengine();

    if ((argc > 3) && argv[3][0] && (strtoul(argv[3], NULL, 16) != hash))
    {
        fprintf(stderr, "FAIL: checksum %08x, expected %s\n", (unsigned) hash, argv[3]);
        return 1;
    }
    return 0;
}
