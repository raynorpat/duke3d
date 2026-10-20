/*
 * Renders E1L1's start view in renderer 3 (OpenGL) in a real window, with
 *  a HUD tile, printext256 text, a drawline256 line and a plotpixel, and
 *  writes, for checking by eye:
 *   <dir>/gl0000.png   screencapture of that frame (PNG)
 *   <dir>/fade.bmp     the screen after setpalettefade(63,0,0,32) over a
 *                      frame that isn't redrawn, as a title screen's
 *   <dir>/unfade.bmp   the screen after setpalettefade(0,0,0,0)
 *   <dir>/still63.bmp, still32.bmp, still00.bmp
 *                      E1's ending screen, drawn once with dastat 128 as
 *                      game.c does, faded in from black: setpalettefade
 *                      (0,0,0,63), then 32, then 0
 *   <dir>/map_npot<n>.bmp
 *                      the E1L1 map view (drawmapview, as overhead_on 2)
 *                      with render_classic2d's translucent floor sprite,
 *                      non-power-of-two textures off/on (<n>)
 *   <dir>/tex_npot<n>_mip<m>.bmp
 *                      the start view again with non-power-of-two textures
 *                      off/on (<n>; on only if the driver has them) and
 *                      gltexmiplevel <m> 0 and 1: the floor underfoot is a
 *                      0x0 tile (it should vanish, not draw grey), and the
 *                      first odd-sized tile is drawn with rotatesprite and
 *                      polymost_drawtilescreen
 *   <dir>/fb_<pal>_npot<n>_<off|on|off2>.bmp
 *                      the start view with its sector made dark (shade 24)
 *                      and given lit tiles (indices 240-254) on its floor,
 *                      walls and a sprite: polymosttexfullbright 256 (off),
 *                      240 (on), then 256 again (off2), in the normal and
 *                      water (<pal>) palettes; normal only for npot 0
 *  It fails on a GL error, a failed capture, a fade that doesn't redden, a
 *  still screen that doesn't brighten as it fades in, a miplevel 1 view
 *  the same as the miplevel 0 one, a 0x0 floor that isn't see-through, a
 *  blank map view, a 3D view drawn after the map view that differs from
 *  the one drawn before it (GL state left behind), a fullbright view that
 *  doesn't brighten, or a fullbright-off view that differs after a
 *  fullbright-on one (glow textures left behind).
 *
 *  usage: render_gl <DUKE3D.GRP> <dir>
 *
 * Returns 77 (skipped) when there's no OpenGL, as under the dummy driver.
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
#include "SDL_opengl.h"

/* Callbacks the game normally provides to the engine. */
void timerhandler(void) { }
void keyhandler(void) { }
void faketimerhandler(void) { }

#define APLAYER 1405  /* names.h: a player start */
#define THREEDEE 2498  /* names.h */
#define FIRSTGUN 2524  /* names.h: the pistol */
#define VICTORY1 3260  /* names.h: E1's ending screen */
#define LITFLOOR 966  /* a poster: the E1L1 wall tile with the most lit texels */
#define LITWALL 931  /* fire, from an E1L1 sprite: lit, and 112 wide */

/* polymost.c, which engine.c includes: the editor's tile picker, and the
 * texture cache reset. */
int32_t polymost_drawtilescreen(int32_t tilex, int32_t tiley, int32_t wallnum, int32_t dimen);
void polymost_glreset(void);
void gltexinvalidateall(void);

/* lookup.dat's water palette, after its palookups. */
static unsigned char waterpal[768];

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
    kread(fp, waterpal, 768);
    kclose(fp);
    palette[765] = palette[766] = palette[767] = 0;
    waterpal[765] = waterpal[766] = waterpal[767] = 0;
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

/* Saves the front buffer (what's on screen, fade included); returns its
 *  mean red and green. */
