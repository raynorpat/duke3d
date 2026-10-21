/*
 * Renders E1L1's start view in renderer 3 (OpenGL) in a real window, with
 *  a HUD tile, printext256 text, a drawline256 line and a plotpixel, and
 *  writes, for checking by eye:
 *   <dir>/gl0000.png   screencapture of that frame (PNG)
 *   <dir>/fade.bmp     the screen after setpalettefade(63,0,0,32) over a
 *                      frame that isn't redrawn, as a title screen's
 *   <dir>/capfade0000.bmp
 *                      screencapture of the faded screen (it has the fade)
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
 *   <dir>/fb_<neither|masked|translucent>_<off|on>.bmp
 *                      the start sector's walls and floor unlit, with
 *                      neither, its two-sided walls masked with a lit
 *                      tile, or the lit sprite translucent; fullbright off
 *                      and on
 *   <dir>/resized.bmp  the last frame shown again after a window resize
 *   <dir>/cmp_<view>_<r0|immediate|vbo|shader>.bmp
 *                      E1L1 viewpoints at horiz 100, 640x480, in renderer 0
 *                      and in renderer 3 on the immediate, vbo and shader paths;
 *                      views "corpse" and "pit" are dead troopers by walls
 *   <dir>/shframe_<hud|water|fade|camera>_<shader|r0>.bmp
 *                      the start view with the HUD's gun, the status bar,
 *                      menu letters, text, a line and a pixel (in the water
 *                      palette; faded to red; with a camera's picture over
 *                      it) on the shader path and in renderer 0, the same size
 *  It fails on a GL error, a failed capture, a fade that doesn't redden or
 *  a screenshot without it, a still screen that doesn't brighten as it
 *  fades in, a clearview outside the view window, a miplevel 1 view
 *  the same as the miplevel 0 one, a 0x0 floor that isn't see-through, a
 *  blank map view, a map view whose cstat 512 floor sprite looks as cstat
 *  2, a 2D picture drawn after the map view that isn't on top of it, a 3D
 *  view drawn after the map view that differs from the one drawn before it
 *  (GL state left behind), a fullbright view that doesn't brighten (a
 *  masked wall and a translucent sprite too), a fullbright-off view that
 *  differs after a fullbright-on one (glow textures left behind), the same
 *  palette again retexturing (seen by changing a tile's art in between),
 *  glsetgamepalette (render_gl_game.c) losing the fade or fullbright or
 *  taking the wrong player's palette, a last frame lost in a resize, or a
 *  mode over the 8-bit 1600x1200 limit whose window isn't the size asked
 *  for, or a draw path (immediate, vertex arrays, vbo) that draws any of a
 *  set of frames differently from immediate mode or doesn't send its
 *  vertices its own way, or renderer 3 further from renderer 0 than pinned
 *  (each viewpoint's mean difference, and the mean of two regions that
 *  renderer 0 darkens by distance), or a shader path (GL 2.1: palette
 *  indices) frame with a colour that isn't the palette's, whose translucent
 *  surfaces (a web and glass, translucent sprites at both levels, the
 *  crosshair and translucent HUD tiles, the map view's floor sprites) are
 *  further than 8 from renderer 0's over their boxes, whose frames change
 *  when every draw is flushed or when all the frame is copied for every
 *  translucent draw, or that loses more than a tenth of a dead trooper
 *  lying against a wall (its picture reaching through the wall, as classic
 *  draws it), draws more than a tenth more of it or of one in a pit than
 *  classic (over a live trooper in front, over the pit's rim) or, on the
 *  shader path, shades it further than 7.5 from classic, or whose map view
 *  fills translucent a sector that is translucent in 3D (floorstat 256,
 *  384), whose 2D frames are far from immediate mode's, whose view smaller than the screen isn't where vbo draws it or
 *  loses the border, whose frame doesn't fade or capture, or that doesn't fall back to
 *  vbo, drawing as vbo does, when its shader or framebuffer fails; or whose
 *  frames with the HUD, status bar, menu letters, text, the water palette, the map view,
 *  a fade or a camera are further than 8 from renderer 0's or not all the
 *  palette's colours, whose screenshots aren't the screen, that makes RGB
 *  textures or retextures (palette changes, fades, glsetgamepalette,
 *  precaching), whose holes in the level don't show what was cleared to,
 *  whose non-masked tiles don't draw index 255, or whose frame, shown or
 *  being drawn, a window resize loses; or that at an internal size (320x200)
 *  doesn't draw at it, is further than 8 from renderer 0's at 320x200,
 *  isn't shown scaled to fit with black bars (after a resize too), or
 *  doesn't validate the size or ignore it off the shader path; or whose
 *  contrast and saturation aren't gl_colorcorrect's on the frame shown and
 *  its screenshots, or change the frame at 1; or, once OpenGL is set, whose
 *  8-bit modes aren't shown through its context as the SDL renderer showed
 *  them before it (palette, fade, brightness, black bars), or that drops or
 *  remakes the window or context between renderers 0 and 3.
 *
 *  usage: render_gl <DUKE3D.GRP> <dir>
 *
 * Returns 77 (skipped) when there's no OpenGL, as under the dummy driver.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "platform.h"
#include "build.h"
#include "display.h"
#include "engine.h"
#include "cache1d.h"
#include "SDL.h"
#include "SDL_opengl.h"
#include "glext.h"
#include "glsubmit.h"

/* The game is linked in (for render_gl_game.c), so it provides the
 * engine's callbacks. */
int glgamepalette_check(void);  /* render_gl_game.c */
void videoswitch_setup(void);  /* render_gl_game.c */
int videoswitch_palette_check(void);  /* render_gl_game.c */
int video_setrenderer(int r);  /* game.c */
int video_setmode(int displaymode, int w, int h);  /* game.c */
void video_requestrenderer(int r);  /* game.c */
void video_requestmode(int displaymode, int w, int h);  /* game.c */
void video_getmode(int32_t *w, int32_t *h);  /* game.c */
void video_applypending(void);  /* game.c */
extern int32_t GLDrawPath;  /* config.c */
extern int32_t video_glunavailable;  /* game.c */
extern SDL_Surface *surface;  /* sdl_driver.c: the 8-bit frame */

#define APLAYER 1405  /* names.h: a player start */
#define THREEDEE 2498  /* names.h */
#define FIRSTGUN 2524  /* names.h: the pistol */
#define VICTORY1 3260  /* names.h: E1's ending screen */
#define BOTTOMSTATUSBAR 2462  /* names.h */
#define STARTALPHANUM 2822  /* names.h: the menu font, from '!' */
#define LITFLOOR 966  /* a poster: the E1L1 wall tile with the most lit texels */
#define FIRE 2271  /* names.h */
#define FIRE2 2311  /* names.h */
#define LITWALL 931  /* fire, from an E1L1 sprite: lit, and 112 wide */
#define EXPLOSION2 1890  /* names.h */
#define SMALLSMOKE 2329  /* names.h */
#define CROSSHAIR 2523  /* names.h */
#define LIZTROOP 1680  /* names.h */
#define TROOPDEAD 1734  /* LIZTROOP's frame 54: GAME.CON's ATROOPDEAD */

/* polymost.c, which engine.c includes: the editor's tile picker, and the
 * texture cache reset. */
int32_t polymost_drawtilescreen(int32_t tilex, int32_t tiley, int32_t wallnum, int32_t dimen);
void polymost_glreset(void);
void gltexinvalidateall(void);
void getfadedpalette(palette_t *pal);  /* engine.c: the palette the shader path shows */
unsigned char *polymost_capture_shader(void);  /* the shader path's frame, RGB, top-down */

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

