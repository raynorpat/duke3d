/*
 * Renders E1L1's start view in renderer 3 (OpenGL) in a real window, with
 *  a HUD tile, printext256 text, a drawline256 line and a plotpixel, and
 *  writes, for checking by eye:
 *   <dir>/gl0000.pcx   screencapture of that frame (24-bit PCX)
 *   <dir>/fade.bmp     the screen after setpalettefade(63,0,0,32) over a
 *                      frame that isn't redrawn, as a title screen's
 *   <dir>/unfade.bmp   the screen after setpalettefade(0,0,0,0)
 *  It fails on a GL error, a failed capture, or a fade that doesn't redden.
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
#define FIRSTGUN 2524  /* names.h: the pistol */

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

int main(int argc, char **argv)
{
    int32_t posx, posy, posz;
    short ang, sectnum;
    char path[1024];
    double r0, g0, r1, g1;
    GLenum err;
    int32_t i;

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
    snprintf(path, sizeof (path), "%s/gl0000.pcx", argv[2]);
    if (screencapture(path, 0) != 0) { fprintf(stderr, "FAIL: screencapture\n"); return 1; }

    setpalettefade(63, 0, 0, 32);
    snprintf(path, sizeof (path), "%s/fade.bmp", argv[2]);
    save_front(path, &r1, &g1);
    setpalettefade(0, 0, 0, 0);
    snprintf(path, sizeof (path), "%s/unfade.bmp", argv[2]);
    save_front(path, &r0, &g0);
    printf("mean red/green: faded %.1f/%.1f, unfaded %.1f/%.1f\n", r1, g1, r0, g0);
    err = glGetError();

    uninitengine();

    if (err != GL_NO_ERROR) { fprintf(stderr, "FAIL: glGetError 0x%x\n", (unsigned) err); return 1; }
    if ((r1 < r0 + 64) || (g1 > g0 + 1)) { fprintf(stderr, "FAIL: the fade isn't red\n"); return 1; }
    printf("PASS: render_gl\n");
    return 0;
}