static void save_front(const char *path, double *red, double *green)
{
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormat(0, xdim, ydim, 24, SDL_PIXELFORMAT_RGB24);
    unsigned char *tmp = malloc(xdim * 3);
    unsigned char *a, *b;
    long i, n = (long) xdim * ydim;
    int y;

    glReadBuffer(GL_FRONT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, xdim, ydim, GL_RGB, GL_UNSIGNED_BYTE, s->pixels);
    glReadBuffer(GL_BACK);

    *red = *green = 0;
    for (i = 0; i < n; i++)
    {
        *red += ((unsigned char *) s->pixels)[i*3+0];
        *green += ((unsigned char *) s->pixels)[i*3+1];
    }
    *red /= n;
    *green /= n;

    for (y = 0; y < ydim / 2; y++)  /* GL rows are bottom-up. */
    {
        a = (unsigned char *) s->pixels + y * s->pitch;
        b = (unsigned char *) s->pixels + (ydim - 1 - y) * s->pitch;
        memcpy(tmp, a, xdim * 3); memcpy(a, b, xdim * 3); memcpy(b, tmp, xdim * 3);
    }
    SDL_SaveBMP(s, path);
    SDL_FreeSurface(s);
    free(tmp);
}

/* Reads the front buffer as RGB into buf (xdim*ydim*3 bytes). */
static void read_front(unsigned char *buf)
{
    glReadBuffer(GL_FRONT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, xdim, ydim, GL_RGB, GL_UNSIGNED_BYTE, buf);
    glReadBuffer(GL_BACK);
}

/* Mean absolute difference per channel of two read_front buffers. */
static double front_diff(const unsigned char *a, const unsigned char *b)
{
    long i, n = (long) xdim * ydim * 3;
    double d = 0;
    for (i = 0; i < n; i++) d += abs((int) a[i] - (int) b[i]);
    return d / n;
}

/* Mean of a read_front buffer's bottom-right corner (the bottom 5%, x from
 *  80% to 98%): floor in the start view, black when the floor is gone. */
static double floor_mean(const unsigned char *a)
{
    int x, y;
    double d = 0;
    for (y = 0; y < ydim / 20; y++)
        for (x = xdim * 80 / 100; x < xdim * 98 / 100; x++)
            d += a[(y * xdim + x) * 3] + a[(y * xdim + x) * 3 + 1] + a[(y * xdim + x) * 3 + 2];
    return d / (3.0 * (ydim / 20) * (xdim * 98 / 100 - xdim * 80 / 100));
}

/* Draws the start view with polymosttexfullbright fb, retexturing, and
 *  saves it as <dir>/<name>.bmp; buf gets the frame. */
static void fb_shot(int32_t fb, const char *dir, const char *name, unsigned char *buf,
                    int32_t x, int32_t y, int32_t z, short a, short sect)
{
    char path[1024];
    double rs, gs;

    polymosttexfullbright = fb;
    gltexinvalidateall();
    clearallviews(0);
    drawrooms(x, y, z, a, 100, sect);
    drawmasks();
    nextpage();
    snprintf(path, sizeof (path), "%s/%s.bmp", dir, name);
    save_front(path, &rs, &gs);
    read_front(buf);
}