/* The mean red and green of a 24-bit BMP (0 if it can't be read). */
static void bmp_mean(const char *path, double *red, double *green)
{
    SDL_Surface *b = SDL_LoadBMP(path);
    SDL_Surface *s = (b != NULL) ? SDL_ConvertSurfaceFormat(b, SDL_PIXELFORMAT_RGB24, 0) : NULL;
    long n;
    int x, y;
    const unsigned char *p;

    *red = *green = 0;
    if (s != NULL)
    {
        for (y = 0; y < s->h; y++)
            for (x = 0, p = (const unsigned char *) s->pixels + y * s->pitch; x < s->w; x++, p += 3)
                { *red += p[0]; *green += p[1]; }
        n = (long) s->w * s->h;
        *red /= n;
        *green /= n;
    }
    SDL_FreeSurface(s);
    SDL_FreeSurface(b);
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

/* Mean absolute difference per channel of a read_front buffer (xdim by
 *  ydim) and a smaller one (w by h) stretched to its size, nearest texel. */
static double stretch_diff(const unsigned char *small, int w, int h, const unsigned char *big)
{
    long x, y, i, d = 0;
    for (y = 0; y < ydim; y++)
        for (x = 0; x < xdim; x++)
            for (i = 0; i < 3; i++)
                d += abs((int) big[(y * xdim + x) * 3 + i] -
                         (int) small[((y * h / ydim) * w + x * w / xdim) * 3 + i]);
    return (double) d / ((double) xdim * ydim * 3);
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

/* A tile at least 64x64 with no lit (240-254) texels, not animated. */
static short unlit_tile(void)
{
    short t;
    int32_t i, n;

    for (t = 0; t < MAXTILES; t++)
    {
        if ((tilesizx[t] < 64) || (tilesizy[t] < 64) || (picanm[t] & 192))
            continue;
        loadtile(t);
        if (waloff[t] == 0)
            continue;
        n = tilesizx[t] * tilesizy[t];
        for (i = 0; i < n; i++)
            if ((((unsigned char *) waloff[t])[i] >= 240) && (((unsigned char *) waloff[t])[i] < 255))
                break;
        if (i == n)
            return(t);
    }
    return(-1);
}

/* Draws the start view without retexturing; buf (if any) gets the frame. */
static void view_shot(unsigned char *buf, int32_t x, int32_t y, int32_t z, short a, short sect)
{
    clearallviews(0);
    drawrooms(x, y, z, a, 100, sect);
    drawmasks();
    nextpage();
    if (buf) read_front(buf);
}

/* Draws the start view in a window a quarter of the screen in from each
 *  side, over a screen cleared to colour 31 (as a view with a status bar
 *  and border); buf gets the frame. */
static void small_shot(unsigned char *buf, int32_t x, int32_t y, int32_t z, short a, short sect)
{
    clearallviews(31);
    setview(xdim/4, ydim/4, xdim*3/4-1, ydim*3/4-1);
    drawrooms(x, y, z, a, 100, sect);
    drawmasks();
    nextpage();
    read_front(buf);
    setview(0, 0, xdim-1, ydim-1);
}

/* Mean absolute difference per channel of two read_front buffers inside
 *  small_shot's window. */
static double small_diff(const unsigned char *a, const unsigned char *b)
{
    long x, y, i, d = 0, n = 0;
    for (y = ydim - ydim*3/4; y < ydim - ydim/4; y++)  /* GL rows are bottom-up. */
        for (x = xdim/4; x < xdim*3/4; x++, n++)
            for (i = 0; i < 3; i++)
                d += abs((int) a[(y * xdim + x) * 3 + i] - (int) b[(y * xdim + x) * 3 + i]);
    return (double) d / (n * 3.0);
}

/* The frames path_shots draws. */
#define PATHSHOTS 8

/* Draws the frames compared across draw paths into shots: the start view
 *  with a HUD tile, text, a line and a pixel, then turned three ways (sky,
 *  sprites), with fullbright on (glow passes over masked walls and a
 *  translucent sprite, as the caller sets them), the map view with lines and
 *  the tile picker, a still screen, and the start view with fullbright on
 *  and HUD tiles opaque and translucent over it (the caller makes sprites
 *  and masked walls opaque and translucent side by side, so only blend and
 *  depth state changes between them). */
static void path_shots(unsigned char **shots, int32_t x, int32_t y, int32_t z, short a,
                       short sect, short tile)
{
    int i;

    for (i = 0; i < 5; i++)
    {
        polymosttexfullbright = (i == 4) ? 240 : 256;
        if (i >= 3) gltexinvalidateall();
        clearallviews(0);
        drawrooms(x, y, z, (short) ((a + (i & 3) * 512) & 2047), 100, sect);
        drawmasks();
        if (i == 0)
        {
            rotatesprite(160<<16, 130<<16, 65536, 0, FIRSTGUN, 0, 0, 2, 0, 0, xdim-1, ydim-1);
            printext256(16, 16, 31, 0, "RENDERER 3 PRINTEXT256", 0);
            printext256(16, 32, 159, -1, "small font", 1);
            drawline256(16<<12, 48<<12, (xdim-16)<<12, (ydim-16)<<12, 31);
            plotpixel(8, 8, 31);
        }
        nextpage();
        read_front(shots[i]);
    }
    polymosttexfullbright = 256;
    gltexinvalidateall();

    clearview(0);
    drawmapview(x, y, 2048, a);
    for (i = 0; i < 64; i++)
        drawline256((xdim/2)<<12, (ydim/2)<<12, (xdim/2 + (sintable[(i*32+512)&2047]>>7))<<12,
                    (ydim/2 + (sintable[(i*32)&2047]>>7))<<12, (unsigned char) (i + 64));
    setpolymost2dview();
    polymost_drawtilescreen(8, 8, tile, 128);
    nextpage();
    read_front(shots[5]);

    clearview(0);
    rotatesprite(0, 50<<16, 65536, 0, VICTORY1, 0, 0, 2+8+16+64+128, 0, 0, xdim-1, ydim-1);
    printext256(16, 16, 31, 0, "RENDERER 3 PRINTEXT256", 0);
    printext256(16, 32, 159, -1, "small font", 1);
    drawline256(16<<12, 48<<12, (xdim-16)<<12, (ydim-16)<<12, 31);
    plotpixel(8, 8, 31);
    nextpage();
    read_front(shots[6]);

    polymosttexfullbright = 240;
    gltexinvalidateall();
    clearallviews(0);
    drawrooms(x, y, z, a, 100, sect);
    drawmasks();
    rotatesprite(100<<16, 130<<16, 65536, 0, FIRSTGUN, 0, 0, 2, 0, 0, xdim-1, ydim-1);
    rotatesprite(160<<16, 130<<16, 65536, 0, FIRSTGUN, 0, 0, 2+1, 0, 0, xdim-1, ydim-1);
    rotatesprite(220<<16, 130<<16, 65536, 0, FIRSTGUN, 8, 0, 2+1+32, 0, 0, xdim-1, ydim-1);
    for (i = 0; i < 3; i++)  /* each over the last, so a translucent one reads what's just drawn */
        rotatesprite((150 + i * 10)<<16, (40 + i * 10)<<16, 65536, 0, THREEDEE, 0, 0,
                     (i == 0) ? 2+8 : ((i == 1) ? 2+8+1 : 2+8+1+32), 0, 0, xdim-1, ydim-1);
    nextpage();
    read_front(shots[7]);
    polymosttexfullbright = 256;
    gltexinvalidateall();
}

/* Shows the last frame again (setpalettefade with no frame drawn): lastframe
 *  must give back what was on screen. 1 if it does. */
static int lastframe_same(void)
{
    unsigned char *a = malloc((size_t) xdim * ydim * 3);
    unsigned char *b = malloc((size_t) xdim * ydim * 3);
    int same;

    read_front(a);
    setpalettefade(0, 0, 0, 0);
    read_front(b);
    same = (memcmp(a, b, (size_t) xdim * ydim * 3) == 0);
    free(a);
    free(b);
    return same;
}

/* The comparison with renderer 0: E1L1 viewpoints at horiz 100 (where the
 *  projections line up), drawn 640x480 in both. Face sprites take their
 *  sector's shade, and fire -127, roughly as game.c animatesprites does. Each view has the
 *  most its mean |renderer 3 - renderer 0| per channel may be, on the fixed
 *  function paths and on the shader path. The shader path's are pinned at
 *  what it measured (+0.3), under the target of 8 (and its regions within 3
 *  of renderer 0's). "transprites" has translucent sprites put in it
 *  (cmp_draw). */
#define CMPW 640
#define CMPH 480
static const struct { const char *name; int32_t x, y, z; short ang; double maxdiff, shmaxdiff; } cmpview[] = {
    { "distance", 13824, 50350, -2048, 1024, 5.8, 4.7 },
    { "closeup", 13824, 43118, -2048, 1536, 2.5, 1.4 },
    { "dark-lit", 13824, 50350, -2048, 512, 3.3, 2.8 },
    { "sprites", 24767, 46541, -2048, 1409, 3.7, 1.8 },
    { "sprites-mid", 14107, 44964, -2048, 1793, 4.6, 2.3 },
    { "sky", -31243, 7160, -181472, 1446, 5.7, 5.4 },
    { "translucent", 14938, 44309, -2048, 300, 6.0, 3.8 },
    { "transprites", 13824, 50350, -2048, 1024, 6.0, 4.9 },
};
#define TRANSPRITEVIEW 7  /* cmp_draw puts translucent sprites in it */
#define NCMPVIEWS ((int) (sizeof (cmpview) / sizeof (cmpview[0])))

/* Regions measured in both (640x480 boxes, top-down): the shaded back room
 *  behind the Pig Cop and the far street, both dark in renderer 0 through
 *  distance shading alone. Each has the most |renderer 3 - renderer 0| of
 *  its mean may be. */
static const struct { const char *name; int view, x0, y0, x1, y1; double maxdiff, shmaxdiff; } cmpregion[] = {
    { "back room", 3, 240, 60, 460, 180, 1.5, 1.0 },
    { "far street", 0, 0, 180, 350, 270, 3.0, 1.0 },
};
#define NCMPREGIONS ((int) (sizeof (cmpregion) / sizeof (cmpregion[0])))

/* The sprites cmp_draw puts ahead of TRANSPRITEVIEW, far to near (as
 *  drawmasks draws them), overlapping: an opaque one behind, then smoke and
 *  explosions translucent at both levels (cstat 2, 2|512), one in another
 *  pal and some shaded. */
static const struct { int32_t dist, side; short picnum, cstat, rep; signed char shade; unsigned char pal; } transprite[] = {
    { 1600, 0, EXPLOSION2, 128, 40, 0, 0 },
    { 1400, -200, SMALLSMOKE, 128|2, 110, 0, 0 },
    { 1200, 160, EXPLOSION2, 128|2|512, 40, 8, 2 },
    { 1000, -60, SMALLSMOKE, 128|2|512, 90, 0, 0 },
    { 800, 100, SMALLSMOKE, 128|2, 64, 4, 0 },
};
#define NTRANSPRITES ((int) (sizeof (transprite) / sizeof (transprite[0])))

static void cmp_draw(int v)
{
    short sect = 0, sn, s[NTRANSPRITES];
    int32_t cx, cy, x, y;
    int i;

    updatesector(cmpview[v].x, cmpview[v].y, &sect);
    for (i = 0; (v == TRANSPRITEVIEW) && (i < NTRANSPRITES); i++)
    {
        cx = sintable[(cmpview[v].ang + 512) & 2047]; cy = sintable[cmpview[v].ang & 2047];
        x = cmpview[v].x + ((cx * transprite[i].dist - cy * transprite[i].side) >> 14);
        y = cmpview[v].y + ((cy * transprite[i].dist + cx * transprite[i].side) >> 14);
        sn = sect;
        updatesector(x, y, &sn);
        if (sn < 0) sn = sect;  /* (not outside the map, which would corrupt the lists) */
        s[i] = insertsprite(sn, 0);
        sprite[s[i]].x = x;
        sprite[s[i]].y = y;
        sprite[s[i]].z = cmpview[v].z;
        sprite[s[i]].picnum = transprite[i].picnum;
        sprite[s[i]].cstat = transprite[i].cstat;
        sprite[s[i]].xrepeat = sprite[s[i]].yrepeat = (unsigned char) transprite[i].rep;
        sprite[s[i]].shade = transprite[i].shade;
        sprite[s[i]].pal = transprite[i].pal;
        sprite[s[i]].xoffset = sprite[s[i]].yoffset = 0;
        sprite[s[i]].ang = (short) ((cmpview[v].ang + 1024) & 2047);
    }
    clearallviews(0);
    drawrooms(cmpview[v].x, cmpview[v].y, cmpview[v].z, cmpview[v].ang, 100, sect);
    drawmasks();
    for (i = 0; (v == TRANSPRITEVIEW) && (i < NTRANSPRITES); i++)
        deletesprite(s[i]);
}

/* Boxes of the comparison views (640x480, top-down) where translucent
 *  surfaces are: a web and glass, and the sprites cmp_draw puts in. Each
 *  box's mean |shader - renderer 0| per channel may be at most 8. */
static const struct { const char *name; int view, x0, y0, x1, y1; } cmpbox[] = {
    { "web", 6, 470, 200, 640, 480 },
    { "translucent sprites", TRANSPRITEVIEW, 230, 180, 420, 310 },
};
#define NCMPBOXES ((int) (sizeof (cmpbox) / sizeof (cmpbox[0])))

/* Mean absolute difference per channel, over a box given in 640x480 terms
 *  (top-down), of a GL frame (xdim by ydim, bottom-up) and a renderer 0 one
 *  (640x480, top-down), nearest pixel. */
static double cmp_boxdiff(const unsigned char *gl, const unsigned char *classic, int x0, int y0, int x1, int y1)
{
    long x, y, i, d = 0, n = 0;

    for (y = y0 * ydim / CMPH; y < y1 * ydim / CMPH; y++)
        for (x = x0 * xdim / CMPW; x < x1 * xdim / CMPW; x++, n++)
            for (i = 0; i < 3; i++)
                d += abs((int) gl[((ydim - 1 - y) * xdim + x) * 3 + i] -
                         (int) classic[((y * CMPH / ydim) * CMPW + x * CMPW / xdim) * 3 + i]);
    return (double) d / ((double) n * 3);
}

/* Mean of a box of a frame (w by h, RGB) given in 640x480 terms; up: rows
 *  are bottom-up (as GL reads them). */
static double cmp_mean(const unsigned char *buf, int w, int h, int up, int x0, int y0, int x1, int y1)
{
    int x, y, yy;
    double d = 0;

    x0 = x0 * w / CMPW; x1 = x1 * w / CMPW;
    y0 = y0 * h / CMPH; y1 = y1 * h / CMPH;
    for (y = y0; y < y1; y++)
        for (x = x0, yy = up ? h - 1 - y : y; x < x1; x++)
            d += buf[(yy * w + x) * 3] + buf[(yy * w + x) * 3 + 1] + buf[(yy * w + x) * 3 + 2];
    return d / (3.0 * (x1 - x0) * (y1 - y0));
}

/* Mean absolute difference per channel of a GL frame (xdim by ydim, bottom-up)
 *  and a renderer 0 one (640x480, top-down), nearest pixel. */
static double cmp_diff(const unsigned char *gl, const unsigned char *classic)
{
    long x, y, i, d = 0;
    for (y = 0; y < ydim; y++)
        for (x = 0; x < xdim; x++)
            for (i = 0; i < 3; i++)
                d += abs((int) gl[(y * xdim + x) * 3 + i] -
                         (int) classic[(((ydim - 1 - y) * CMPH / ydim) * CMPW + x * CMPW / xdim) * 3 + i]);
    return (double) d / ((double) xdim * ydim * 3);
}

/* Pixels of a GL frame (xdim by ydim) whose colour isn't one of curpalette's. */
static long off_palette(const unsigned char *gl)
{
    static unsigned char inpal[1 << 21];  /* a bit per RGB colour */
    long i, n = 0, c;

    memset(inpal, 0, sizeof (inpal));
    for (i = 0; i < 256; i++)
    {
        c = ((long) curpalette[i].r << 16) | (curpalette[i].g << 8) | curpalette[i].b;
        inpal[c >> 3] |= 1 << (c & 7);
    }
    for (i = 0; i < (long) xdim * ydim; i++)
    {
        c = ((long) gl[i*3] << 16) | (gl[i*3+1] << 8) | gl[i*3+2];
        if (!(inpal[c >> 3] & (1 << (c & 7)))) n++;
    }
    return n;
}

/* Dead troopers and where they're seen from: corpse_find's against a wall
 *  (with a live trooper in front of it), and one at the bottom of the E1L1
 *  pit past the start (sector 131, its floor 4096 below the corridor's),
 *  seen from the corridor (sector 128): what classic shows of it, the pit's
 *  rim and the walls in front hide in Polymost too, wherever it's drawn
 *  (classic shows two slivers of it between them, Polymost none). */
#define NCORPSES 2
static struct { int32_t x, y, z, cx, cy, cz; short sect, csect, ang; } corpse[NCORPSES] = {
    { 0 }, { 26464, 47040, 0, 26464, 48440, 0, 0, 0, 1536 },  /* corpse_find fills in the rest */
};
static const char *const corpsename[NCORPSES] = { "corpse", "pit" };

/* A dead trooper as the 1.3D demos leave one (a face sprite, xrepeat 40,
 *  256 above the floor) 96 in from the middle of sector sect's longest
 *  solid wall, seen from 900 away at 50 degrees off the wall's normal, from
 *  eye height. Its flat picture, 640 wide, reaches through the wall, which
 *  classic draws it over: the wall is behind it. 1 if there's such a view. */
static int corpse_at(short sect)
{
    int32_t w, best = -1, dx, dy, s, t;
    double len, bestlen = 0, nx = 0, ny = 0;

    for (w = sector[sect].wallptr; w < sector[sect].wallptr + sector[sect].wallnum; w++)
    {
        if (wall[w].nextsector >= 0) continue;
        dx = wall[wall[w].point2].x - wall[w].x; dy = wall[wall[w].point2].y - wall[w].y;
        len = sqrt((double) dx * dx + (double) dy * dy);
        if (len > bestlen) { bestlen = len; best = w; }
    }
    if (bestlen < 1024) return 0;
    dx = wall[wall[best].point2].x - wall[best].x; dy = wall[wall[best].point2].y - wall[best].y;
    for (s = -1; s <= 1; s += 2)  /* the normal into the sector */
    {
        nx = -dy * s / bestlen; ny = dx * s / bestlen;
        corpse[0].x = (wall[best].x + wall[wall[best].point2].x) / 2 + (int32_t) (nx * 96);
        corpse[0].y = (wall[best].y + wall[wall[best].point2].y) / 2 + (int32_t) (ny * 96);
        if (inside(corpse[0].x, corpse[0].y, sect) == 1) break;
    }
    if (s > 1) return 0;
    corpse[0].sect = sect;
    corpse[0].z = sector[sect].floorz - 256;
    corpse[0].cz = sector[sect].floorz - 9889;  /* the demo's eye height */
    for (t = -1; t <= 1; t += 2)
    {
        corpse[0].cx = corpse[0].x + (int32_t) (900 * (nx * 0.643 + dx * t / bestlen * 0.766));
        corpse[0].cy = corpse[0].y + (int32_t) (900 * (ny * 0.643 + dy * t / bestlen * 0.766));
        corpse[0].csect = sect;
        updatesector(corpse[0].cx, corpse[0].cy, &corpse[0].csect);
        if ((corpse[0].csect >= 0) && (corpse[0].cz > sector[corpse[0].csect].ceilingz) &&
            cansee(corpse[0].cx, corpse[0].cy, corpse[0].cz, corpse[0].csect, corpse[0].x, corpse[0].y, corpse[0].z - 1024, sect))
        {
            corpse[0].ang = getangle(corpse[0].x - corpse[0].cx, corpse[0].y - corpse[0].cy);
            return 1;
        }
    }
    return 0;
}

/* corpse_at the first sector it finds a view in, and the pit's sectors and
 *  heights from where it and the corridor are; 0 if either isn't there (the
 *  pit's floor not well below the corridor's). */
static int corpse_find(void)
{
    short sect;

    corpse[1].sect = corpse[1].csect = 0;
    updatesector(corpse[1].x, corpse[1].y, &corpse[1].sect);
    updatesector(corpse[1].cx, corpse[1].cy, &corpse[1].csect);
    if ((corpse[1].sect < 0) || (corpse[1].csect < 0) ||
        (sector[corpse[1].sect].floorz < sector[corpse[1].csect].floorz + 2048))
        return 0;
    corpse[1].z = sector[corpse[1].sect].floorz - 256;
    corpse[1].cz = sector[corpse[1].csect].floorz - 9889;
    for (sect = 0; sect < numsectors; sect++)
        if (!(sector[sect].floorstat & 3) && corpse_at(sect))
        {
            printf("classic comparison: a dead trooper in sector %d at %d,%d seen from %d,%d; "
                   "one in the pit, sector %d, seen from sector %d\n", sect,
                   (int) corpse[0].x, (int) corpse[0].y, (int) corpse[0].cx, (int) corpse[0].cy,
                   corpse[1].sect, corpse[1].csect);
            return 1;
        }
    return 0;
}

/* Draws dead trooper c's view, with it or without; buf gets the frame (gl
 *  0: renderer 0's, CMPW by CMPH, top-down; 1: read_front's). Its sector is
 *  made eight times as misty, so its distance shade shows, and for the one
 *  against a wall a live trooper stands 40 in front of it, a little to the
 *  side, drawn over it as classic draws the nearer sprite last. */
static void corpse_shot(unsigned char *buf, int c, int with, int gl)
{
    short s = -1, live = -1;
    int x, y;
    unsigned char vis = sector[corpse[c].sect].visibility;
    double d = sqrt((double) (corpse[c].x - corpse[c].cx) * (corpse[c].x - corpse[c].cx) +
                    (double) (corpse[c].y - corpse[c].cy) * (corpse[c].y - corpse[c].cy));

    if (with)
    {
        s = insertsprite(corpse[c].sect, 0);
        sprite[s].x = corpse[c].x; sprite[s].y = corpse[c].y; sprite[s].z = corpse[c].z;
        sprite[s].picnum = TROOPDEAD;
        sprite[s].cstat = 0;
        sprite[s].xrepeat = sprite[s].yrepeat = 40;
        sprite[s].xoffset = sprite[s].yoffset = 0;
        sprite[s].shade = 0;
        sprite[s].pal = 0;
        sprite[s].ang = corpse[c].ang;
    }
    if (c == 0) live = insertsprite(corpse[c].sect, 0);
    if (live >= 0)
    {
        sprite[live].x = corpse[c].x - (int32_t) ((corpse[c].x - corpse[c].cx) * 40 / d) - (int32_t) ((corpse[c].y - corpse[c].cy) * 160 / d);
        sprite[live].y = corpse[c].y - (int32_t) ((corpse[c].y - corpse[c].cy) * 40 / d) + (int32_t) ((corpse[c].x - corpse[c].cx) * 160 / d);
        sprite[live].z = sector[corpse[c].sect].floorz;
        sprite[live].picnum = LIZTROOP;
        sprite[live].cstat = 0;
        sprite[live].xrepeat = sprite[live].yrepeat = 40;
        sprite[live].xoffset = sprite[live].yoffset = 0;
        sprite[live].shade = 0;
        sprite[live].pal = 0;
        sprite[live].ang = corpse[c].ang;
    }
    sector[corpse[c].sect].visibility = 112;
    clearallviews(0);
    drawrooms(corpse[c].cx, corpse[c].cy, corpse[c].cz, corpse[c].ang, 100, corpse[c].csect);
    drawmasks();
    nextpage();
    sector[corpse[c].sect].visibility = vis;
    if (gl)
        read_front(buf);
    else
        for (y = 0; y < CMPH; y++)
            for (x = 0; x < CMPW; x++)
            {
                SDL_Color *c = &surface->format->palette->colors[((unsigned char *) surface->pixels)[y * surface->pitch + x]];
                buf[(y * CMPW + x) * 3] = c->r; buf[(y * CMPW + x) * 3 + 1] = c->g; buf[(y * CMPW + x) * 3 + 2] = c->b;
            }
    if (s >= 0) deletesprite(s);
    if (live >= 0) deletesprite(live);
}

/* Of the dead trooper's pixels in renderer 0 (where its frames with and
 *  without it differ), the share whose GL pixel (nearest) doesn't change;
 *  colour gets the mean |GL - renderer 0| per channel over the others, and
 *  extra the GL pixels that change where renderer 0's don't, as a share of
 *  renderer 0's. */
static double corpse_missing(unsigned char *const r0[2], unsigned char *const gl[2], double *colour, double *extra)
{
    long x, y, i, j, k, n = 0, missing = 0, d = 0, more = 0;

    for (y = 0; y < CMPH; y++)
        for (x = 0; x < CMPW; x++)
        {
            i = (y * CMPW + x) * 3;
            j = ((ydim - 1 - y * ydim / CMPH) * xdim + x * xdim / CMPW) * 3;
            if (memcmp(&r0[0][i], &r0[1][i], 3) == 0)
            {
                if (memcmp(&gl[0][j], &gl[1][j], 3) != 0) more++;
                continue;
            }
            n++;
            if (memcmp(&gl[0][j], &gl[1][j], 3) == 0) { missing++; continue; }
            for (k = 0; k < 3; k++) d += abs((int) gl[1][j + k] - (int) r0[1][i + k]);
        }
    *colour = (n > missing) ? (double) d / ((n - missing) * 3) : 0;
    *extra = n ? (double) more / n : 1.0;
    return n ? (double) missing / n : 1.0;
}

/* Draws the comparison views in renderer 0, then in renderer 3 on the
 *  immediate (vertex arrays), vbo and shader paths, and checks them against
 *  the limits; the shader path's views must be all palette colours. Then
 *  the dead troopers by walls: on each path at most a tenth of renderer 0's
 *  pixels of the one against a wall may be missing, at most a tenth more may
 *  be drawn of either, and on the shader path the rest may be at most 7.5
 *  from renderer 0's. Captures go to
 *  <dir>/cmp_<view>_<r0|immediate|vbo|shader>.bmp. 0: fail, 77: no OpenGL. */
static int classic_compare(const char *dir)
{
    static const struct { int32_t path; const char *name; } cmppath[] = {
        { GLPATH_IMMEDIATE, "immediate" }, { GLPATH_VBO, "vbo" }, { GLPATH_SHADER, "shader" },
    };
    static signed char savedshade[MAXSPRITES];
    unsigned char *classic[NCMPVIEWS], *gl;
    char path[1024];
    double d, m0, m3, rs, gs, most;
    int v, p, r, y, x, ok = 1, sh;
    long off;
    uint32_t copies;
    SDL_Surface *s;
    unsigned char *corpse0[NCORPSES][2], *corpse3[NCORPSES][2];
    int c;

    for (v = 0; v < MAXSPRITES; v++)
    {
        savedshade[v] = sprite[v].shade;
        if ((sprite[v].statnum < MAXSTATUS) && !(sprite[v].cstat & 48))
        {
            sectortype *sc = &sector[sprite[v].sectnum];
            sprite[v].shade = (sc->ceilingstat & 1) ? sc->ceilingshade : sc->floorshade;
            if ((sprite[v].picnum == FIRE) || (sprite[v].picnum == FIRE2))
                sprite[v].shade = -127;  /* as game.c animatesprites: never darkened */
        }
    }

    if (_setgamemode(1, CMPW, CMPH) != 0) { fprintf(stderr, "FAIL: _setgamemode(640x480)\n"); return 0; }
    setbrightness(0, palette);
    for (v = 0; v < NCMPVIEWS; v++)
    {
        classic[v] = malloc(CMPW * CMPH * 3);
        cmp_draw(v);
        nextpage();
        for (y = 0; y < CMPH; y++)
            for (x = 0; x < CMPW; x++)
            {
                SDL_Color *c = &surface->format->palette->colors[((unsigned char *) surface->pixels)[y * surface->pitch + x]];
                classic[v][(y * CMPW + x) * 3] = c->r;
                classic[v][(y * CMPW + x) * 3 + 1] = c->g;
                classic[v][(y * CMPW + x) * 3 + 2] = c->b;
            }
        s = SDL_CreateRGBSurfaceWithFormatFrom(classic[v], CMPW, CMPH, 24, CMPW * 3, SDL_PIXELFORMAT_RGB24);
        snprintf(path, sizeof (path), "%s/cmp_%s_r0.bmp", dir, cmpview[v].name);
        SDL_SaveBMP(s, path);
        SDL_FreeSurface(s);
    }
    if (!corpse_find()) { fprintf(stderr, "FAIL: no view of a dead trooper against a wall or in the pit\n"); return 0; }
    for (c = 0; c < NCORPSES; c++)
    {
        for (v = 0; v < 2; v++)
        {
            corpse0[c][v] = malloc(CMPW * CMPH * 3);
            corpse_shot(corpse0[c][v], c, v, 0);
        }
        s = SDL_CreateRGBSurfaceWithFormatFrom(corpse0[c][1], CMPW, CMPH, 24, CMPW * 3, SDL_PIXELFORMAT_RGB24);
        snprintf(path, sizeof (path), "%s/cmp_%s_r0.bmp", dir, corpsename[c]);
        SDL_SaveBMP(s, path);
        SDL_FreeSurface(s);
    }

    for (p = 0; p < (int) (sizeof (cmppath) / sizeof (cmppath[0])); p++)
    {
        gldrawpathrequest = cmppath[p].path;
        if (setgamemode_gl(1) != 0)
        {
            printf("SKIP: no OpenGL (%s)\n", SDL_GetError());
            return 77;
        }
        if (gldrawpath != cmppath[p].path)
        {
            printf("classic comparison, %s: not on this driver, skipped\n", cmppath[p].name);
            continue;
        }
        setbrightness(0, palette);
        polymosttexfullbright = 240;  /* as the game's palettes (menues.c glsetgamepalette) */
        gltexinvalidateall();
        gl = malloc((size_t) xdim * ydim * 3);
        sh = (cmppath[p].path == GLPATH_SHADER);
        for (v = 0; v < NCMPVIEWS; v++)
        {
            copies = gl_transcopies;
            cmp_draw(v);
            copies = gl_transcopies - copies;
            nextpage();
            snprintf(path, sizeof (path), "%s/cmp_%s_%s.bmp", dir, cmpview[v].name, cmppath[p].name);
            save_front(path, &rs, &gs);
            read_front(gl);
            d = cmp_diff(gl, classic[v]);
            most = sh ? cmpview[v].shmaxdiff : cmpview[v].maxdiff;
            printf("classic comparison, %s: %s mean |r3 - r0| %.2f (most %.2f)\n", cmppath[p].name,
                   cmpview[v].name, d, most);
            if (d > most) ok = 0;
            if (sh)
            {
                off = off_palette(gl);
                printf("classic comparison, %s: %s has %ld pixels not of the palette; %u copies for translucent draws\n",
                       cmppath[p].name, cmpview[v].name, off, (unsigned) copies);
                if (off) ok = 0;
                for (r = 0; r < NCMPBOXES; r++)
                {
                    if (cmpbox[r].view != v) continue;
                    d = cmp_boxdiff(gl, classic[v], cmpbox[r].x0, cmpbox[r].y0, cmpbox[r].x1, cmpbox[r].y1);
                    printf("classic comparison, %s: %s mean |r3 - r0| %.2f (most 8.00)\n",
                           cmppath[p].name, cmpbox[r].name, d);
                    if (d > 8) ok = 0;
                }
            }
            for (r = 0; r < NCMPREGIONS; r++)
            {
                if (cmpregion[r].view != v) continue;
                m0 = cmp_mean(classic[v], CMPW, CMPH, 0, cmpregion[r].x0, cmpregion[r].y0,
                              cmpregion[r].x1, cmpregion[r].y1);
                m3 = cmp_mean(gl, xdim, ydim, 1, cmpregion[r].x0, cmpregion[r].y0,
                              cmpregion[r].x1, cmpregion[r].y1);
                most = sh ? cmpregion[r].shmaxdiff : cmpregion[r].maxdiff;
                printf("classic comparison, %s: %s mean r0 %.1f, r3 %.1f (most apart %.1f)\n",
                       cmppath[p].name, cmpregion[r].name, m0, m3, most);
                if (fabs(m3 - m0) > most) ok = 0;
            }
        }
        free(gl);
        for (c = 0; c < NCORPSES; c++)
        {
            for (v = 0; v < 2; v++)
            {
                corpse3[c][v] = malloc((size_t) xdim * ydim * 3);
                corpse_shot(corpse3[c][v], c, v, 1);
            }
            snprintf(path, sizeof (path), "%s/cmp_%s_%s.bmp", dir, corpsename[c], cmppath[p].name);
            save_front(path, &rs, &gs);
            d = corpse_missing(corpse0[c], corpse3[c], &m3, &m0);
            printf("classic comparison, %s: %s dead trooper %.1f%% missing%s, %.1f%% extra (most 10%%), "
                   "the rest mean |r3 - r0| %.2f\n", cmppath[p].name, corpsename[c], 100 * d,
                   c ? "" : " (most 10%)", 100 * m0, m3);
            if (((c == 0) && (d > 0.1)) || (m0 > 0.1)) ok = 0;
            if (sh && (m3 > 7.5)) ok = 0;  /* the shader path's shade is classic's */
            for (v = 0; v < 2; v++) free(corpse3[c][v]);
        }
        if (glGetError() != GL_NO_ERROR) { fprintf(stderr, "FAIL: GL error in the classic comparison\n"); return 0; }
    }
    gldrawpathrequest = GLPATH_AUTO;
    polymosttexfullbright = 256;
    for (v = 0; v < NCMPVIEWS; v++) free(classic[v]);
    for (c = 0; c < NCORPSES; c++)
        for (v = 0; v < 2; v++) free(corpse0[c][v]);
    for (v = 0; v < MAXSPRITES; v++) sprite[v].shade = savedshade[v];
    if (!ok) fprintf(stderr, "FAIL: renderer 3 is further from renderer 0 than pinned\n");
    return ok;
}

/* The frames shader_frames draws in both renderers. */
#define SHFRAMES 5
static const char *const shframename[SHFRAMES] = { "hud", "water", "fade", "camera", "map" };
static unsigned char camerabuf[200 * 320];  /* the camera's tile, as game.c's tilt tile */

/* Draws frame f of shader_frames and shows it: the start view (with a
 *  camera's picture over it, drawn to a tile in renderer 0 as game.c's
 *  cameras and tilt are, for "camera"), the HUD's gun, the status bar, menu
 *  letters shaded and in another pal, the crosshair and two logos
 *  translucent (dastat 1, 1|32), and text, a line and a pixel; in the
 *  water palette for "water", faded to red after it's shown for "fade".
 *  "map" is the map view instead, with two floor sprites ahead, translucent
 *  at the two levels, one over the other, their sector lit. */
static void shader_frame(int f, int32_t x, int32_t y, int32_t z, short a, short sect)
{
    int i;
    short s[2];
    signed char floorshade;

    setbrightness(0, (f == 1) ? waterpal : palette);
    if (f == 4)
    {
        floorshade = sector[sect].floorshade;
        sector[sect].floorshade = 0;
        for (i = 0; i < 2; i++)
        {
            s[i] = insertsprite(sect, 0);
            sprite[s[i]].x = x + (sintable[(a+512)&2047]>>6) + i * (sintable[a&2047]>>7);
            sprite[s[i]].y = y + (sintable[a&2047]>>6) - i * (sintable[(a+512)&2047]>>7);
            sprite[s[i]].z = sector[sect].floorz;
            sprite[s[i]].cstat = i ? 32|2|512 : 32|2;
            sprite[s[i]].picnum = THREEDEE;
            sprite[s[i]].shade = 0;
            sprite[s[i]].pal = 0;
            sprite[s[i]].xrepeat = sprite[s[i]].yrepeat = 64;
            sprite[s[i]].xoffset = sprite[s[i]].yoffset = 0;
            sprite[s[i]].ang = a;
        }
        clearview(0);
        drawmapview(x, y, 2048, a);
        nextpage();
        for (i = 0; i < 2; i++) deletesprite(s[i]);
        sector[sect].floorshade = floorshade;
        return;
    }
    clearallviews(0);
    if (f == 3)
    {
        waloff[MAXTILES-2] = (intptr_t) camerabuf;
        setviewtotile(MAXTILES-2, 200, 320);
        drawrooms(x, y, z, (short) ((a + 1024) & 2047), 100, sect);
        drawmasks();
        setviewback();
    }
    drawrooms(x, y, z, a, 100, sect);
    drawmasks();
    if (f == 3)
        rotatesprite(160<<16, 80<<16, 32768, 512, MAXTILES-2, 0, 0, 4+2+64, 0, 0, xdim-1, ydim-1);
    rotatesprite(160<<16, 130<<16, 65536, 0, FIRSTGUN, 0, 0, 2, 0, 0, xdim-1, ydim-1);
    rotatesprite(0, (200 - tilesizy[BOTTOMSTATUSBAR])<<16, 65536, 0, BOTTOMSTATUSBAR, 4, 0,
                 2+8+16+64, 0, 0, xdim-1, ydim-1);
    for (i = 0; i < 4; i++)
        rotatesprite((40 + i * 16)<<16, 30<<16, 65536, 0, STARTALPHANUM + ("DUKE"[i] - '!'),
                     (i & 1) ? 16 : 0, (i & 2) ? 2 : 0, 2+8+16, 0, 0, xdim-1, ydim-1);
    rotatesprite(160<<16, 100<<16, 65536, 0, CROSSHAIR, 0, 0, 2+1, 0, 0, xdim-1, ydim-1);
    rotatesprite(230<<16, 60<<16, 32768, 0, THREEDEE, 0, 0, 2+8+1, 0, 0, xdim-1, ydim-1);
    rotatesprite(250<<16, 75<<16, 32768, 0, THREEDEE, 8, 2, 2+8+1+32, 0, 0, xdim-1, ydim-1);
    printext256(16, 16, 31, 0, "RENDERER 3 PRINTEXT256", 0);
    printext256(16, 32, 159, -1, "small font", 1);
    drawline256(16<<12, 48<<12, (xdim-16)<<12, (ydim-16)<<12, 31);
    plotpixel(8, 8, 31);
    nextpage();
    if (f == 2) setpalettefade(63, 0, 0, 32);
}

/* Mean absolute difference per channel of a GL frame (gw by gh, bottom-up)
 *  and a renderer 0 one (cw by ch, top-down), nearest pixel. */
static double frame_diff(const unsigned char *gl, int gw, int gh, const unsigned char *classic, int cw, int ch)
{
    long x, y, i, d = 0;
    for (y = 0; y < gh; y++)
        for (x = 0; x < gw; x++)
            for (i = 0; i < 3; i++)
                d += abs((int) gl[(y * gw + x) * 3 + i] -
                         (int) classic[(((gh - 1 - y) * ch / gh) * cw + x * cw / gw) * 3 + i]);
    return (double) d / ((double) gw * gh * 3);
}

/* Boxes of shader_frame's frames (a bit per frame), in thousandths of the
 *  frame (top-down), around its translucent tiles: the crosshair, two logos
 *  at the two levels, one over the other, and the map view's floor sprites. */
static const struct { const char *name; int frames, x0, y0, x1, y1; } shbox[] = {
    { "crosshair", 15, 480, 470, 520, 530 },
    { "translucent logos", 15, 630, 260, 790, 420 },
    { "floor sprites", 16, 435, 410, 555, 495 },
};
#define NSHBOXES ((int) (sizeof (shbox) / sizeof (shbox[0])))

/* frame_diff over a box (x0, y0, x1, y1: thousandths of the frame, top-down). */
static double frame_boxdiff(const unsigned char *gl, int gw, int gh, const unsigned char *classic, int cw, int ch,
                            const int *box)
{
    long x, y, i, d = 0, n = 0;
    for (y = box[1] * gh / 1000; y < box[3] * gh / 1000; y++)
        for (x = box[0] * gw / 1000; x < box[2] * gw / 1000; x++, n++)
            for (i = 0; i < 3; i++)
                d += abs((int) gl[((gh - 1 - y) * gw + x) * 3 + i] -
                         (int) classic[((y * ch / gh) * cw + x * cw / gw) * 3 + i]);
    return (double) d / ((double) n * 3);
}

/* Pixels of a GL frame (n pixels) whose colour isn't one of pal's 256. */
static long off_colours(const unsigned char *gl, long n, const palette_t *pal)
{
    static unsigned char inpal[1 << 21];  /* a bit per RGB colour */
    long i, m = 0, c;

    memset(inpal, 0, sizeof (inpal));
    for (i = 0; i < 256; i++)
    {
        c = ((long) pal[i].r << 16) | (pal[i].g << 8) | pal[i].b;
        inpal[c >> 3] |= 1 << (c & 7);
    }
    for (i = 0; i < n; i++)
    {
        c = ((long) gl[i*3] << 16) | (gl[i*3+1] << 8) | gl[i*3+2];
        if (!(inpal[c >> 3] & (1 << (c & 7)))) m++;
    }
    return m;
}

/* 1 if the 24-bit BMP at path is the read_front buffer buf (xdim by ydim). */
static int bmp_is(const char *path, const unsigned char *buf)
{
    SDL_Surface *b = SDL_LoadBMP(path);
    SDL_Surface *s = (b != NULL) ? SDL_ConvertSurfaceFormat(b, SDL_PIXELFORMAT_RGB24, 0) : NULL;
    int y, same = ((s != NULL) && (s->w == xdim) && (s->h == ydim));

    for (y = 0; same && (y < ydim); y++)  /* GL rows are bottom-up. */
        same = (memcmp((unsigned char *) s->pixels + y * s->pitch, buf + (ydim - 1 - y) * xdim * 3, xdim * 3) == 0);
    SDL_FreeSurface(s);
    SDL_FreeSurface(b);
    return same;
}

/* Waits up to 3 s for the window's drawable to stop being oldw wide. */
static void await_resize(int32_t oldw)
{
    Uint32 t0;
    for (t0 = SDL_GetTicks(); (xdim == oldw) && (SDL_GetTicks() - t0 < 3000); SDL_Delay(10))
        _handle_events();
}

/* The shader path's contrast and saturation (glcontrast, glsaturation):
 *  at other values than 1 the frame shown, unfaded and faded, is the one
 *  at 1 with gl_colorcorrect applied to each pixel, and a screenshot is it;
 *  back at 1 the frame is the same as before. 0 fails, 77 no shader path. */
static int color_frames(const char *dir, int32_t x, int32_t y, int32_t z, short a, short sect)
{
    static const int frames[2] = { 0, 2 };  /* hud, and it faded */
    unsigned char *ref[2], *buf;
    char path[1024];
    int f, ok = 1, capsame;
    long i, n, wrong, changed;

    gldrawpathrequest = GLPATH_SHADER;
    if (setgamemode_gl(1) != 0) { fprintf(stderr, "FAIL: OpenGL with draw path shader\n"); return 0; }
    gldrawpathrequest = GLPATH_AUTO;
    if (gldrawpath != GLPATH_SHADER) { printf("color frames: no shader path, skipped\n"); return 77; }
    n = (long) xdim * ydim;
    buf = malloc((size_t) n * 3);
    for (f = 0; f < 2; f++)
    {
        ref[f] = malloc((size_t) n * 3);
        glcontrast = glsaturation = 1.0;
        shader_frame(frames[f], x, y, z, a, sect);
        read_front(ref[f]);
        setpalettefade(0, 0, 0, 0);
        glcontrast = 1.3;
        glsaturation = 0.4;
        shader_frame(frames[f], x, y, z, a, sect);
        read_front(buf);
        for (wrong = changed = 0, i = 0; i < n; i++)
        {
            unsigned char c[3];
            memcpy(c, &ref[f][i * 3], 3);
            gl_colorcorrect(c, glcontrast, glsaturation);
            if (memcmp(c, &buf[i * 3], 3)) wrong++;
            if (memcmp(&ref[f][i * 3], &buf[i * 3], 3)) changed++;
        }
        snprintf(path, sizeof (path), "%s/cccap%d0000.bmp", dir, f);
        remove(path);
        snprintf(path, sizeof (path), "%s/cccap%dxxxx.bmp", dir, f);
        capsame = screencapture(path, 0) == 0;
        snprintf(path, sizeof (path), "%s/cccap%d0000.bmp", dir, f);
        capsame = capsame && bmp_is(path, buf);
        setpalettefade(0, 0, 0, 0);
        printf("color frames: %s at contrast 1.3, saturation 0.4: %ld pixels changed, %ld not as the formula says; "
               "screenshot %s\n", f ? "fade" : "hud", changed, wrong, capsame ? "the screen" : "NOT the screen");
        if ((wrong != 0) || (changed < n / 10))  /* black stays black */
            { fprintf(stderr, "FAIL: the shader path's contrast and saturation aren't the formula's\n"); ok = 0; }
        if (!capsame)
            { fprintf(stderr, "FAIL: a shader path screenshot lacks the contrast and saturation\n"); ok = 0; }
    }
    glcontrast = glsaturation = 1.0;
    shader_frame(0, x, y, z, a, sect);
    read_front(buf);
    printf("color frames: back at 1, 1: mean |frame - frame before| %.2f\n", front_diff(buf, ref[0]));
    if (memcmp(buf, ref[0], (size_t) n * 3))
        { fprintf(stderr, "FAIL: the shader path's frame changes at contrast and saturation 1\n"); ok = 0; }
    free(ref[0]);
    free(ref[1]);
    free(buf);
    return ok;
}

/*
 * The shader path draws everything as palette indices and shows them
 *  through the palette, as renderer 0 does: shader_frame's frames (HUD,
 *  status bar, menu letters, text, the water palette, a fade, a camera)
 *  each within 8 of renderer 0's at the same size, every colour the
 *  palette's (the faded palette's for the fade), screenshots that are the
 *  screen, faded or not, with no RGB texture made or retextured all along
 *  (palette changes, fades, glsetgamepalette, precaching); a hole in the
 *  level shows what clearallviews cleared to; and a window resize keeps a
 *  frame shown or still being drawn. 1: pass, 0: fail, 77: no shader path.
 */
static int shader_frames(const char *dir, int32_t x, int32_t y, int32_t z, short a, short sect, short zerotile)
{
    unsigned char *gl[SHFRAMES], *classic[SHFRAMES], *buf, *ref;
    palette_t pal[256], fadepal[2][256], glfade[256];
    int fadeoff = 0;
    char path[1024];
    int f, i, ok = 1, gw, gh, cw, ch, capsame[2], winw, winh;
    int32_t oldw, oldh;
    uint32_t loads, invals;
    long off, offpal[SHFRAMES], n, unmasked[2];
    double d, stilld, middrawd, refmean;
    short floorpic, rsect, wallcstat[MAXWALLS];
    SDL_Surface *s;

    gldrawpathrequest = GLPATH_SHADER;
    if (setgamemode_gl(1) != 0) { fprintf(stderr, "FAIL: OpenGL with draw path shader\n"); return 0; }
    gldrawpathrequest = GLPATH_AUTO;
    if (gldrawpath != GLPATH_SHADER) { printf("shader frames: no shader path, skipped\n"); return 77; }
    polymosttexfullbright = 240;  /* as the game's palettes: the shader path ignores it */
    gw = xdim; gh = ydim;
    n = (long) gw * gh;
    loads = gl_rgbtexloads;
    invals = gl_rgbtexinvalidations;
    for (f = 0; f < SHFRAMES; f++)
    {
        gl[f] = malloc((size_t) n * 3);
        shader_frame(f, x, y, z, a, sect);
        snprintf(path, sizeof (path), "%s/shframe_%s_shader.bmp", dir, shframename[f]);
        { double rs, gs; save_front(path, &rs, &gs); }
        read_front(gl[f]);
        getfadedpalette(pal);  /* what's on screen: the fade's palette for "fade" */
        if (f == 2) memcpy(glfade, pal, sizeof (glfade));
        offpal[f] = off_colours(gl[f], n, pal);
        if ((f == 0) || (f == 2))  /* a screenshot is the screen, faded or not */
        {
            snprintf(path, sizeof (path), "%s/shcap%d0000.bmp", dir, f);
            remove(path);
            snprintf(path, sizeof (path), "%s/shcap%dxxxx.bmp", dir, f);
            i = screencapture(path, 0);
            snprintf(path, sizeof (path), "%s/shcap%d0000.bmp", dir, f);
            capsame[f >> 1] = (i == 0) && bmp_is(path, gl[f]);
        }
        if (f == 2) setpalettefade(0, 0, 0, 0);
    }
    /* The map view fills a sector translucent in 3D (floorstat 256, 384)
     * opaque, as classic's drawmapview does: the same frame. */
    buf = malloc((size_t) n * 3);
    for (i = 0; i < 2; i++)
    {
        sector[sect].floorstat |= i ? 384 : 256;
        shader_frame(4, x, y, z, a, sect);
        sector[sect].floorstat &= ~384;
        read_front(buf);
        d = front_diff(buf, gl[4]);
        printf("shader frames: map view mean |sector floorstat %d - 0| %.2f\n", i ? 384 : 256, d);
        if (d != 0) { fprintf(stderr, "FAIL: the shader path's map view fills a translucent sector translucent\n"); ok = 0; }
    }
    free(buf);
    polymost_precache(FIRSTGUN, 0, 1);
    polymost_precache(BOTTOMSTATUSBAR, 2, 0);
    printf("shader frames: %u RGB textures made, %u retexturings (palettes, a fade, precaching)\n",
           (unsigned) (gl_rgbtexloads - loads), (unsigned) (gl_rgbtexinvalidations - invals));
    if ((gl_rgbtexloads != loads) || (gl_rgbtexinvalidations != invals))
        { fprintf(stderr, "FAIL: the shader path makes RGB textures or retextures\n"); ok = 0; }
    loads = gl_rgbtexloads;
    i = glgamepalette_check();
    printf("shader frames: glsetgamepalette %s, %u RGB textures made\n", i ? "as expected" : "wrong",
           (unsigned) (gl_rgbtexloads - loads));
    if ((!i) || (gl_rgbtexloads != loads))
        { fprintf(stderr, "FAIL: glsetgamepalette on the shader path\n"); ok = 0; }
    printf("shader frames: screenshots %s unfaded, %s faded\n", capsame[0] ? "the screen" : "NOT the screen",
           capsame[1] ? "the screen" : "NOT the screen");
    if ((!capsame[0]) || (!capsame[1]))
        { fprintf(stderr, "FAIL: a shader path screenshot isn't the screen\n"); ok = 0; }

    /* A hole (the start floor without art) shows what was cleared to: a
     * good part of the frame (the masked walls the draw paths' checks put in
     * the start view off, as they'd cover it). */
    setbrightness(0, palette);
    floorpic = sector[sect].floorpicnum;
    sector[sect].floorpicnum = zerotile;
    for (i = sector[sect].wallptr; i < sector[sect].wallptr + sector[sect].wallnum; i++)
        { wallcstat[i - sector[sect].wallptr] = wall[i].cstat; wall[i].cstat &= ~(16|128); }
    buf = malloc((size_t) n * 3);
    clearallviews(31);
    drawrooms(x, y, z, a, 100, sect);
    drawmasks();
    nextpage();
    read_front(buf);
    sector[sect].floorpicnum = floorpic;
    for (i = sector[sect].wallptr; i < sector[sect].wallptr + sector[sect].wallnum; i++)
        wall[i].cstat = wallcstat[i - sector[sect].wallptr];
    for (off = 0, i = 0; i < n; i++)  /* (white: nothing else in the view is) */
        if (memcmp(&buf[i * 3], &curpalette[31], 3) == 0) off++;
    printf("shader frames: %.1f%% of the frame is the hole's colour 31\n", 100.0 * off / n);
    if (off < n / 50) { fprintf(stderr, "FAIL: a hole on the shader path isn't what was cleared to\n"); ok = 0; }

    /* A tile drawn non-masked (dastat 64) draws its index 255 texels, as
     * classic does: fewer pixels of what's under it than drawn masked. */
    for (f = 0; f < 2; f++)
    {
        clearallviews(31);
        rotatesprite(0, 0, 65536, 0, FIRSTGUN, 0, 0, 2+8+16+(f ? 64 : 0), 0, 0, xdim-1, ydim-1);
        nextpage();
        read_front(buf);
        for (unmasked[f] = 0, i = 0; i < n; i++)
            if (memcmp(&buf[i * 3], &curpalette[31], 3) == 0) unmasked[f]++;
    }
    printf("shader frames: pixels of colour 31 around a tile masked %ld, not masked %ld\n", unmasked[0], unmasked[1]);
    if (unmasked[1] >= unmasked[0])
        { fprintf(stderr, "FAIL: a non-masked tile on the shader path doesn't draw index 255\n"); ok = 0; }

    /* A window resize keeps a frame shown (a still screen) and one still
     * being drawn (a 3D view not yet shown, as _nextpage's events may
     * resize before its swap). */
    ref = malloc((size_t) n * 3);
    rsect = 0;
    updatesector(cmpview[1].x, cmpview[1].y, &rsect);  /* close up: bright, so a lost frame shows */
    view_shot(ref, cmpview[1].x, cmpview[1].y, cmpview[1].z, cmpview[1].ang, rsect);
    refmean = cmp_mean(ref, xdim, ydim, 1, 0, 0, CMPW, CMPH);
    oldw = xdim; oldh = ydim;
    SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &winw, &winh);
    SDL_SetWindowSize(SDL_GL_GetCurrentWindow(), 800, 520);
    await_resize(oldw);
    free(buf);
    buf = malloc((size_t) xdim * ydim * 3);
    read_front(buf);
    stilld = stretch_diff(ref, oldw, oldh, buf);
    for (i = 0; i < 2; i++)  /* both buffers: on screen if the view below is lost */
    {
        clearallviews(31);
        nextpage();
    }
    SDL_SetWindowSize(SDL_GL_GetCurrentWindow(), winw, winh);
    clearallviews(0);
    drawrooms(cmpview[1].x, cmpview[1].y, cmpview[1].z, cmpview[1].ang, 100, rsect);
    drawmasks();
    await_resize(xdim);
    nextpage();
    if ((xdim != oldw) || (ydim != oldh))
    {
        printf("shader frames: window not back to %dx%d (%dx%d); mid-frame resize unchecked\n",
               (int) oldw, (int) oldh, (int) xdim, (int) ydim);
        middrawd = 0;
    }
    else
    {
        free(buf);
        buf = malloc((size_t) xdim * ydim * 3);
        read_front(buf);
        middrawd = stretch_diff(ref, oldw, oldh, buf);
    }
    printf("shader frames: after a resize, mean |frame - frame before| still %.2f, mid-frame %.2f (frame mean %.1f)\n",
           stilld, middrawd, refmean);
    if ((stilld > 4) || (middrawd > 8))  /* lost: the frame's mean, 70 */
        { fprintf(stderr, "FAIL: a resize loses the shader path's frame\n"); ok = 0; }
    free(ref);
    free(buf);

    /* A resize mid-fade under water keeps the fade's palette (built from the
     * base palette, which the mode's setup resets), and the still screen
     * shown again at the new size is in it. */
    setbrightness(0, waterpal);
    view_shot(NULL, x, y, z, a, sect);
    setpalettefade(63, 0, 0, 32);
    getfadedpalette(fadepal[0]);
    oldw = xdim;
    SDL_SetWindowSize(SDL_GL_GetCurrentWindow(), 800, 520);
    await_resize(oldw);
    getfadedpalette(fadepal[1]);
    buf = malloc((size_t) xdim * ydim * 3);
    read_front(buf);
    off = off_colours(buf, (long) xdim * ydim, fadepal[0]);
    free(buf);
    printf("shader frames: resize %dx%d mid-fade under water: faded palette %s, %ld pixels shown not of it\n",
           (int) oldw, (int) xdim, memcmp(fadepal[0], fadepal[1], sizeof (fadepal[0])) ? "CHANGED" : "kept", off);
    if ((xdim == oldw) || memcmp(fadepal[0], fadepal[1], sizeof (fadepal[0])) || off)
        { fprintf(stderr, "FAIL: a resize changes the shader path's fade\n"); ok = 0; }
    setpalettefade(0, 0, 0, 0);
    setbrightness(0, palette);
    SDL_SetWindowSize(SDL_GL_GetCurrentWindow(), winw, winh);
    await_resize(xdim);
    if (glGetError() != GL_NO_ERROR) { fprintf(stderr, "FAIL: GL error in the shader frames\n"); ok = 0; }

    /* Renderer 0 at the same size (as near as 8-bit goes). */
    cw = min(gw, 1600); ch = min(gh, 1200);
    if (_setgamemode(1, cw, ch) != 0) { fprintf(stderr, "FAIL: _setgamemode(%dx%d)\n", cw, ch); return 0; }
    cw = xdim; ch = ydim;
    for (f = 0; f < SHFRAMES; f++)
    {
        shader_frame(f, x, y, z, a, sect);
        classic[f] = malloc((size_t) cw * ch * 3);
        for (i = 0; i < cw * ch; i++)
        {
            SDL_Color *c = &surface->format->palette->colors[((unsigned char *) surface->pixels)[(i / cw) * surface->pitch + i % cw]];
            classic[f][i * 3] = c->r; classic[f][i * 3 + 1] = c->g; classic[f][i * 3 + 2] = c->b;
        }
        s = SDL_CreateRGBSurfaceWithFormatFrom(classic[f], cw, ch, 24, cw * 3, SDL_PIXELFORMAT_RGB24);
        snprintf(path, sizeof (path), "%s/shframe_%s_r0.bmp", dir, shframename[f]);
        SDL_SaveBMP(s, path);
        SDL_FreeSurface(s);
        if (f == 2)  /* the faded palette is classic's, but for its 6-to-8-bit rounding */
        {
            for (i = 0; i < 256; i++)
            {
                SDL_Color *c = &surface->format->palette->colors[i];
                fadeoff = max(fadeoff, abs(c->r - glfade[i].r));
                fadeoff = max(fadeoff, abs(c->g - glfade[i].g));
                fadeoff = max(fadeoff, abs(c->b - glfade[i].b));
            }
            setpalettefade(0, 0, 0, 0);
        }
    }
    printf("shader frames: faded palette at most %d from renderer 0's\n", fadeoff);
    if (fadeoff > 1) { fprintf(stderr, "FAIL: the shader path's faded palette isn't classic's\n"); ok = 0; }
    setbrightness(0, palette);
    for (f = 0; f < SHFRAMES; f++)
    {
        d = frame_diff(gl[f], gw, gh, classic[f], cw, ch);
        printf("shader frames: %s mean |shader - r0| %.2f (most 8.00), %ld pixels not of the %spalette\n",
               shframename[f], d, offpal[f], (f == 2) ? "faded " : "");
        if ((d > 8) || offpal[f]) ok = 0;
        for (i = 0; i < NSHBOXES; i++)
        {
            if (!(shbox[i].frames & (1 << f))) continue;
            d = frame_boxdiff(gl[f], gw, gh, classic[f], cw, ch, &shbox[i].x0);
            printf("shader frames: %s, %s mean |shader - r0| %.2f (most 8.00)\n", shframename[f], shbox[i].name, d);
            if (d > 8) ok = 0;
        }
        free(gl[f]);
        free(classic[f]);
    }
    polymosttexfullbright = 256;
    if (!ok) fprintf(stderr, "FAIL: the shader path's frames\n");
    return ok;
}

/* The internal-size frames: a view close up (bright, few texels to a
 *  pixel) with the HUD's gun, the status bar and text, and a still screen
 *  drawn only in 2D (twice, so the second is drawn after one is shown): a
 *  logo, menu letters, the status bar and text. */
#define INFRAMES 2
static void internal_frame(int f)
{
    int i, j;

    for (i = 0; i < 2; i++)
    {
        if (f == 0) cmp_draw(1);
        else
        {
            clearview(0);
            for (j = 4; j < 200; j += 8)  /* lines first: in the view the last present left */
                drawline256(0, j<<12, 319<<12, j<<12, 31);
            rotatesprite(160<<16, 80<<16, 65536, 0, THREEDEE, 0, 0, 2+8, 0, 0, xdim-1, ydim-1);
            for (j = 0; j < 4; j++)
                rotatesprite((120 + j * 16)<<16, 140<<16, 65536, 0, STARTALPHANUM + ("DUKE"[j] - '!'),
                             (j & 1) ? 16 : 0, (j & 2) ? 2 : 0, 2+8+16, 0, 0, xdim-1, ydim-1);
        }
        if (f == 0) rotatesprite(160<<16, 130<<16, 65536, 0, FIRSTGUN, 0, 0, 2, 0, 0, xdim-1, ydim-1);
        rotatesprite(0, (200 - tilesizy[BOTTOMSTATUSBAR])<<16, 65536, 0, BOTTOMSTATUSBAR, 4, 0,
                     2+8+16+64, 0, 0, xdim-1, ydim-1);
        printext256(16, 16, 31, 0, "INTERNAL SIZE", 0);
        nextpage();
        if (f == 0) return;
    }
}

/* Mean absolute difference per channel of two top-down RGB frames, w by h. */
static double rgb_diff(const unsigned char *a, const unsigned char *b, int w, int h)
{
    long i, n = (long) w * h * 3, d = 0;
    for (i = 0; i < n; i++) d += abs((int) a[i] - (int) b[i]);
    return (double) d / n;
}

/* The window shown (dw by dh, read bottom-up) against a top-down frame (w by
 *  h) scaled into gl_fitrect's rectangle, nearest pixel: mean |difference|
 *  inside it, and *bars the pixels outside it that aren't black. */
static double shown_diff(const unsigned char *shown, int dw, int dh, const unsigned char *frame, int w, int h,
                         long *bars)
{
    int32_t r[4];
    long x, y, i, d = 0, n = 0;
    const unsigned char *p;

    gl_fitrect(w, h, dw, dh, r);
    *bars = 0;
    for (y = 0; y < dh; y++)
        for (x = 0; x < dw; x++)
        {
            p = &shown[(y * dw + x) * 3];
            if ((x < r[0]) || (x >= r[0] + r[2]) || (y < r[1]) || (y >= r[1] + r[3]))
                { if (p[0] | p[1] | p[2]) (*bars)++; continue; }
            for (i = 0; i < 3; i++)
                d += abs((int) p[i] - (int) frame[(((r[1] + r[3] - 1 - y) * h / r[3]) * w + (x - r[0]) * w / r[2]) * 3 + i]);
            n++;
        }
    return (n > 0) ? (double) d / ((double) n * 3) : 999;
}

/* Reads the whole drawable's front buffer (*dw by *dh, bottom-up) into a malloc'd buffer. */
static unsigned char *read_shown(int *dw, int *dh)
{
    unsigned char *buf;

    SDL_GL_GetDrawableSize(SDL_GL_GetCurrentWindow(), dw, dh);
    buf = malloc((size_t) *dw * *dh * 3);
    glReadBuffer(GL_FRONT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, *dw, *dh, GL_RGB, GL_UNSIGNED_BYTE, buf);
    glReadBuffer(GL_BACK);
    return buf;
}

/* Sets OpenGL with draw path path at internal size w by h (and gl_forcefail
 *  fail); 1 if it then draws at the size want (0: the drawable's). */
static int internal_mode(int32_t path, int32_t w, int32_t h, int32_t fail, int32_t wantw, int32_t wanth,
                         const char *what)
{
    int dw, dh, ok;

    glinternalwidth = w; glinternalheight = h;
    gldrawpathrequest = path;
    gl_forcefail = fail;
    if (setgamemode_gl(1) != 0) { fprintf(stderr, "FAIL: OpenGL at internal size %dx%d\n", (int) w, (int) h); return 0; }
    gl_forcefail = 0;
    SDL_GL_GetDrawableSize(SDL_GL_GetCurrentWindow(), &dw, &dh);
    if (!wantw) { wantw = dw; wanth = dh; }
    ok = (xdim == wantw) && (ydim == wanth);
    printf("internal size %dx%d, %s: path %s, drawn at %dx%d (drawable %dx%d)\n", (int) w, (int) h, what,
           gl_pathname(gldrawpath), (int) xdim, (int) ydim, dw, dh);
    if (!ok) fprintf(stderr, "FAIL: internal size %dx%d, %s: drawn at %dx%d, not %dx%d\n", (int) w, (int) h, what,
                     (int) xdim, (int) ydim, (int) wantw, (int) wanth);
    return ok;
}

/*
 * The shader path at an internal size (GLInternalWidth/Height 320x200):
 *  xdim/ydim are that size; its frames (3D view with the HUD, a 2D-only
 *  screen drawn after a frame is shown) are within 8 of renderer 0's at
 *  320x200; the window shows them scaled by the largest factor that fits,
 *  nearest pixel, centred with black bars; screenshots are the internal
 *  frame; a window resize keeps the size and shows the frame again scaled
 *  to the new window; the mouse area is the frame's part of the window. A
 *  width without a height draws at the window's size, a size under 320x200
 *  at 320x200; the vbo path, and the shader path when it fails, ignore it.
 *  1: pass, 0: fail, 77: no shader path.
 */
static int internal_frames(const char *dir)
{
    unsigned char *classic[INFRAMES], *cap, *shown;
    char path[1024];
    int f, ok = 1, dw, dh, winw, winh, ww, wh, mw, mh, oldw;
    int32_t r[4];
    long bars, i;
    double d, sd;
    Uint32 t0;
    SDL_Surface *s;

    /* renderer 0 at 320x200 (its window 640x400, as the GL window's) */
    if (_setgamemode(1, 320, 200) != 0) { fprintf(stderr, "FAIL: _setgamemode(320x200)\n"); return 0; }
    polymosttexfullbright = 240;
    for (f = 0; f < INFRAMES; f++)
    {
        internal_frame(f);
        classic[f] = malloc(320 * 200 * 3);
        for (i = 0; i < 320 * 200; i++)
        {
            SDL_Color *c = &surface->format->palette->colors[((unsigned char *) surface->pixels)[(i / 320) * surface->pitch + i % 320]];
            classic[f][i * 3] = c->r; classic[f][i * 3 + 1] = c->g; classic[f][i * 3 + 2] = c->b;
        }
        s = SDL_CreateRGBSurfaceWithFormatFrom(classic[f], 320, 200, 24, 320 * 3, SDL_PIXELFORMAT_RGB24);
        snprintf(path, sizeof (path), "%s/internal%d_r0.bmp", dir, f);
        SDL_SaveBMP(s, path);
        SDL_FreeSurface(s);
    }

    if (!internal_mode(GLPATH_SHADER, 320, 200, 0, 320, 200, "shader"))
    {
        if (gldrawpath != GLPATH_SHADER) { printf("internal size: no shader path, skipped\n"); ok = 77; }
        else ok = 0;
        goto done;
    }
    setbrightness(0, palette);
    for (f = 0; f < INFRAMES; f++)
    {
        internal_frame(f);
        snprintf(path, sizeof (path), "%s/internal%d_shader0000.bmp", dir, f);
        remove(path);
        snprintf(path, sizeof (path), "%s/internal%d_shaderxxxx.bmp", dir, f);
        i = screencapture(path, 0);
        snprintf(path, sizeof (path), "%s/internal%d_shader0000.bmp", dir, f);
        s = (i == 0) ? SDL_LoadBMP(path) : NULL;
        if ((s == NULL) || (s->w != 320) || (s->h != 200))
            { fprintf(stderr, "FAIL: a screenshot at an internal size isn't 320x200\n"); ok = 0; }
        SDL_FreeSurface(s);
        cap = polymost_capture_shader();
        shown = read_shown(&dw, &dh);
        d = rgb_diff(cap, classic[f], 320, 200);
        sd = shown_diff(shown, dw, dh, cap, 320, 200, &bars);
        gl_fitrect(320, 200, dw, dh, r);
        printf("internal size 320x200, frame %d: mean |shader - r0| %.2f (most 8.00); shown at %d,%d %dx%d "
               "of %dx%d, mean |shown - frame| %.2f, %ld bar pixels not black\n",
               f, d, (int) r[0], (int) r[1], (int) r[2], (int) r[3], dw, dh, sd, bars);
        if ((d > 8) || (sd > 0.5) || bars) ok = 0;
        free(cap);
        free(shown);
    }

    /* a resize: the same size, shown again scaled to the new window */
    cap = polymost_capture_shader();
    SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &winw, &winh);
    SDL_GL_GetDrawableSize(SDL_GL_GetCurrentWindow(), &oldw, &dh);
    SDL_SetWindowSize(SDL_GL_GetCurrentWindow(), 800, 520);
    for (t0 = SDL_GetTicks(), dw = oldw; (dw == oldw) && (SDL_GetTicks() - t0 < 3000); SDL_Delay(10))
    {
        _handle_events();
        SDL_GL_GetDrawableSize(SDL_GL_GetCurrentWindow(), &dw, &dh);
    }
    _handle_events();
    shown = read_shown(&dw, &dh);
    sd = shown_diff(shown, dw, dh, cap, 320, 200, &bars);
    getmousearea(&mw, &mh);
    gl_fitrect(320, 200, dw, dh, r);
    SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &ww, &wh);
    printf("internal size 320x200 after a resize to %dx%d: drawn at %dx%d, shown %dx%d, mean |shown - frame| %.2f, "
           "%ld bar pixels not black; mouse area %dx%d of window %dx%d\n", dw, dh, (int) xdim, (int) ydim,
           (int) r[2], (int) r[3], sd, bars, mw, mh, ww, wh);
    if ((dw == oldw) || (xdim != 320) || (ydim != 200) || (sd > 0.5) || bars)
        { fprintf(stderr, "FAIL: a resize at an internal size\n"); ok = 0; }
    if ((abs(mw - (int) ((int64_t) r[2] * ww / dw)) > 1) || (abs(mh - (int) ((int64_t) r[3] * wh / dh)) > 1))
        { fprintf(stderr, "FAIL: the mouse area isn't the frame's part of the window\n"); ok = 0; }
    free(cap);
    free(shown);
    for (f = 0; f < 2; f++)  /* the bars are black whatever the window's back buffer held */
    {
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        rotatesprite(0, (200 - tilesizy[BOTTOMSTATUSBAR])<<16, 65536, 0, BOTTOMSTATUSBAR, 4, 0,
                     2+8+16+64, 0, 0, xdim-1, ydim-1);
        nextpage();
    }
    cap = polymost_capture_shader();
    shown = read_shown(&dw, &dh);
    sd = shown_diff(shown, dw, dh, cap, 320, 200, &bars);
    printf("internal size 320x200 over a white back buffer: mean |shown - frame| %.2f, %ld bar pixels not black\n",
           sd, bars);
    if ((sd > 0.5) || bars) { fprintf(stderr, "FAIL: the bars at an internal size\n"); ok = 0; }
    free(cap);
    free(shown);
    SDL_SetWindowSize(SDL_GL_GetCurrentWindow(), winw, winh);
    if (glGetError() != GL_NO_ERROR) { fprintf(stderr, "FAIL: GL error at an internal size\n"); ok = 0; }

    /* the setting's checks, and the paths that ignore it */
    ok &= internal_mode(GLPATH_SHADER, 320, 0, 0, 0, 0, "no height");
    ok &= internal_mode(GLPATH_SHADER, 100, 50, 0, 320, 200, "too small");
    ok &= internal_mode(GLPATH_VBO, 320, 200, 0, 0, 0, "vbo");
    ok &= internal_mode(GLPATH_SHADER, 320, 200, 3, 0, 0, "incomplete framebuffer");
    if (glGetError() != GL_NO_ERROR) { fprintf(stderr, "FAIL: GL error at an internal size\n"); ok = 0; }

