#include "profile_test_support.h"
/* Hor+: widening a view must preserve its central 4:3 picture, even pitched.
 * Uses real E1L1 data and the same camera at a fixed framebuffer height. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "build.h"
#include "display.h"
#include "engine.h"
#include "cache1d.h"
#include "SDL.h"
#ifdef USE_OPENGL
#include "SDL_opengl.h"
#include "glext.h"
#endif

extern SDL_Surface *surface;
void timerhandler(void) { }
void keyhandler(void) { }
void faketimerhandler(void) { }

static int32_t posx, posy, posz;
static short ang, sect;
#define REFW 480
#define REFH 360

static int mode(int width, int renderer)
{
    if (_setgamemode(1, width, REFH)) return 1;
#ifdef USE_OPENGL
    if (renderer == 3 && setgamemode_gl(1)) return 77;
#endif
    setrendermode(renderer);
    return 0;
}

/* A central crop, in palette indices for software or RGB for OpenGL. */
static void picture(unsigned char *dst, int renderer, int width, int height)
{
    int y;
#ifdef USE_OPENGL
    if (renderer == 3)
    {
        glReadBuffer(GL_FRONT);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels((xdim-width)/2, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, dst);
        glReadBuffer(GL_BACK);
        return;
    }
#else
    (void)renderer;
#endif
    for (y = 0; y < height; y++)
        memcpy(dst + y*width, (unsigned char *)surface->pixels + y*surface->pitch + (xdim-width)/2, width);
}

static void draw(int horiz, int fractional)
{
    clearallviews(0);
    setviewanglefraction(0, fractional ? 32768 : 0);
    drawrooms(posx, posy, posz, ang, horiz, sect);
    drawmasks();
    nextpage();
}

/* A split-colour HUD tile must stretch from edge to edge, keeping its
 * midpoint at the screen centre, on each software/OpenGL draw path. */
static int hud_width(int renderer)
{
    unsigned char *art;
    unsigned char got[3], want;
    int x, y, s;
    art = (unsigned char *)allocatepermanenttile(MAXTILES-1,320,16);
    if (!art) return 1;
    for (x = 0; x < 320; x++)
        for (y = 0; y < 16; y++) art[x*16+y] = x < 160 ? 47 : 79;
    invalidatetile(MAXTILES-1,-1,-1);
    clearallviews(0);
    rotatesprite(0,184<<16,65536,0,MAXTILES-1,0,0,2+8+16+64+512,0,0,xdim-1,ydim-1);
    nextpage();
    for (s = 0; s < 4; s++)
    {
        x = s == 0 ? 1 : s == 3 ? xdim-2 : xdim*s/3;
        y = ydim*190/200;
        want = (unsigned char)palookup[0][s < 2 ? 47 : 79];
#ifdef USE_OPENGL
        if (renderer == 3)
        {
            glReadBuffer(GL_FRONT);
            glReadPixels(x,ydim-1-y,1,1,GL_RGB,GL_UNSIGNED_BYTE,got);
            glReadBuffer(GL_BACK);
            /* Fixed-function vertex colours can round down by one. */
            if (abs(got[0]-curpalette[want].r) <= 1 && abs(got[1]-curpalette[want].g) <= 1 &&
                abs(got[2]-curpalette[want].b) <= 1)
                continue;
            fprintf(stderr,"RGB %u,%u,%u expected %u,%u,%u\n",got[0],got[1],got[2],
                    curpalette[want].r,curpalette[want].g,curpalette[want].b);
        }
        else
#else
        (void)renderer;
        (void)got;
#endif
        if (((unsigned char *)surface->pixels)[y*surface->pitch+x] == want)
            continue;
        fprintf(stderr,"FAIL: full-width HUD tile at column %d\n",x);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    unsigned char *ref, *pic;
    /* 16:9, 21:9 and 32:9 at a common height. */
    static const int widths[] = { 640, 840, 1280 };
    static const int horizons[] = { 100, 70, 130 };
    int r, h, f, w, x, channels, refw, refh, samples, bad, status, fail = 0;

    if (argc < 3) return 2;
    if (initgroupfile(argv[1]) == -1) return 1;
    initengine();
    if (!palookup[0] || loadpics("tiles000.art") != 0 ||
        loadboard("E1L1.MAP", &posx, &posy, &posz, &ang, &sect) == -1)
        return 1;
    /* Hide map markers and the player start. */
    for (x = 0; x < MAXSPRITES; x++)
        if (sprite[x].statnum < MAXSTATUS &&
            ((sprite[x].picnum >= 1 && sprite[x].picnum <= 10) || sprite[x].picnum == 1405))
            sprite[x].cstat |= 32768;
    _platform_init(argc, argv, "render_horplus", "render");
    r = atoi(argv[2]);
#ifdef USE_OPENGL
    if (argc > 3) gldrawpathrequest = gl_pathfromname(argv[3]);
#endif
    channels = r == 3 ? 3 : 1;
    for (h = 0; h < 3; h++)
        for (f = 0; f < 2; f++)
        {
            if ((status = mode(REFW, r)) != 0) return status;
            refw = xdim; refh = ydim;
            samples = refw*refh*channels;
            ref = malloc(samples); pic = malloc(samples);
            if (!ref || !pic) return 1;
            draw(horizons[h], f);
            picture(ref, r, refw, refh);
            for (w = 0; w < 3; w++)
            {
                if ((status = mode(widths[w], r)) != 0) return status;
                if (ydim != refh || xdim < refw) return 1;
                draw(horizons[h], f);
                picture(pic, r, refw, refh);
                bad = 0;
                for (x = 0; x < samples; x++)
                    bad += ref[x] != pic[x];
                printf("renderer %d, %dx%d, horizon %d%s: %.2f%% central pixels differ\n",
                       r, (int)xdim, (int)ydim, horizons[h], f ? ".5" : "", bad*100.0/samples);
                /* Fixed-point rounding and edge coverage may differ slightly. */
                if (bad > samples/20) fail = 1;
            }
            free(ref); free(pic);
        }
    for (w = 0; w < 3; w++)
    {
        if ((status = mode(widths[w],r)) != 0) return status;
        fail |= hud_width(r);
    }
    uninitengine();
    if (fail) fprintf(stderr, "FAIL: widening the view changed the central 4:3 picture\n");
    return fail;
}