int main(int argc, char **argv)
{
    int32_t posx, posy, posz;
    short ang, sectnum;
    char path[1024];
    double r0, g0, r1, g1;
    static const int stillfade[3] = { 63, 32, 0 };
    double still[3], rs, gs;
    GLenum err;
    int32_t i;
    int npot, hasnpot, mip, zerotile = -1, oddtile = -1;
    unsigned char *shot[2][2], *view3d[2], *map[2], *fb[2][2][3];
    double mapmean[2];
    short floorpic;
    int pal;
    static char litsect[MAXSECTORS];
    int32_t litx, lity;
    short litsectnum;
    char name[64];
    static const char *palname[2] = { "normal", "water" };
    static const char *fbname[3] = { "off", "on", "off2" };
    static const int32_t fbval[3] = { 256, 240, 256 };

    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <DUKE3D.GRP> <dir>\n", argv[0]);
        return 2;
    }

    if (initgroupfile(argv[1]) == -1) { fprintf(stderr, "FAIL: initgroupfile\n"); return 1; }
    initengine();
    if (loadpics("tiles000.art") != 0) { fprintf(stderr, "FAIL: loadpics\n"); return 1; }
    if (loadboard("E1L1.MAP", &posx, &posy, &posz, &ang, &sectnum) == -1)
        { fprintf(stderr, "FAIL: loadboard(E1L1.MAP)\n"); return 1; }

    /* Like game.c spawn(): hide the editor-only marker sprites and players. */
    for (i = 0; i < MAXSPRITES; i++)
        if ((sprite[i].statnum < MAXSTATUS) &&
            (((sprite[i].picnum >= 1) && (sprite[i].picnum <= 10)) ||
             (sprite[i].picnum == APLAYER)))
            sprite[i].cstat |= 32768;
    if (!load_lookups()) { fprintf(stderr, "FAIL: lookup.dat\n"); return 1; }
    setup_la_backdrop();

    _platform_init(argc, argv, "render_gl", "render");
    if (_setgamemode(1, 320, 200) != 0) { fprintf(stderr, "FAIL: _setgamemode(320x200)\n"); return 1; }
    if (setgamemode_gl(1) != 0)
    {
        printf("SKIP: no OpenGL (%s)\n", SDL_GetError());
        return 77;
    }
    printf("OpenGL mode %dx%d, renderer %d\n", (int) xdim, (int) ydim, getrendermode());
    setbrightness(0, palette);

    clearallviews(0);
    drawrooms(posx, posy, posz, ang, 100, sectnum);
    drawmasks();
    rotatesprite(160<<16, 130<<16, 65536, 0, FIRSTGUN, 0, 0, 2, 0, 0, xdim-1, ydim-1);
    printext256(16, 16, 31, 0, "RENDERER 3 PRINTEXT256", 0);
    printext256(16, 32, 159, -1, "small font", 1);
    drawline256(16<<12, 48<<12, (xdim-16)<<12, (ydim-16)<<12, 31);
    plotpixel(8, 8, 31);
    nextpage();
    snprintf(path, sizeof (path), "%s/gl0000.png", argv[2]);
    if (screencapture(path, 0) != 0) { fprintf(stderr, "FAIL: screencapture\n"); return 1; }

    setpalettefade(63, 0, 0, 32);
    snprintf(path, sizeof (path), "%s/fade.bmp", argv[2]);
    save_front(path, &r1, &g1);
    setpalettefade(0, 0, 0, 0);
    snprintf(path, sizeof (path), "%s/unfade.bmp", argv[2]);
    save_front(path, &r0, &g0);
    printf("mean red/green: faded %.1f/%.1f, unfaded %.1f/%.1f\n", r1, g1, r0, g0);

    /* game.c's E1 ending: the screen goes up once, then palto fades it in. */
    setpalettefade(0, 0, 0, 63);
    clearview(0);
    rotatesprite(0, 50<<16, 65536, 0, VICTORY1, 0, 0, 2+8+16+64+128, 0, 0, xdim-1, ydim-1);
    nextpage();
    for (i = 0; i < 3; i++)
    {
        setpalettefade(0, 0, 0, stillfade[i]);
        snprintf(path, sizeof (path), "%s/still%02d.bmp", argv[2], stillfade[i]);
        save_front(path, &rs, &gs);
        still[i] = (rs + gs) / 2;
    }
    printf("still screen mean red+green/2: fade 63 %.1f, 32 %.1f, 0 %.1f\n", still[0], still[1], still[2]);
    err = glGetError();

    /* A 0x0 tile, and the first tile with both sides odd and over 32. */
    for (i = 0; i < MAXTILES; i++)
    {
        if ((zerotile < 0) && (tilesizx[i] == 0) && (tilesizy[i] == 0)) zerotile = i;
        if ((oddtile < 0) && (tilesizx[i] > 32) && (tilesizy[i] > 32) &&
            (tilesizx[i] & 1) && (tilesizy[i] & 1)) oddtile = i;
    }
    if ((zerotile < 0) || (oddtile < 0)) { fprintf(stderr, "FAIL: no 0x0 or odd-sized tile\n"); return 1; }
    floorpic = sector[sectnum].floorpicnum;
    sector[sectnum].floorpicnum = zerotile;
    printf("0x0 tile %d on sector %d's floor; odd tile %d is %dx%d\n", zerotile, sectnum,
           oddtile, tilesizx[oddtile], tilesizy[oddtile]);

    hasnpot = glinfo.texnpot;
    for (npot = 0; npot <= hasnpot; npot++)
        for (mip = 0; mip < 2; mip++)
        {
            /* Textures already made keep their size: start again. */
            glinfo.texnpot = npot;
            gltexmiplevel = mip;
            polymost_glreset();
            clearallviews(0);
            drawrooms(posx, posy, posz, ang, 100, sectnum);
            drawmasks();
            rotatesprite(270<<16, 150<<16, 65536/2, 0, oddtile, 0, 0, 2, 0, 0, xdim-1, ydim-1);
            setpolymost2dview();
            polymost_drawtilescreen(8, 8, oddtile, 256);
            polymost_drawtilescreen(160, 8, zerotile, 256);
            nextpage();
            snprintf(path, sizeof (path), "%s/tex_npot%d_mip%d.bmp", argv[2], npot, mip);
            save_front(path, &rs, &gs);
            shot[npot][mip] = malloc((size_t) xdim * ydim * 3);
            read_front(shot[npot][mip]);
            if (err == GL_NO_ERROR) err = glGetError();
        }
    glinfo.texnpot = hasnpot;
    gltexmiplevel = 0;
    for (npot = 0; npot <= hasnpot; npot++)
        printf("npot %d: mean |miplevel 1 - miplevel 0| %.2f; 0x0 floor mean %.1f, %.1f\n", npot,
               front_diff(shot[npot][0], shot[npot][1]),
               floor_mean(shot[npot][0]), floor_mean(shot[npot][1]));
    if (hasnpot)
        printf("miplevel 0: mean |npot 1 - npot 0| %.2f\n", front_diff(shot[0][0], shot[1][0]));
    else
        printf("no GL_ARB_texture_non_power_of_two: npot 1 not drawn\n");

    /* The map view, between two 3D views that must come out the same. */
    sector[sectnum].floorpicnum = floorpic;
    i = insertsprite(sectnum, 0);  /* render_classic2d's translucent floor sprite */
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
    memset(show2dsector, 0xff, sizeof (show2dsector));
    for (i = 0; i < 2; i++)
    {
        polymost_glreset();
        clearallviews(0);
        drawrooms(posx, posy, posz, ang, 100, sectnum);
        drawmasks();
        nextpage();
        view3d[i] = malloc((size_t) xdim * ydim * 3);
        read_front(view3d[i]);
        if (i) break;
        for (npot = 0; npot <= hasnpot; npot++)
        {
            glinfo.texnpot = npot;
            polymost_glreset();
            clearview(0);
            drawmapview(posx, posy, 2048, ang);
            nextpage();
            snprintf(path, sizeof (path), "%s/map_npot%d.bmp", argv[2], npot);
            save_front(path, &rs, &gs);
            mapmean[npot] = (rs + gs) / 2;
            map[npot] = malloc((size_t) xdim * ydim * 3);
            read_front(map[npot]);
            if (err == GL_NO_ERROR) err = glGetError();
        }
        glinfo.texnpot = hasnpot;
    }
    for (npot = 0; npot <= hasnpot; npot++)
        printf("map view npot %d: mean red+green/2 %.1f\n", npot, mapmean[npot]);
    if (hasnpot)
        printf("map view: mean |npot 1 - npot 0| %.2f\n", front_diff(map[0], map[1]));
    printf("3D view: mean |after map - before map| %.2f\n", front_diff(view3d[0], view3d[1]));

    /* Fullbright: E1L1 has few lit (240-254) texels on walls or floors, so the
     * start sector is made a dark room with them: a floor of LITFLOOR, walls
     * (its own and its neighbours') of LITWALL, not a power of two wide (the
     * split wall path without non-power-of-two textures), and a face sprite
     * of LITFLOOR ahead to the right, all at shade 24. Then off, on and off
     * again, at each npot and palette. */
    printf("lit floor %d is %dx%d, lit wall %d is %dx%d\n", LITFLOOR, tilesizx[LITFLOOR],
           tilesizy[LITFLOOR], LITWALL, tilesizx[LITWALL], tilesizy[LITWALL]);
    sector[sectnum].floorpicnum = LITFLOOR;
    sector[sectnum].floorshade = sector[sectnum].ceilingshade = 24;
    litsect[sectnum] = 1;  /* the walls of the start sector and its neighbours */
    for (i = sector[sectnum].wallptr; i < sector[sectnum].wallptr + sector[sectnum].wallnum; i++)
        if (wall[i].nextsector >= 0) litsect[wall[i].nextsector] = 1;
    for (i = 0; i < numwalls; i++)
        if (litsect[sectorofwall(i)]) { wall[i].picnum = LITWALL; wall[i].shade = 24; }
    litx = posx + (sintable[(ang+512+160)&2047]>>3);  /* ahead, to the right */
    lity = posy + (sintable[(ang+160)&2047]>>3);
    litsectnum = sectnum;
    updatesector(litx, lity, &litsectnum);
    if (litsectnum < 0) { fprintf(stderr, "FAIL: the lit sprite is outside the map\n"); return 1; }
    i = insertsprite(litsectnum, 0);
    sprite[i].x = litx;
    sprite[i].y = lity;
    sprite[i].z = sector[litsectnum].floorz;
    sprite[i].cstat = 0;
    sprite[i].picnum = LITFLOOR;
    sprite[i].shade = 24;
    sprite[i].pal = 0;
    sprite[i].xrepeat = sprite[i].yrepeat = 64;
    sprite[i].xoffset = sprite[i].yoffset = 0;
    sprite[i].ang = ang;
    memset(fb, 0, sizeof (fb));
    for (npot = 0; npot <= hasnpot; npot++)
        for (pal = 0; pal < 2; pal++)
        {
            if ((pal == 1) && (npot != hasnpot)) continue;
            glinfo.texnpot = npot;
            polymost_glreset();
            setbrightness(0, pal ? waterpal : palette);
            for (i = 0; i < 3; i++)
            {
                snprintf(name, sizeof (name), "fb_%s_npot%d_%s", palname[pal], npot, fbname[i]);
                fb[npot][pal][i] = malloc((size_t) xdim * ydim * 3);
                fb_shot(fbval[i], argv[2], name, fb[npot][pal][i], posx, posy, posz, ang, sectnum);
                if (err == GL_NO_ERROR) err = glGetError();
            }
            printf("fullbright %s npot %d: mean |on - off| %.2f, |off again - off| %.2f\n",
                   palname[pal], npot, front_diff(fb[npot][pal][1], fb[npot][pal][0]),
                   front_diff(fb[npot][pal][2], fb[npot][pal][0]));
        }
    glinfo.texnpot = hasnpot;
    polymosttexfullbright = 256;

    uninitengine();

    if (err != GL_NO_ERROR) { fprintf(stderr, "FAIL: glGetError 0x%x\n", (unsigned) err); return 1; }
    if ((r1 < r0 + 64) || (g1 > g0 + 1)) { fprintf(stderr, "FAIL: the fade isn't red\n"); return 1; }
    if ((still[1] < still[0] + 8) || (still[2] < still[1] + 8))
        { fprintf(stderr, "FAIL: the still screen doesn't fade in\n"); return 1; }
    for (npot = 0; npot <= hasnpot; npot++)
        if (front_diff(shot[npot][0], shot[npot][1]) == 0)
            { fprintf(stderr, "FAIL: miplevel 1 looks the same as 0 (npot %d)\n", npot); return 1; }
    for (npot = 0; npot <= hasnpot; npot++)
        for (mip = 0; mip < 2; mip++)
            if (floor_mean(shot[npot][mip]) > 32)
                { fprintf(stderr, "FAIL: the 0x0 floor draws (npot %d, miplevel %d)\n", npot, mip); return 1; }
    for (npot = 0; npot <= hasnpot; npot++)
        if (mapmean[npot] < 8)
            { fprintf(stderr, "FAIL: the map view is blank (npot %d)\n", npot); return 1; }
    if (memcmp(view3d[0], view3d[1], (size_t) xdim * ydim * 3) != 0)
        { fprintf(stderr, "FAIL: the 3D view after the map view differs\n"); return 1; }
    for (npot = 0; npot <= hasnpot; npot++)
        for (pal = 0; pal < 2; pal++)
        {
            if (!fb[npot][pal][0]) continue;
            if (front_diff(fb[npot][pal][1], fb[npot][pal][0]) < 0.1)
                { fprintf(stderr, "FAIL: fullbright doesn't brighten (%s, npot %d)\n", palname[pal], npot); return 1; }
            if (memcmp(fb[npot][pal][2], fb[npot][pal][0], (size_t) xdim * ydim * 3) != 0)
                { fprintf(stderr, "FAIL: fullbright off differs after on (%s, npot %d)\n", palname[pal], npot); return 1; }
        }
    printf("PASS: render_gl\n");
    return 0;
}