done:
    glinternalwidth = glinternalheight = 0;
    gldrawpathrequest = GLPATH_AUTO;
    gl_forcefail = 0;
    polymosttexfullbright = 256;
    for (f = 0; f < INFRAMES; f++) free(classic[f]);
    if (!ok) fprintf(stderr, "FAIL: the shader path at an internal size\n");
    return ok;
}

/* An 8-bit frame of every palette index, each pixel unlike its neighbours
 *  (so filtering shows), shown with the game's palette (c 0), faded to red
 *  (1) or at brightness 8 (2). */
static void present8_frame(int c)
{
    int32_t x, y;

    setbrightness((c == 2) ? 8 : 0, palette);
    for (y = 0; y < ydim; y++)
        for (x = 0; x < xdim; x++)
            ((unsigned char *) frameplace)[y * bytesperline + x] = (unsigned char) (x + y * 3);
    if (c == 1) setpalettefade(63, 0, 0, 32);
    nextpage();
}

/* The window's picture (*ow by *oh, bottom-up rows): through the SDL
 *  renderer before OpenGL (sdl), else the context's front buffer. */
static unsigned char *present8_read(SDL_Window *win, int sdl, int *ow, int *oh)
{
    SDL_Renderer *r;
    unsigned char *buf, *tmp;
    int y;

    if (!sdl) return read_shown(ow, oh);
    nextpage();  /* the same again: the read is of the back buffer, the frame before */
    r = SDL_GetRenderer(win);
    SDL_RenderSetLogicalSize(r, 0, 0);  /* all of it, bars too */
    SDL_GetRendererOutputSize(r, ow, oh);
    buf = malloc((size_t) *ow * *oh * 3);
    tmp = malloc((size_t) *ow * 3);
    SDL_RenderReadPixels(r, NULL, SDL_PIXELFORMAT_RGB24, buf, *ow * 3);
    SDL_RenderSetLogicalSize(r, xdim, ydim);
    for (y = 0; y < *oh / 2; y++)
    {
        memcpy(tmp, buf + (size_t) y * *ow * 3, *ow * 3);
        memcpy(buf + (size_t) y * *ow * 3, buf + (size_t) (*oh - 1 - y) * *ow * 3, *ow * 3);
        memcpy(buf + (size_t) (*oh - 1 - y) * *ow * 3, tmp, *ow * 3);
    }
    free(tmp);
    return buf;
}

