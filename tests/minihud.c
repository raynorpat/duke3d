/*
 * Draws the small full-screen HUD (screen_size 4: health, ammo and the
 *  inventory box) with game.c's coolgaugetext, in the classic renderer at a
 *  given 8-bit mode, headless.
 *
 *  usage: minihud <DUKE3D.GRP> <out-prefix> <W> <H> [expected-plain [expected-inv]]
 *
 * Writes <out-prefix>_plain.bmp and <out-prefix>_inv.bmp (without and with
 *  an inventory item) and checks where the HUD's pixels are. In a
 *  widescreen mode health must be at the left edge of the screen and ammo
 *  and inventory at the right edge, with nothing between; in a 4:3 or tall
 *  mode everything stays in the left half. The frames' checksums are
 *  checked when given. Run with SDL_VIDEODRIVER=dummy.
 */
#define SDL_MAIN_HANDLED  /* game.c's main() is renamed; this one is ours. */
#include "duke3d.h"
#include "SDL.h"

extern SDL_Surface *surface;  /* sdl_driver.c */

#define BGA 0
#define BGB 31

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

/* Draws the HUD over colour bg. */
static void hud_on(int bg, int inv)
{
    setview(0, 0, xdim-1, ydim-1);
    clearview(bg);
    ps[0].inven_icon = inv;
    ps[0].firstaid_amount = 50;
    ps[0].last_extra = 100;
    ps[0].curr_weapon = PISTOL_WEAPON;
    ps[0].ammo_amount[PISTOL_WEAPON] = 48;
    pus = 1;
    coolgaugetext(0);
}

static const unsigned char *row(int32_t y)
{
    return (const unsigned char *) surface->pixels + y * surface->pitch;
}

/* Draws the HUD, saves the frame and returns its checksum. The columns of
 *  the HUD's pixels (those that change with the background) run from *minx
 *  to *maxx, and the widest gap between two of them is *gapa..*gapb. */
static uint32_t shot(const char *prefix, const char *name, int inv,
                     int32_t *minx, int32_t *maxx, int32_t *gapa, int32_t *gapb)
{
    char path[1024];
    uint32_t hash = 2166136261u;
    unsigned char *bgb;
    char *cols;
    int32_t x, y, last;

    cols = (char *) calloc(xdim, 1);
    bgb = (unsigned char *) malloc((size_t) xdim * ydim);
    hud_on(BGB, inv);
    nextpage();
    for (y = 0; y < ydim; y++) memcpy(bgb + (size_t) y * xdim, row(y), xdim);
    hud_on(BGA, inv);
    nextpage();
    snprintf(path, sizeof (path), "%s_%s.bmp", prefix, name);
    if (SDL_SaveBMP(surface, path) != 0)
        fprintf(stderr, "WARN: SDL_SaveBMP(%s): %s\n", path, SDL_GetError());
    for (y = 0; y < ydim; y++)
        for (x = 0; x < xdim; x++)
        {
            hash ^= row(y)[x];
            hash *= 16777619u;
            if (row(y)[x] == BGA && bgb[(size_t) y * xdim + x] == BGB)
                continue;  /* the background showing through */
            cols[x] = 1;
        }
    *minx = xdim; *maxx = -1; *gapa = *gapb = 0; last = -1;
    for (x = 0; x < xdim; x++)
    {
        if (!cols[x]) continue;
        if (x < *minx) *minx = x;
        *maxx = x;
        if ((last >= 0) && (x-last > *gapb-*gapa)) { *gapa = last; *gapb = x; }
        last = x;
    }
    free(cols); free(bgb);
    return(hash);
}

int main(int argc, char **argv)
{
    int32_t w, h, minx[2], maxx[2], gapa[2], gapb[2], i, edge;
    uint32_t sum[2];
    const char *name[2] = { "plain", "inv" };
    int fail = 0;

    if (argc < 5)
    {
        fprintf(stderr, "usage: %s <DUKE3D.GRP> <out-prefix> <W> <H> [expected-plain [expected-inv]]\n", argv[0]);
        return 2;
    }
    w = atoi(argv[3]);
    h = atoi(argv[4]);

    if (initgroupfile(argv[1]) == -1)
    {
        fprintf(stderr, "FAIL: initgroupfile(%s)\n", argv[1]);
        return 1;
    }
    initengine();
    if ((palookup[0] == NULL) || (loadpics("tiles000.art") != 0) || !load_lookups())
    {
        fprintf(stderr, "FAIL: palette, tiles or lookup.dat\n");
        return 1;
    }

    _platform_init(argc, argv, "minihud", "minihud");
    if (_setgamemode(1, w, h) != 0)
    {
        fprintf(stderr, "FAIL: _setgamemode(%dx%d)\n", (int) w, (int) h);
        return 1;
    }
    if ((xdim != w) || (ydim != h))
    {
        fprintf(stderr, "FAIL: got %dx%d, not %dx%d\n", (int) xdim, (int) ydim, (int) w, (int) h);
        return 1;
    }
    setbrightness(0, palette);
    printf("mode: %dx%d, widescreen %d\n", (int) xdim, (int) ydim, (int) widescreen);

    myconnectindex = screenpeek = connecthead = 0;
    connectpoint2[0] = -1;
    ud.screen_size = 4;
    ud.multimode = 1;
    ud.coop = 0;
    ps[0].gm = MODE_GAME;
    ps[0].i = 0;
    sprite[0].pal = 0;
    current_menu = 0;

    edge = ydim/20;  /* the HUD's own 5 of 240 rows margin is under this */
    for (i = 0; i < 2; i++)
    {
        sum[i] = shot(argv[2], name[i], i, &minx[i], &maxx[i], &gapa[i], &gapb[i]);
        printf("%s=%08x cols %d..%d gap %d..%d\n", name[i], (unsigned) sum[i],
               (int) minx[i], (int) maxx[i], (int) gapa[i], (int) gapb[i]);
        if (maxx[i] < 0)
            { fprintf(stderr, "FAIL: %s: nothing drawn\n", name[i]); fail = 1; continue; }
        if (widescreen)
        {
            if (minx[i] > edge)
                { fprintf(stderr, "FAIL: %s: health starts at column %d, not the left edge\n", name[i], (int) minx[i]); fail = 1; }
            if (maxx[i] < xdim-1-edge)
                { fprintf(stderr, "FAIL: %s: HUD ends at column %d, not the right edge\n", name[i], (int) maxx[i]); fail = 1; }
            if ((gapa[i] > xdim/4) || (gapb[i] < xdim*3/4))
                { fprintf(stderr, "FAIL: %s: health and ammo not at opposite edges (gap %d..%d)\n", name[i], (int) gapa[i], (int) gapb[i]); fail = 1; }
        }
        else if (maxx[i] >= xdim/2)
            { fprintf(stderr, "FAIL: %s: HUD reaches column %d, past the left half\n", name[i], (int) maxx[i]); fail = 1; }
    }

    uninitengine();
    for (i = 0; i < 2; i++)
        if ((argc > 5+i) && argv[5+i][0] && (strtoul(argv[5+i], NULL, 16) != sum[i]))
        {
            fprintf(stderr, "FAIL: %s checksum %08x, expected %s\n", name[i], (unsigned) sum[i], argv[5+i]);
            fail = 1;
        }
    return(fail);
}
