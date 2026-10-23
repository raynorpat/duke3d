#include "profile_test_support.h"
/*
 * drawmasks' order of sprites and masked walls (JFBuild 61483f0), headless:
 *  a two-sector map with a slanted masked wall of a solid tile, a sprite
 *  behind it and a farther one in front of it. The one behind is closer,
 *  so it's drawn after the masked wall unless drawmasks finds it behind the
 *  wall first, which needs the wall's screen extent (xb1/xb2 in renderer
 *  0, dxb1/dxb2 in Polymost). Behind a solid wall it must be hidden: the
 *  frame is the same as without it, as renderer 1 shows (renderer 2's
 *  z-buffer hides it whatever the order). Behind a see-through (cstat 128)
 *  one it must show through, drawn before the wall is blended over it, as
 *  renderers 0 and 2 show (drawn after, renderer 2's z-buffer, which the
 *  wall wrote, hides it; renderer 1 draws flat colour, so it's hidden).
 *
 *  usage: render_maskorder <DUKE3D.GRP> <renderer>...
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

/* A tile at least 64x64 with no see-through (255) texels, after skip. */
static short solid_tile(short skip)
{
    short t;
    int32_t i, n;

    for (t = 0; t < MAXTILES; t++)
    {
        if ((t == skip) || (tilesizx[t] < 64) || (tilesizy[t] < 64) || (picanm[t] & 192))
            continue;
        loadtile(t);
        if (waloff[t] == 0)
            continue;
        n = tilesizx[t] * tilesizy[t];
        for (i = 0; i < n; i++)
            if (((unsigned char *) waloff[t])[i] == 255)
                break;
        if (i == n)
            return(t);
    }
    return(-1);
}

static void add_wall(int w, int32_t x, int32_t y, short next, short nextwall, short pic, short over)
{
    memset(&wall[w], 0, sizeof (walltype));
    wall[w].x = x;
    wall[w].y = y;
    wall[w].nextsector = next;
    wall[w].nextwall = nextwall;
    wall[w].picnum = pic;
    wall[w].overpicnum = over;
    wall[w].cstat = (next >= 0) ? 16 : 0;  /* masked */
    wall[w].xrepeat = wall[w].yrepeat = 8;
}

static void add_sector(int s, short first, short n, short pic)
{
    int w;

    memset(&sector[s], 0, sizeof (sectortype));
    sector[s].wallptr = first;
    sector[s].wallnum = n;
    sector[s].ceilingz = -16384;
    sector[s].floorz = 16384;
    sector[s].ceilingpicnum = sector[s].floorpicnum = pic;
    for (w = first; w < first + n; w++)
        wall[w].point2 = (w + 1 < first + n) ? w + 1 : first;
}

static short add_sprite(int32_t x, int32_t y, short pic)
{
    short sect = 0, i;

    updatesector(x, y, &sect);
    i = insertsprite(sect, 0);
    sprite[i].x = x;
    sprite[i].y = y;
    sprite[i].z = sector[sect].floorz;
    sprite[i].cstat = 0;
    sprite[i].picnum = pic;
    sprite[i].xrepeat = sprite[i].yrepeat = 32;
    return(i);
}

/* Draws the view; buf gets the frame. */
static void draw(unsigned char *buf)
{
    int32_t y;

    clearallviews(0);
    drawrooms(0, 0, 0, 0, 100, 0);
    drawmasks();
    nextpage();
    for (y = 0; y < ydim; y++)
        memcpy(buf + y * xdim, (unsigned char *) surface->pixels + y * surface->pitch, xdim);
}

int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    short walltile, spritetile, behind, front;
    unsigned char *with, *without, *neither;
    int32_t i, a, diff, shown, see;
    int fail = 0;

    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <DUKE3D.GRP> <renderer>...\n", argv[0]);
        return 2;
    }
    if (initgroupfile(argv[1]) == -1) { fprintf(stderr, "FAIL: initgroupfile\n"); return 1; }
    initengine();
    if (loadpics("tiles000.art") != 0) { fprintf(stderr, "FAIL: loadpics\n"); return 1; }
    walltile = solid_tile(-1);
    spritetile = solid_tile(walltile);
    if ((walltile < 0) || (spritetile < 0)) { fprintf(stderr, "FAIL: no solid tiles\n"); return 1; }
    printf("masked wall tile %d, sprite tile %d\n", walltile, spritetile);

    /* The viewer at (0,0) looks east. Sector 0 holds it; sector 1 is past
     * the masked wall from (2000,-4000) to (6000,4000). */
    add_wall(0, -2000, -4000, -1, -1, walltile, 0);
    add_wall(1, 2000, -4000, 1, 7, walltile, walltile);
    add_wall(2, 6000, 4000, -1, -1, walltile, 0);
    add_wall(3, -2000, 4000, -1, -1, walltile, 0);
    add_wall(4, 2000, -4000, -1, -1, walltile, 0);
    add_wall(5, 9000, -4000, -1, -1, walltile, 0);
    add_wall(6, 9000, 4000, -1, -1, walltile, 0);
    add_wall(7, 6000, 4000, 0, 1, walltile, walltile);
    add_sector(0, 0, 4, walltile);
    add_sector(1, 4, 4, walltile);
    numwalls = 8;
    numsectors = 2;
    initspritelists();
    behind = add_sprite(4500, -1000, spritetile);  /* 4500 ahead, behind the wall */
    front = add_sprite(5000, 3000, spritetile);    /* 5000 ahead, in front of it */
    if ((sprite[behind].sectnum != 1) || (sprite[front].sectnum != 0))
        { fprintf(stderr, "FAIL: the sprites aren't where they should be\n"); return 1; }

    _platform_init(argc, argv, "render_maskorder", "render");
    if (_setgamemode(1, 320, 200) != 0) { fprintf(stderr, "FAIL: _setgamemode\n"); return 1; }
    setbrightness(0, palette);
    with = malloc(xdim * ydim);
    without = malloc(xdim * ydim);
    neither = malloc(xdim * ydim);

    for (a = 2; a < argc; a++)
        for (see = 0; see < 2; see++)
        {
            setrendermode(atoi(argv[a]));
            wall[1].cstat = wall[7].cstat = see ? 16|128 : 16;
            sprite[behind].cstat = sprite[front].cstat = (short) 32768;
            draw(neither);
            sprite[front].cstat = 0;
            draw(without);
            sprite[behind].cstat = 0;
            draw(with);
            for (i = 0, shown = 0, diff = 0; i < xdim * ydim; i++)
            {
                shown += (without[i] != neither[i]);
                diff += (with[i] != without[i]);
            }
            printf("renderer %d, %s wall: the front sprite covers %d pixels; the one behind the wall shows in %d\n",
                   getrendermode(), see ? "see-through" : "solid", (int) shown, (int) diff);
            if (shown == 0)
            {
                fprintf(stderr, "FAIL: renderer %d doesn't draw the sprite in front\n", getrendermode());
                fail = 1;
            }
            if ((diff != 0) && (!see || (getrendermode() == 1)))
            {
                fprintf(stderr, "FAIL: renderer %d draws the sprite behind the masked wall over it\n", getrendermode());
                fail = 1;
            }
            if ((diff == 0) && see && (getrendermode() != 1))
            {
                fprintf(stderr, "FAIL: renderer %d hides the sprite behind the see-through wall\n", getrendermode());
                fail = 1;
            }
        }
    uninitengine();
    return(fail);
}