/* The frame through the 8-bit surface's palette, top-down RGB. */
static unsigned char *present8_expected(void)
{
    const SDL_Color *pal = surface->format->palette->colors;
    unsigned char *rgb = malloc((size_t) xdim * ydim * 3), *p = rgb;
    long i;

    for (i = 0; i < (long) xdim * ydim; i++, p += 3)
    {
        const SDL_Color *c = &pal[((unsigned char *) frameplace)[i]];
        p[0] = c->r; p[1] = c->g; p[2] = c->b;
    }
    return rgb;
}

/*
 * One OpenGL context for the run, with the 8-bit modes shown through it.
 *  The frames of present8_frame, shown by the SDL renderer before OpenGL
 *  is set and through the context after it (the 8-bit mode set again), each
 *  read back from the window, are the frame through the 8-bit palette,
 *  scaled to fit, nearest pixel, with black bars: in the 640x400 window
 *  and in one resized to 900x400. Renderer 0 -> 3 -> 0 -> 3 keeps the
 *  window and the context, current. 1: pass, 0: fail, 77: no OpenGL.
 */
#define P8CASES 3
static int present8_check(void)
{
    static const char *const casename[P8CASES] = { "palette", "fade", "brightness 8" };
    SDL_Window *win = NULL;
    SDL_GLContext ctx;
    Uint32 winid;
    unsigned char *shown, *want;
    int c, gl, wide, ow, oh, ok = 1;
    long bars;
    double d[2][2][P8CASES];
    Uint32 id;

    for (gl = 0; gl < 2; gl++)
    {
        if (_setgamemode(1, 320, 200) != 0) { fprintf(stderr, "FAIL: _setgamemode(320x200)\n"); return 0; }
        if ((gl) && (SDL_GL_GetCurrentContext() == NULL))
            { fprintf(stderr, "FAIL: an 8-bit mode after OpenGL dropped the context\n"); return 0; }
        if (!gl)
            for (id = 1; (id < 1000) && (win == NULL); id++)
                win = SDL_GetWindowFromID(id);
        for (wide = 0; wide < 2; wide++)
        {
            if (wide)
                SDL_SetWindowSize(win, 900, 400);
            for (id = SDL_GetTicks(); SDL_GetTicks() - id < 300; SDL_Delay(10))  /* shown, at its size */
                _handle_events();
            for (c = 0; c < P8CASES; c++)
            {
                present8_frame(c);
                want = present8_expected();
                shown = present8_read(win, !gl, &ow, &oh);
                d[gl][wide][c] = shown_diff(shown, ow, oh, want, xdim, ydim, &bars);
                printf("8-bit present, %s, %s, %dx%d window: mean |diff| %.3f, %ld bar pixels not black\n",
                       gl ? "OpenGL" : "SDL renderer", casename[c], ow, oh, d[gl][wide][c], bars);
                if ((d[gl][wide][c] > 0.5) || (bars != 0))
                {
                    fprintf(stderr, "FAIL: 8-bit present, %s, %s, %dx%d: diff %.3f, %ld bar pixels\n",
                            gl ? "OpenGL" : "SDL renderer", casename[c], ow, oh, d[gl][wide][c], bars);
                    ok = 0;
                }
                free(shown);
                free(want);
            }
        }
        if (gl) break;
        if (setgamemode_gl(1) != 0)
        {
            printf("SKIP: no OpenGL (%s)\n", SDL_GetError());
            return 77;
        }
        win = SDL_GL_GetCurrentWindow();
    }

    /* 0 -> 3 -> 0 -> 3: the same window and context throughout. */
    ctx = SDL_GL_GetCurrentContext();
    winid = SDL_GetWindowID(win);
    for (c = 0; c < 2; c++)
    {
        if ((setgamemode_gl(1) != 0) || (getrendermode() != 3))
            { fprintf(stderr, "FAIL: renderer 3 again\n"); return 0; }
        if ((SDL_GL_GetCurrentContext() != ctx) || (SDL_GL_GetCurrentWindow() == NULL) ||
            (SDL_GetWindowID(SDL_GL_GetCurrentWindow()) != winid))
            { fprintf(stderr, "FAIL: renderer 3 again made a new window or context\n"); ok = 0; }
        if (_setgamemode(1, 320, 200) != 0) { fprintf(stderr, "FAIL: _setgamemode(320x200)\n"); return 0; }
        if ((SDL_GL_GetCurrentContext() != ctx) || (SDL_GetWindowFromID(winid) == NULL) ||
            (SDL_GetRenderer(SDL_GetWindowFromID(winid)) != NULL))
            { fprintf(stderr, "FAIL: an 8-bit mode after OpenGL dropped the window or context\n"); ok = 0; }
    }
    printf("renderer 0 -> 3 -> 0 -> 3: window %u, context %p kept: %s\n", (unsigned) winid, ctx,
           ok ? "yes" : "no");
    return ok;
}

/* A white frame in the renderer in effect, shown. */
static void fade_frame(void)
{
    if (getrendermode() == 3)
        clearallviews(31);
    else
        memset((void *) frameplace, 31, (size_t) bytesperline * ydim);
    nextpage();
}

static double shown_mean(const unsigned char *shown, int w, int h)
{
    double sum = 0;
    long i;

    for (i = 0; i < (long) w * h * 3; i++)
        sum += shown[i];
    return sum / ((double) w * h * 3);
}

/*
 * The VIDEO page's fade around a change (game.c video_request*): a request
 *  for the renderer in effect (the fade alone), drawn and read for 1.5 s of
 *  real time, over a game fade (63, 0, 0, 16). Each frame read is the
 *  white frame darkened by the driver's getvideofade()/256; it goes to
 *  black and back through the levels between, and ends the picture it was.
 *  The game's fade, curpalette and the 8-bit palette are the same at black
 *  and after. midw: a mode (windowed midw by midh) asked for 100 ms in,
 *  made at black with the first. sdl: read through the SDL renderer (whose
 *  read can lag a frame, so a looser match). 1: pass.
 */
static int video_fade_run(SDL_Window *win, int sdl, int midw, int midh, const char *what)
{
    unsigned char *before, *shown, fadedelta = 0;
    palette_t pal[256], fadergb;
    SDL_Color spal[256];
    int ow, oh, bw, bh, ok = 1, r = getrendermode(), asked = 0, black = 0, out = 0, in = 0, n = 0;
    int32_t w0, h0, w1, h1, madeat = -1;
    double base, ratio, err, worst = 0, least = 1;
    Uint32 t0;

    setpalettefade(63, 0, 0, 16);
    fade_frame();
    before = present8_read(win, sdl, &bw, &bh);
    base = shown_mean(before, bw, bh);
    fadergb = palfadergb;
    fadedelta = palfadedelta;
    memcpy(pal, curpalette, sizeof (pal));
    if (r != 3) memcpy(spal, surface->format->palette->colors, sizeof (spal));

    video_requestrenderer(r);
    for (t0 = SDL_GetTicks(); SDL_GetTicks() - t0 < 1500; n++)
    {
        if ((midw) && (!asked) && (SDL_GetTicks() - t0 >= 100))
            { video_requestmode(DISPLAY_WINDOWED, midw, midh); asked = 1; }
        fade_frame();
        video_getmode(&w0, &h0);
        if (getvideofade() == 256)  /* the game's state as it was, under the black */
        {
            black++;
            if ((palfadedelta != fadedelta) || (memcmp(&palfadergb, &fadergb, sizeof (fadergb)) != 0) ||
                (memcmp(curpalette, pal, sizeof (pal)) != 0))
                { fprintf(stderr, "FAIL: %s: the game's palette or fade changed during the fade\n", what); ok = 0; }
        }
        video_applypending();
        video_getmode(&w1, &h1);
        if ((w1 != w0) || (h1 != h0)) madeat = getvideofade();
        shown = present8_read(win, sdl, &ow, &oh);
        ratio = shown_mean(shown, ow, oh) / base;
        free(shown);
        err = fabs(ratio - (double) (256 - getvideofade()) / 256.0);
        if (err > worst) worst = err;
        if (ratio < least) least = ratio;
        if ((ratio > 0.2) && (ratio < 0.8)) { if (black) in++; else out++; }
    }
    if ((midw) && ((w1 != midw) || (h1 != midh) || (madeat != 256)))
        { fprintf(stderr, "FAIL: %s: the mode asked for during the fade: %dx%d, made at darkness %d\n",
                  what, (int) w1, (int) h1, (int) madeat); ok = 0; }

    fade_frame();
    shown = present8_read(win, sdl, &ow, &oh);
    printf("%s: %d frames, %d black, %d fading out and %d in between 0.2 and 0.8, darkest %.3f, "
           "worst |shown - darkness| %.3f, fading %d after\n", what, n, black, out, in, least, worst, videofading());
    if ((!black) || (!out) || (!in) || (least > 0.02) || (worst > (sdl ? 0.07 : 0.02)) || (videofading()))
        { fprintf(stderr, "FAIL: %s: the frame didn't fade to black and back with the darkness\n", what); ok = 0; }
    if ((!midw) && ((ow != bw) || (oh != bh) || (memcmp(shown, before, (size_t) bw * bh * 3) != 0)))
        { fprintf(stderr, "FAIL: %s: the picture after the fade isn't the one before\n", what); ok = 0; }
    if ((palfadedelta != fadedelta) || (memcmp(&palfadergb, &fadergb, sizeof (fadergb)) != 0) ||
        ((!midw) && (memcmp(curpalette, pal, sizeof (pal)) != 0)) ||
        ((r != 3) && (!midw) && (memcmp(spal, surface->format->palette->colors, sizeof (spal)) != 0)))
        { fprintf(stderr, "FAIL: %s: the game's palette or fade changed\n", what); ok = 0; }
    free(shown);
    free(before);
    setpalettefade(0, 0, 0, 0);
    return ok;
}

/*
 * The VIDEO page's renderer switch (game.c video_setrenderer) in a run that
 *  starts in renderers 0-2, so in its own process (render_gl_video). OpenGL
 *  failing (gl_forcefail 4) keeps the renderer, the SDL renderer's window
 *  and its picture, and isn't tried again; then 3 -> 0 -> 3 -> 0 -> 3 makes
 *  the GL window once, keeping it and the context; the player's palette and
 *  fade survive each switch. 1: pass, 0: fail, 77: no OpenGL.
 */
static int video_switch_check(void)
{
    static const int seq[] = { 0, 3, 0, 3 };
    SDL_Window *win = NULL;
    SDL_GLContext ctx;
    Uint32 id, winid;
    unsigned char *shown, *want;
    int ow, oh, i, r, ok = 1;
    long bars;
    double d;

    videoswitch_setup();
    if (video_setmode(DISPLAY_WINDOWED, 320, 200) != 0) { fprintf(stderr, "FAIL: video_setmode(320x200)\n"); return 0; }
    if (video_setrenderer(2) != 2) { fprintf(stderr, "FAIL: renderer 2\n"); return 0; }
    for (id = 1; (id < 1000) && (win == NULL); id++)
        win = SDL_GetWindowFromID(id);
    winid = SDL_GetWindowID(win);

    gl_forcefail = 4;
    r = video_setrenderer(3);
    gl_forcefail = 0;
    printf("renderer 2 -> 3, OpenGL failing: renderer %d, OpenGL greyed %d\n", r, (int) video_glunavailable);
    if ((r != 2) || (getrendermode() != 2) || (!video_glunavailable))
        { fprintf(stderr, "FAIL: OpenGL failing didn't keep renderer 2 and grey OpenGL\n"); ok = 0; }
    if ((SDL_GetWindowFromID(winid) != win) || (SDL_GetRenderer(win) == NULL))
        { fprintf(stderr, "FAIL: OpenGL failing lost the window or its SDL renderer\n"); return 0; }
    for (id = SDL_GetTicks(); SDL_GetTicks() - id < 300; SDL_Delay(10))
        _handle_events();
    present8_frame(0);
    want = present8_expected();
    shown = present8_read(win, 1, &ow, &oh);
    d = shown_diff(shown, ow, oh, want, xdim, ydim, &bars);
    printf("after the failure, %dx%d window: mean |diff| %.3f, %ld bar pixels not black\n", ow, oh, d, bars);
    if ((d > 0.5) || (bars != 0))
        { fprintf(stderr, "FAIL: after OpenGL failed the 8-bit picture isn't shown\n"); ok = 0; }
    free(shown);
    free(want);
    if (video_setrenderer(0) != 0) { fprintf(stderr, "FAIL: renderer 0\n"); return 0; }
    if (!video_fade_run(win, 1, 0, 0, "fade, renderer 0 through the SDL renderer")) ok = 0;
    video_setrenderer(2);
    if (video_setrenderer(3) != 2)
        { fprintf(stderr, "FAIL: OpenGL tried again after it failed\n"); ok = 0; }

    video_glunavailable = 0;  /* as on a machine that has it */
    if (video_setrenderer(3) != 3)
    {
        printf("SKIP: no OpenGL (%s)\n", SDL_GetError());
        return 77;
    }
    ctx = SDL_GL_GetCurrentContext();
    win = SDL_GL_GetCurrentWindow();
    winid = SDL_GetWindowID(win);
    if ((ctx == NULL) || (SDL_GetRenderer(win) != NULL))
        { fprintf(stderr, "FAIL: renderer 3 isn't in a GL window\n"); return 0; }
    for (i = 0; i < (int) (sizeof (seq) / sizeof (seq[0])); i++)
    {
        r = video_setrenderer(seq[i]);
        if ((r != seq[i]) || (getrendermode() != seq[i]))
            { fprintf(stderr, "FAIL: switch to renderer %d gave %d\n", seq[i], r); ok = 0; }
        if ((SDL_GL_GetCurrentContext() != ctx) || (SDL_GetWindowFromID(winid) == NULL) ||
            (SDL_GL_GetCurrentWindow() != SDL_GetWindowFromID(winid)))
            { fprintf(stderr, "FAIL: switch to renderer %d made a new window or context\n", seq[i]); ok = 0; }
    }
    printf("renderer 3 -> 0 -> 3 -> 0 -> 3: window %u, context %p kept: %s\n", (unsigned) winid, ctx,
           ok ? "yes" : "no");
    if (!videoswitch_palette_check()) ok = 0;
    return ok;
}

/* BUILD_WINDOWED set or not, portably. */
static void set_build_windowed(int on)
{
#if PLATFORM_WIN32
    _putenv_s(BUILD_WINDOWED, on ? "1" : "");
#else
    if (on)
        setenv(BUILD_WINDOWED, "1", 1);
    else
        unsetenv(BUILD_WINDOWED);
#endif
}

/*
 * The VIDEO page's resolution and display mode (game.c video_setmode) in
 *  renderer 3: each keeps the window and the context, xdim/ydim follow the
 *  drawable, and a frame fills it (no black left after clearing to white).
 *  In renderer 0 the 8-bit frame is drawn at the size asked for (borderless:
 *  the desktop's), shown through the same context, filling it too. A mode
 *  set at the same size leaves the window where it is. 1: pass, 0: fail.
 */
static int video_mode_check(void)
{
    static const struct { int mode, w, h, r; } seq[] = {
        { DISPLAY_WINDOWED, 800, 600, 3 }, { DISPLAY_WINDOWED, 1024, 576, 3 },
        { DISPLAY_BORDERLESS, 640, 480, 3 }, { DISPLAY_WINDOWED, 640, 400, 3 },
        { DISPLAY_WINDOWED, 800, 600, 0 }, { DISPLAY_BORDERLESS, 640, 480, 0 },
        { DISPLAY_WINDOWED, 1024, 576, 3 }, { DISPLAY_WINDOWED, 1024, 576, 0 },
        { DISPLAY_WINDOWED, 1024, 576, 3 }
    };
    SDL_GLContext ctx = SDL_GL_GetCurrentContext();
    SDL_Window *win = SDL_GL_GetCurrentWindow();
    Uint32 id, winid = SDL_GetWindowID(win);
    unsigned char *shown;
    int i, k, ok = 1, dw, dh, ww, wh, x, y, x2, y2, same, white = 31;
    long dark;

    set_build_windowed(0);  /* so borderless is */
    for (i = 0; i < (int) (sizeof (seq) / sizeof (seq[0])); i++)
    {
        same = ((i > 0) && (seq[i].mode == DISPLAY_WINDOWED) && (seq[i - 1].mode == DISPLAY_WINDOWED) &&
                (seq[i].w == seq[i - 1].w) && (seq[i].h == seq[i - 1].h));
        if (same)  /* moved by the player */
        {
            SDL_SetWindowPosition(win, 60, 80);
            for (id = SDL_GetTicks(); SDL_GetTicks() - id < 300; SDL_Delay(10))
                _handle_events();
        }
        SDL_GetWindowPosition(win, &x, &y);
        if (video_setrenderer(seq[i].r) != seq[i].r)
            { fprintf(stderr, "FAIL: renderer %d\n", seq[i].r); ok = 0; }
        if (video_setmode(seq[i].mode, seq[i].w, seq[i].h) != 0)
            { fprintf(stderr, "FAIL: video_setmode(%d, %dx%d)\n", seq[i].mode, seq[i].w, seq[i].h); return 0; }
        for (id = SDL_GetTicks(); SDL_GetTicks() - id < 1500; SDL_Delay(10))  /* the new size shown */
            _handle_events();
        if ((getrendermode() != seq[i].r) || (SDL_GL_GetCurrentContext() != ctx) ||
            (SDL_GetWindowFromID(winid) != win) || (SDL_GL_GetCurrentWindow() != win))
            { fprintf(stderr, "FAIL: %s %dx%d made a new window or context\n",
                      display_modename(seq[i].mode), seq[i].w, seq[i].h); ok = 0; }
        if (getvideodisplaymode() != seq[i].mode)
            { fprintf(stderr, "FAIL: display %d, not %d\n", getvideodisplaymode(), seq[i].mode); ok = 0; }
        SDL_GetWindowSize(win, &ww, &wh);
        SDL_GL_GetDrawableSize(win, &dw, &dh);
        if ((seq[i].mode == DISPLAY_WINDOWED) && ((ww != seq[i].w) || (wh != seq[i].h)))
            { fprintf(stderr, "FAIL: window %dx%d, not %dx%d\n", ww, wh, seq[i].w, seq[i].h); ok = 0; }
        if ((seq[i].r == 3) && ((xdim != min(dw, MAXXDIM)) || (ydim != min(dh, MAXYDIM))))
            { fprintf(stderr, "FAIL: drawn at %dx%d in a %dx%d drawable\n", (int) xdim, (int) ydim, dw, dh); ok = 0; }
        if ((seq[i].r == 0) && (seq[i].mode == DISPLAY_WINDOWED) && ((xdim != seq[i].w) || (ydim != seq[i].h)))
            { fprintf(stderr, "FAIL: renderer 0 drawn at %dx%d, not %dx%d\n", (int) xdim, (int) ydim, seq[i].w, seq[i].h); ok = 0; }
        if (same)
        {
            SDL_GetWindowPosition(win, &x2, &y2);
            printf("window at %d,%d before the renderer switch and mode set, %d,%d after\n", x, y, x2, y2);
            if ((x2 != x) || (y2 != y))
                { fprintf(stderr, "FAIL: the window moved from %d,%d to %d,%d\n", x, y, x2, y2); ok = 0; }
        }

        for (k = 0; k < 2; k++)  /* the front buffer: the frame drawn */
        {
            if (getrendermode() == 3)
                clearallviews(white);
            else  /* classic's clears to colour 0 */
                memset((void *) frameplace, white, (size_t) bytesperline * ydim);
            nextpage();
        }
        shown = read_shown(&dw, &dh);
        for (dark = 0, k = 0; k < dw * dh; k++)
            if ((shown[k * 3] | shown[k * 3 + 1] | shown[k * 3 + 2]) == 0)
                dark++;
        free(shown);
        printf("renderer %d, %s %dx%d: window %dx%d, drawable %dx%d, drawn at %dx%d, %ld black pixels\n",
               seq[i].r, display_modename(getvideodisplaymode()), seq[i].w, seq[i].h, ww, wh, dw, dh,
               (int) xdim, (int) ydim, dark);
        if (dark != 0)
            { fprintf(stderr, "FAIL: the frame doesn't fill the window\n"); ok = 0; }
    }

    printf("renderer 3 draws on the %s path\n", gl_pathname(gldrawpath));
    if (!video_fade_run(win, 0, 800, 600, "fade, renderer 3, 800x600 asked for during it")) ok = 0;
    GLDrawPath = GLPATH_VBO;  /* the fixed function tiers' fade quad */
    video_setmode(DISPLAY_WINDOWED, 800, 600);
    printf("renderer 3 draws on the %s path\n", gl_pathname(gldrawpath));
    if (!video_fade_run(win, 0, 0, 0, "fade, renderer 3 (vbo)")) ok = 0;
    GLDrawPath = GLPATH_AUTO;
    video_setrenderer(0);
    if (!video_fade_run(win, 0, 0, 0, "fade, renderer 0 through OpenGL")) ok = 0;
    set_build_windowed(1);
    return ok;
}

int main(int argc, char **argv)
{
    int32_t posx, posy, posz;
    short ang, sectnum;
    char path[1024];
    double r0, g0, r1, g1, capr, capg;
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
    unsigned char *palshot[3];
    palette_t savedpal[256], savedfade;
    unsigned char savedfadedelta;
    int32_t oldxdim, oldydim;
    int lfnpot, lfpot, resized, palkept, palsame, winw, winh;
    unsigned char *buf, *prev, cleared[2];
    double palwater, resizeddiff;
    Uint32 t0;
    short floorspr, unlit, litspr;
    unsigned char *map512, *over[2], *artcopy, *glowshot[2];
    double mapdiff512, maptrans[2], glow[3];
    static const char *glowname[3] = { "neither", "masked", "translucent" };
    int overtop, startw, starth, artsize, gamepal, askh;
    SDL_Rect usable;
    static const struct { int32_t path, force, mode; const char *name; } paths[] = {
        { GLPATH_IMMEDIATE, GLSUB_IMMEDIATE, GLSUB_IMMEDIATE, "immediate" },
        { GLPATH_IMMEDIATE, GLSUB_ARRAYS, GLSUB_ARRAYS, "immediate, vertex arrays" },
        { GLPATH_VBO, -1, GLSUB_VBO, "vbo" },
    };
    unsigned char *pathshot[2][PATHSHOTS], *shshot[3][PATHSHOTS], *ref, *mirror[2], *small[2];
    uint32_t copies;
    double smalld;
    double shfade[2][2];
    int p, s, pathdiffer;
    long off;
    double d, d2;
    uint32_t draws;
    int32_t refxdim = 0, refydim = 0;

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
    if ((argc > 3) && (strcmp(argv[3], "video") == 0))
    {
        i = video_switch_check();
        if (i == 1) i = video_mode_check();
        return (i == 1) ? 0 : (i == 77) ? 77 : 1;
    }
    i = present8_check();
    if (i != 1) return (i == 77) ? 77 : 1;
    i = classic_compare(argv[2]);
    if (i != 1) return (i == 77) ? 77 : 1;
    if (_setgamemode(1, 320, 200) != 0) { fprintf(stderr, "FAIL: _setgamemode(320x200)\n"); return 1; }
    /* The checks up to the draw paths' are of the fixed function tiers (the
     * glow pass, mip levels, retexturing); the shader path's come after. */
    gldrawpathrequest = GLPATH_VBO;
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
    /* A screenshot has the fade, as the screen does (and as in 8-bit). */
    snprintf(path, sizeof (path), "%s/capfade0000.bmp", argv[2]);
    remove(path);
    snprintf(path, sizeof (path), "%s/capfadexxxx.bmp", argv[2]);
    if (screencapture(path, 0) != 0) { fprintf(stderr, "FAIL: screencapture of the fade\n"); return 1; }
    snprintf(path, sizeof (path), "%s/capfade0000.bmp", argv[2]);
    bmp_mean(path, &capr, &capg);
    setpalettefade(0, 0, 0, 0);
    snprintf(path, sizeof (path), "%s/unfade.bmp", argv[2]);
    save_front(path, &r0, &g0);
    printf("mean red/green: faded %.1f/%.1f, unfaded %.1f/%.1f, faded screenshot %.1f/%.1f\n",
           r1, g1, r0, g0, capr, capg);

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

    /* clearview clears only the view window, as in classic (the border
     * around a small view stays). */
    clearallviews(31);
    setview(xdim/4, ydim/4, xdim*3/4-1, ydim*3/4-1);
    clearview(0);
    setview(0, 0, xdim-1, ydim-1);
    nextpage();
    buf = malloc((size_t) xdim * ydim * 3);
    read_front(buf);
    cleared[0] = buf[(2 * xdim + 2) * 3];  /* bottom left: border */
    cleared[1] = buf[((ydim/2) * xdim + xdim/2) * 3];  /* middle: view */
    free(buf);
    printf("clearview: border red %d, view red %d\n", cleared[0], cleared[1]);

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
    floorspr = i = insertsprite(sectnum, 0);  /* render_classic2d's translucent floor sprite */
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
    /* The floor sprite more see-through (cstat 512), then a 2D picture over
     * the map view, which must come out as over a cleared screen. */
    sprite[floorspr].cstat = 32|2|512;
    clearview(0);
    drawmapview(posx, posy, 2048, ang);
    nextpage();
    map512 = malloc((size_t) xdim * ydim * 3);
    read_front(map512);
    mapdiff512 = front_diff(map512, map[hasnpot]);
    sprite[floorspr].cstat = 32|2;
    /* A sector translucent in 3D (floorstat 256, 384) is filled opaque in the
     * map view, as classic's drawmapview does: the same frame. */
    for (i = 0; i < 2; i++)
    {
        sector[sectnum].floorstat |= i ? 384 : 256;
        clearview(0);
        drawmapview(posx, posy, 2048, ang);
        nextpage();
        read_front(map512);
        maptrans[i] = front_diff(map512, map[hasnpot]);
        sector[sectnum].floorstat &= ~384;
    }
    for (i = 0; i < 2; i++)
    {
        clearview(0);
        if (i == 0) drawmapview(posx, posy, 2048, ang);
        rotatesprite(160<<16, 100<<16, 32768, 0, VICTORY1, 0, 0, 2+8+64, 0, 0, xdim-1, ydim-1);
        nextpage();
        over[i] = malloc((size_t) xdim * ydim * 3);
        read_front(over[i]);
    }
    for (i = ydim * 7 / 16, overtop = 1; i < ydim * 9 / 16; i++)
        if (memcmp(over[0] + ((size_t) i * xdim + xdim * 7 / 16) * 3,
                   over[1] + ((size_t) i * xdim + xdim * 7 / 16) * 3, (size_t) (xdim / 8) * 3) != 0)
            overtop = 0;
    if (err == GL_NO_ERROR) err = glGetError();
    for (npot = 0; npot <= hasnpot; npot++)
        printf("map view npot %d: mean red+green/2 %.1f\n", npot, mapmean[npot]);
    printf("map view: mean |floor sprite cstat 512 - 2| %.2f; a 2D picture over it %s\n",
           mapdiff512, overtop ? "is on top" : "isn't on top");
    printf("map view: mean |sector floorstat 256 - 0| %.2f, |384 - 0| %.2f\n", maptrans[0], maptrans[1]);
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
    litspr = i = insertsprite(litsectnum, 0);
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

    /* Fullbright on a masked wall and on a translucent sprite: the start
     * sector's walls and floor unlit, then (1) its two-sided walls masked
     * with LITFLOOR, or (2) the face sprite translucent, against (0)
     * neither (what else is lit in view). */
    unlit = unlit_tile();
    if (unlit < 0) { fprintf(stderr, "FAIL: no unlit tile\n"); return 1; }
    sector[sectnum].floorpicnum = sector[sectnum].ceilingpicnum = unlit;
    for (i = 0; i < numwalls; i++)
        if (litsect[sectorofwall(i)]) wall[i].picnum = unlit;
    setbrightness(0, palette);
    for (pal = 0; pal < 3; pal++)
    {
        for (i = sector[sectnum].wallptr; i < sector[sectnum].wallptr + sector[sectnum].wallnum; i++)
            if (wall[i].nextsector >= 0)
            {
                wall[i].overpicnum = LITFLOOR;
                if (pal == 1) wall[i].cstat |= 16; else wall[i].cstat &= ~16;
            }
        sprite[litspr].cstat = (pal == 2) ? 2 : (short) 32768;
        for (i = 0; i < 2; i++)
        {
            glowshot[i] = malloc((size_t) xdim * ydim * 3);
            snprintf(name, sizeof (name), "fb_%s_%s", glowname[pal], fbname[i]);
            fb_shot(fbval[i], argv[2], name, glowshot[i], posx, posy, posz, ang, sectnum);
        }
        glow[pal] = front_diff(glowshot[1], glowshot[0]);
        free(glowshot[0]);
        free(glowshot[1]);
    }
    sprite[litspr].cstat = 0;
    polymosttexfullbright = 256;
    if (err == GL_NO_ERROR) err = glGetError();
    printf("fullbright: mean |on - off| neither %.3f, masked wall %.3f, translucent sprite %.3f\n",
           glow[0], glow[1], glow[2]);

    /* A palette change through setbrightness retextures without a reset; the
     * same palette again (a flash's end) doesn't: with the unlit tile's art
     * changed in memory between, a retexture would show it. */
    artsize = tilesizx[unlit] * tilesizy[unlit];
    artcopy = malloc(artsize);
    for (i = 0; i < 3; i++)
    {
        palshot[i] = malloc((size_t) xdim * ydim * 3);
        if (i == 1)
        {
            memcpy(artcopy, (void *) waloff[unlit], artsize);
            memset((void *) waloff[unlit], 31, artsize);
        }
        setbrightness(0, (i == 2) ? waterpal : palette);
        view_shot(palshot[i], posx, posy, posz, ang, sectnum);
        if (i == 1)
            memcpy((void *) waloff[unlit], artcopy, artsize);
    }
    palsame = (memcmp(palshot[1], palshot[0], (size_t) xdim * ydim * 3) == 0);
    palwater = front_diff(palshot[2], palshot[0]);
    printf("palette: same again %s; mean |water - normal| %.2f\n",
           palsame ? "unchanged" : "changed", palwater);
    gamepal = glgamepalette_check();
    printf("game palette (glsetgamepalette): %s\n", gamepal ? "as expected" : "wrong");
    lfnpot = lastframe_same();
    prev = malloc((size_t) xdim * ydim * 3);
    read_front(prev);  /* the last frame, unfaded */
    if (err == GL_NO_ERROR) err = glGetError();

    /* A window resize (as a fullscreen toggle) underwater, mid-fade: the
     * palette and fade survive. The last-frame texture is made again, this
     * time without non-power-of-two textures. */
    setpalettefade(0, 0, 63, 16);
    memcpy(savedpal, curpalette, sizeof (savedpal));
    savedfade = palfadergb;
    savedfadedelta = palfadedelta;
    oldxdim = xdim;
    oldydim = ydim;
    SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &startw, &starth);
    glinfo.texnpot = 0;
    polymost_glreset();  /* textures keep their size: remake them (the resize does too, if it happens) */
    SDL_SetWindowSize(SDL_GL_GetCurrentWindow(), 800, 520);
    for (t0 = SDL_GetTicks(); (xdim == oldxdim) && (SDL_GetTicks() - t0 < 3000); SDL_Delay(10))
        _handle_events();
    resized = (xdim != oldxdim);
    palkept = ((memcmp(savedpal, curpalette, sizeof (savedpal)) == 0) &&
               (memcmp(&savedfade, &palfadergb, sizeof (savedfade)) == 0) &&
               (palfadedelta == savedfadedelta));
    printf("resize: %dx%d to %dx%d; palette and fade %s\n", (int) oldxdim, (int) oldydim,
           (int) xdim, (int) ydim, palkept ? "kept" : "lost");
    /* Nothing was being drawn (as on a still screen): the last frame is
     * still there to fade, stretched to the new size, not black. */
    setpalettefade(0, 0, 0, 0);
    snprintf(path, sizeof (path), "%s/resized.bmp", argv[2]);
    save_front(path, &rs, &gs);
    buf = malloc((size_t) xdim * ydim * 3);
    read_front(buf);
    resizeddiff = stretch_diff(prev, oldxdim, oldydim, buf);
    free(buf);
    free(prev);
    printf("after the resize, unfaded: mean |frame - last frame stretched| %.2f\n", resizeddiff);
    view_shot(NULL, posx, posy, posz, ang, sectnum);
    lfpot = lastframe_same();
    glinfo.texnpot = hasnpot;
    polymost_glreset();
    /* back to the first size: the checks below use xdim and ydim. */
    SDL_SetWindowSize(SDL_GL_GetCurrentWindow(), startw, starth);
    for (t0 = SDL_GetTicks(); (xdim != oldxdim) && (SDL_GetTicks() - t0 < 3000); SDL_Delay(10))
        _handle_events();
    if ((xdim != oldxdim) || (ydim != oldydim))
        { fprintf(stderr, "FAIL: the window didn't resize back\n"); return 1; }
    printf("last frame shown again unchanged: npot %d %s, npot 0 %s\n", hasnpot,
           lfnpot ? "yes" : "no", lfpot ? "yes" : "no");
    if (err == GL_NO_ERROR) err = glGetError();

    if (err != GL_NO_ERROR) { fprintf(stderr, "FAIL: glGetError 0x%x\n", (unsigned) err); return 1; }
    if ((r1 < r0 + 64) || (g1 > g0 + 1)) { fprintf(stderr, "FAIL: the fade isn't red\n"); return 1; }
    if ((fabs(capr - r1) > 1) || (fabs(capg - g1) > 1))
        { fprintf(stderr, "FAIL: the screenshot's fade isn't the screen's\n"); return 1; }
    if ((still[1] < still[0] + 8) || (still[2] < still[1] + 8))
        { fprintf(stderr, "FAIL: the still screen doesn't fade in\n"); return 1; }
    if ((cleared[0] == curpalette[0].r) || (cleared[1] != curpalette[0].r))
        { fprintf(stderr, "FAIL: clearview clears outside the view or not inside it\n"); return 1; }
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
    if (mapdiff512 == 0)
        { fprintf(stderr, "FAIL: the map view's cstat 512 floor sprite looks as cstat 2\n"); return 1; }
    if ((maptrans[0] != 0) || (maptrans[1] != 0))
        { fprintf(stderr, "FAIL: the map view fills a sector with floorstat 256 or 384 translucent\n"); return 1; }
    if (!overtop)
        { fprintf(stderr, "FAIL: a 2D picture drawn after the map view isn't on top\n"); return 1; }
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
    if (glow[1] <= glow[0])
        { fprintf(stderr, "FAIL: fullbright doesn't brighten a masked wall\n"); return 1; }
    if (glow[2] <= glow[0])
        { fprintf(stderr, "FAIL: fullbright doesn't brighten a translucent sprite\n"); return 1; }
    if (!palsame)
        { fprintf(stderr, "FAIL: the same palette again retextures\n"); return 1; }
    if (!gamepal)
        { fprintf(stderr, "FAIL: glsetgamepalette\n"); return 1; }
    if (palwater < 1)
        { fprintf(stderr, "FAIL: the water palette doesn't retexture\n"); return 1; }
    if (!resized) { fprintf(stderr, "FAIL: the window didn't resize\n"); return 1; }
    if (!palkept) { fprintf(stderr, "FAIL: the resize lost the palette or fade\n"); return 1; }
    if (resizeddiff > 4) { fprintf(stderr, "FAIL: the last frame isn't shown again after the resize\n"); return 1; }
    if ((!lfnpot) || (!lfpot)) { fprintf(stderr, "FAIL: the last frame shows differently\n"); return 1; }

    /* Each draw path, forced, draws the same frames byte for byte as
     * immediate mode, with the masked walls and translucent sprite of the
     * fullbright checks; a path the driver lacks is skipped. Sectors get
     * different visibilities (fog densities), every other masked wall is
     * translucent, and the lit sprite gets an opaque and a reverse
     * translucent twin beside it, so that a batch drawn under the wrong
     * fog, blend or depth state shows. */
    for (i = 0; i < numsectors; i++)
        sector[i].visibility = (unsigned char) ((i * 37) & 127);
    for (i = sector[sectnum].wallptr; i < sector[sectnum].wallptr + sector[sectnum].wallnum; i++)
        if (wall[i].nextsector >= 0) wall[i].cstat |= (i & 1) ? 16|128 : 16;
    sprite[litspr].cstat = 2;
    for (s = 1; s <= 2; s++)
    {
        i = insertsprite(sprite[litspr].sectnum, 0);
        sprite[i] = sprite[litspr];
        sprite[i].x += (sintable[(ang+s*512)&2047]>>6) * 3;  /* to one side, then the other */
        sprite[i].y += (sintable[(ang+s*512+1536)&2047]>>6) * 3;
        sprite[i].cstat = (s == 1) ? 0 : 2|512;
    }
    for (s = 0; s < PATHSHOTS; s++)
    {
        pathshot[0][s] = malloc((size_t) xdim * ydim * 3);
        pathshot[1][s] = malloc((size_t) xdim * ydim * 3);
    }
    for (p = 0; p < (int) (sizeof (paths) / sizeof (paths[0])); p++)
    {
        gldrawpathrequest = paths[p].path;
        glsub_force = paths[p].force;
        if (setgamemode_gl(1) != 0)
            { fprintf(stderr, "FAIL: OpenGL with draw path %s\n", paths[p].name); return 1; }
        if (gldrawpath != paths[p].path)
        {
            printf("draw path %s: not on this driver, skipped\n", paths[p].name);
            continue;
        }
        if (p == 0) { refxdim = xdim; refydim = ydim; }
        if ((xdim != refxdim) || (ydim != refydim))
            { fprintf(stderr, "FAIL: draw path %s is %dx%d\n", paths[p].name, (int) xdim, (int) ydim); return 1; }
        setbrightness(0, palette);
        draws = glsub_arraydraws;
        path_shots(pathshot[p != 0], posx, posy, posz, ang, sectnum, oddtile);
        draws = glsub_arraydraws - draws;
        if ((err = glGetError()) != GL_NO_ERROR)
            { fprintf(stderr, "FAIL: glGetError 0x%x on draw path %s\n", (unsigned) err, paths[p].name); return 1; }
        for (s = 0, pathdiffer = 0; (p != 0) && (s < PATHSHOTS); s++)
            if (memcmp(pathshot[0][s], pathshot[1][s], (size_t) xdim * ydim * 3) != 0)
            {
                printf("draw path %s: frame %d differs from immediate, mean |difference| %.4f\n",
                       paths[p].name, s, front_diff(pathshot[1][s], pathshot[0][s]));
                pathdiffer = 1;
            }
        printf("draw path %s: %d array draws, %s\n", paths[p].name, (int) draws,
               (p == 0) ? "the reference" : (pathdiffer ? "differs" : "the same as immediate"));
        if (glsubmode != paths[p].mode)
            { fprintf(stderr, "FAIL: draw path %s sends vertices as mode %d\n", paths[p].name, (int) glsubmode); return 1; }
        if ((draws == 0) != (paths[p].mode == GLSUB_IMMEDIATE))
            { fprintf(stderr, "FAIL: draw path %s made %d array draws\n", paths[p].name, (int) draws); return 1; }
        if (pathdiffer)
            { fprintf(stderr, "FAIL: draw path %s draws differently from immediate\n", paths[p].name); return 1; }
    }
    gldrawpathrequest = GLPATH_AUTO;
    glsub_force = -1;

    /* The shader path (GL 2.1) draws 3D as palette indices, so its 3D frames
     * differ from the other paths' on purpose. Its frames must come out the
     * same with every draw flushed at once (no state change between polygons,
     * framebuffer or program switch, missing a flush) and with all the frame
     * copied for every translucent draw (no copy it reads missing what was
     * drawn under it since), with the start floor
     * and masked walls a tile without art too (its polygons write only
     * depth, the mirror hack, by a colour mask); its 2D frames (the map
     * view, a still screen) must be immediate mode's; a fade over its frame must redden the screen and a screenshot
     * work; and a shader that doesn't compile or a framebuffer that's
     * incomplete must fall back to the vbo path, drawing as it does. */
    gldrawpathrequest = GLPATH_SHADER;
    if (setgamemode_gl(1) != 0) { fprintf(stderr, "FAIL: OpenGL with draw path shader\n"); return 1; }
    if (gldrawpath != GLPATH_SHADER)
        printf("draw path shader: not on this driver, skipped\n");
    else
    {
        setbrightness(0, palette);
        for (i = 0; i < 3; i++)  /* batched, each draw flushed, all the frame copied for each translucent draw */
        {
            for (s = 0; s < PATHSHOTS; s++) shshot[i][s] = malloc((size_t) xdim * ydim * 3);
            glsub_flusheach = (i == 1);
            gl_transcopyall = (i == 2);
            draws = glsub_arraydraws;
            copies = gl_transcopies;
            path_shots(shshot[i], posx, posy, posz, ang, sectnum, oddtile);
            draws = glsub_arraydraws - draws;
            copies = gl_transcopies - copies;
            printf("draw path shader%s: %d array draws, %u copies for translucent draws in %d frames\n",
                   (i == 1) ? ", each draw flushed" : ((i == 2) ? ", all the frame copied for each translucent draw" : ""),
                   (int) draws, (unsigned) copies, PATHSHOTS);
        }
        gl_transcopyall = 0;
        floorpic = sector[sectnum].floorpicnum;
        sector[sectnum].floorpicnum = zerotile;
        for (i = sector[sectnum].wallptr; i < sector[sectnum].wallptr + sector[sectnum].wallnum; i++)
            if (wall[i].nextsector >= 0) wall[i].overpicnum = zerotile;  /* over the walls behind */
        for (i = 0; i < 2; i++)
        {
            glsub_flusheach = i;
            mirror[i] = malloc((size_t) xdim * ydim * 3);
            view_shot(mirror[i], posx, posy, posz, ang, sectnum);
        }
        glsub_flusheach = 0;
        sector[sectnum].floorpicnum = floorpic;
        for (i = sector[sectnum].wallptr; i < sector[sectnum].wallptr + sector[sectnum].wallnum; i++)
            if (wall[i].nextsector >= 0) wall[i].overpicnum = LITFLOOR;
        if ((err = glGetError()) != GL_NO_ERROR)
            { fprintf(stderr, "FAIL: glGetError 0x%x on draw path shader\n", (unsigned) err); return 1; }
        if ((glsubmode != GLSUB_ARRAYS) || (draws == 0))
            { fprintf(stderr, "FAIL: draw path shader sends vertices as mode %d\n", (int) glsubmode); return 1; }
        for (s = 0; s < PATHSHOTS; s++)
            if (memcmp(shshot[0][s], shshot[1][s], (size_t) xdim * ydim * 3) != 0)
            {
                fprintf(stderr, "FAIL: draw path shader: frame %d differs with each draw flushed, mean |difference| %.4f\n",
                        s, front_diff(shshot[0][s], shshot[1][s]));
                return 1;
            }
        for (s = 0; s < PATHSHOTS; s++)  /* the copies for translucent draws cover what they read */
            if (memcmp(shshot[0][s], shshot[2][s], (size_t) xdim * ydim * 3) != 0)
            {
                fprintf(stderr, "FAIL: draw path shader: frame %d differs with all the frame copied for each "
                        "translucent draw, mean |difference| %.4f\n", s, front_diff(shshot[0][s], shshot[2][s]));
                return 1;
            }
        if (memcmp(mirror[0], mirror[1], (size_t) xdim * ydim * 3) != 0)
        {
            fprintf(stderr, "FAIL: draw path shader: tiles without art differ with each draw flushed\n");
            return 1;
        }
        for (s = 0; s < PATHSHOTS; s++)
            if ((off = off_palette(shshot[0][s])) != 0)
            {
                fprintf(stderr, "FAIL: draw path shader: frame %d has %ld pixels not of the palette\n", s, off);
                return 1;
            }
        /* Its 2D frames come out near immediate's: the map view's floors
         * shade through the palookups (immediate's through a brightness
         * ramp), and the still screen with text is the palette's colours
         * exactly (immediate's are a shade ramp's 1.0, a unit or two off). */
        d = front_diff(shshot[0][5], pathshot[0][5]);
        d2 = front_diff(shshot[0][6], pathshot[0][6]);
        printf("draw path shader: the same with each draw flushed; all palette colours; mean |shader - immediate| "
               "map view %.2f, still screen %.2f\n", d, d2);
        if ((d > 8) || (d2 > 1))
            { fprintf(stderr, "FAIL: draw path shader: its 2D frames are far from immediate's\n"); return 1; }

        view_shot(NULL, posx, posy, posz, ang, sectnum);
        snprintf(path, sizeof (path), "%s/shader_unfaded.bmp", argv[2]);
        save_front(path, &shfade[0][0], &shfade[0][1]);
        setpalettefade(63, 0, 0, 32);
        snprintf(path, sizeof (path), "%s/shader_faded.bmp", argv[2]);
        save_front(path, &shfade[1][0], &shfade[1][1]);
        snprintf(path, sizeof (path), "%s/shadercap0000.bmp", argv[2]);
        remove(path);
        snprintf(path, sizeof (path), "%s/shadercapxxxx.bmp", argv[2]);
        i = screencapture(path, 0);
        setpalettefade(0, 0, 0, 0);
        printf("draw path shader: mean red/green unfaded %.1f/%.1f, faded %.1f/%.1f; screenshot %s\n",
               shfade[0][0], shfade[0][1], shfade[1][0], shfade[1][1], i ? "failed" : "written");
        if ((shfade[1][0] < shfade[0][0] + 64) || (shfade[1][1] > shfade[0][1] + 1))
            { fprintf(stderr, "FAIL: draw path shader: the fade isn't red\n"); return 1; }
        if (i != 0) { fprintf(stderr, "FAIL: draw path shader: screencapture\n"); return 1; }
        if ((err = glGetError()) != GL_NO_ERROR)
            { fprintf(stderr, "FAIL: glGetError 0x%x after the shader path's fade\n", (unsigned) err); return 1; }
        small[0] = malloc((size_t) xdim * ydim * 3);
        small[1] = malloc((size_t) xdim * ydim * 3);
        small_shot(small[0], posx, posy, posz, ang, sectnum);

        gldrawpathrequest = GLPATH_VBO;
        if (setgamemode_gl(1) != 0) { fprintf(stderr, "FAIL: OpenGL with draw path vbo\n"); return 1; }
        setbrightness(0, palette);
        ref = malloc((size_t) xdim * ydim * 3);
        buf = malloc((size_t) xdim * ydim * 3);
        view_shot(ref, posx, posy, posz, ang, sectnum);
        /* A view smaller than the screen: the border stays, and the view is
         * where vbo draws it, the right way up (near vbo's, as at full size). */
        small_shot(small[1], posx, posy, posz, ang, sectnum);
        smalld = small_diff(small[0], small[1]);
        printf("draw path shader, small view: border red %d (colour 31's %d); mean |shader - vbo| inside %.2f\n",
               small[0][(2 * xdim + 2) * 3], curpalette[31].r, smalld);
        if ((memcmp(&small[0][(2 * xdim + 2) * 3], &small[1][(2 * xdim + 2) * 3], 3) != 0) ||
            (small[0][(2 * xdim + 2) * 3] != curpalette[31].r) || (smalld > 8))
            { fprintf(stderr, "FAIL: draw path shader: a view smaller than the screen\n"); return 1; }
        free(small[0]);
        free(small[1]);
        for (i = 2; i <= 3; i++)
        {
            gl_forcefail = i;
            gldrawpathrequest = GLPATH_SHADER;
            if (setgamemode_gl(1) != 0) { fprintf(stderr, "FAIL: OpenGL with draw path shader\n"); return 1; }
            setbrightness(0, palette);
            s = gldrawpath;
            view_shot(buf, posx, posy, posz, ang, sectnum);
            err = glGetError();
            printf("draw path shader with %s: %s, then %s; %s vbo's frame\n",
                   (i == 2) ? "a shader that doesn't compile" : "an incomplete framebuffer",
                   gl_pathname(s), gl_pathname(gldrawpath),
                   memcmp(buf, ref, (size_t) xdim * ydim * 3) ? "not" : "as");
            if ((s != GLPATH_SHADER) || (gldrawpath != GLPATH_VBO) || (glsubmode != GLSUB_VBO) || (err != GL_NO_ERROR) ||
                (memcmp(buf, ref, (size_t) xdim * ydim * 3) != 0))
                { fprintf(stderr, "FAIL: draw path shader doesn't fall back to vbo (gl_forcefail %d)\n", i); return 1; }
        }
        gl_forcefail = 0;
        gldrawpathrequest = GLPATH_AUTO;
        free(ref);
        free(buf);
    }

    i = shader_frames(argv[2], posx, posy, posz, ang, sectnum, zerotile);
    if (i == 0) return 1;
    i = color_frames(argv[2], posx, posy, posz, ang, sectnum);
    if (i == 0) return 1;
    i = internal_frames(argv[2]);
    if (i == 0) return 1;

    /* A mode over the 8-bit 1600x1200 limit (it changes xdim and ydim, so
     * after the checks above): the OpenGL window still gets the size asked
     * for. Its height fits the display's usable area (less room for the
     * title bar), so the window gets it exactly, where the clamp's 1200 is
     * at least as tall as the display lets a window be. Its width may be held
     * to a narrower display. */
    askh = 900;
    usable.w = usable.h = 0;
    if (SDL_GetDisplayUsableBounds(0, &usable) == 0)
        askh = min(askh, usable.h - 96);
    if ((_setgamemode(1, 1680, askh) != 0) || (setgamemode_gl(1) != 0))
        { fprintf(stderr, "FAIL: the 1680x%d mode\n", askh); return 1; }
    /* OpenGL again from OpenGL: a new window replaces the old one. */
    if (setgamemode_gl(1) != 0)
        { fprintf(stderr, "FAIL: OpenGL again from OpenGL\n"); return 1; }
    view_shot(NULL, posx, posy, posz, ang, sectnum);
    if ((err = glGetError()) != GL_NO_ERROR)
        { fprintf(stderr, "FAIL: glGetError 0x%x after OpenGL again\n", (unsigned) err); return 1; }
    SDL_GetWindowSize(SDL_GL_GetCurrentWindow(), &winw, &winh);
    printf("1680x%d mode: window %dx%d (display usable %dx%d)\n", askh, winw, winh, usable.w, usable.h);
    uninitengine();
    /* A window manager may move a size by a few pixels; the unfixed window
     * is at least 96 taller (1200, or the display's height). */
    if ((abs(winh - askh) > 16) || ((abs(winw - 1680) > 16) && (usable.w >= 1680 + 64)))
        { fprintf(stderr, "FAIL: the window isn't the size asked for\n"); return 1; }
    printf("PASS: render_gl\n");
    return 0;
}
