/* The VIDEO page's helpers: resolution list, draw size, supersampling and
 * save tracking. Engine-free. */

#include <stdio.h>
#include <string.h>

#include "videoopts.h"

static int fails;

#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); fails++; } } while (0)

static int has(const vmode_t *l, int n, int w, int h)
{
    int i;
    for (i = 0; i < n; i++)
        if (l[i].w == w && l[i].h == h)
            return 1;
    return 0;
}

int main(void)
{
    /* duplicates (refresh rates, HiDPI pairs), unsorted, one too big */
    const vmode_t sdl[] = { { 1920, 1080 }, { 1280, 720 }, { 1920, 1080 }, { 3840, 2160 },
                            { 1280, 720 }, { 800, 600 } };
    vmode_t out[32];
    int n, i, w, h;
    videoopts_t v;
    videovals_t saved, inuse, dest;

    /* windowed: fits the desktop, plus the classic sizes that fit */
    n = videoopts_buildreslist(sdl, 6, 1920, 1080, 0, out, 32);
    CHECK(has(out, n, 1920, 1080) && has(out, n, 1280, 720) && has(out, n, 800, 600));
    CHECK(!has(out, n, 3840, 2160));
    CHECK(has(out, n, 320, 200) && has(out, n, 640, 400) && has(out, n, 640, 480));
    CHECK(has(out, n, 1024, 768));
    CHECK(has(out, n, 1280, 1024) && !has(out, n, 1600, 1200));  /* 1600x1200 is taller than 1080 */
    for (i = 1; i < n; i++)
        CHECK(out[i - 1].w < out[i].w || (out[i - 1].w == out[i].w && out[i - 1].h < out[i].h));
    CHECK(n == 8);
    n = videoopts_buildreslist(sdl, 6, 2560, 1440, 0, out, 32);
    CHECK(has(out, n, 1600, 1200));
    CHECK(videoopts_buildreslist(sdl, 6, 1920, 1080, 0, out, 3) == 3);  /* maxout */

    /* exclusive: the SDL modes only, deduped, sorted */
    n = videoopts_buildreslist(sdl, 6, 1920, 1080, 2, out, 32);
    CHECK(n == 4 && out[0].w == 800 && out[3].w == 3840);
    CHECK(!has(out, n, 320, 200));

    /* borderless: greyed */
    CHECK(videoopts_buildreslist(sdl, 6, 1920, 1080, 1, out, 32) == 0);

    /* draw size */
    videoopts_drawsize(640, 480, 0, &w, &h);
    CHECK(w == 640 && h == 480);
    videoopts_drawsize(1920, 1080, 1, &w, &h);
    CHECK(w == 1600 && h == 900);
    videoopts_drawsize(3840, 2160, 2, &w, &h);
    CHECK(w == 1600 && h == 900);
    videoopts_drawsize(1600, 2400, 0, &w, &h);
    CHECK(w == 800 && h == 1200);
    videoopts_drawsize(1920, 1080, 3, &w, &h);
    CHECK(w == 1920 && h == 1080);
    videoopts_drawsize(3840, 2160, 3, &w, &h);
    CHECK(w == 3840 && h == 2160);

    /* supersampling */
    CHECK(videoopts_ssheight(0) == 0 && videoopts_ssheight(1) == 1440);
    CHECK(videoopts_ssheight(2) == 2160 && videoopts_ssheight(3) == 4320);
    CHECK(videoopts_ssheight(9) == 0 && videoopts_ssheight(-1) == 0);
    videoopts_sssize(1, 1920, 1080, 16384, &w, &h);
    CHECK(w == 2560 && h == 1440);
    videoopts_sssize(3, 1920, 1080, 16384, &w, &h);
    CHECK(w == 7680 && h == 4320);
    videoopts_sssize(3, 1920, 1080, 4096, &w, &h);  /* clamped, aspect kept */
    CHECK(w == 4096 && h == 2304);
    videoopts_sssize(0, 1920, 1080, 4096, &w, &h);
    CHECK(w == 1920 && h == 1080);
    CHECK(videoopts_ssavailable(0, 1920, 1080, 16384, 0, 1));  /* off: always */
    CHECK(videoopts_ssavailable(1, 1920, 1080, 16384, 1, 0));
    CHECK(!videoopts_ssavailable(1, 1920, 1080, 16384, 0, 0));  /* no shader path */
    CHECK(!videoopts_ssavailable(1, 1920, 1080, 16384, 1, 1));  /* internal size wins */
    CHECK(!videoopts_ssavailable(1, 2560, 1440, 16384, 1, 0));  /* not larger than the window */
    CHECK(videoopts_ssavailable(2, 2560, 1440, 16384, 1, 0));
    CHECK(!videoopts_ssavailable(3, 1920, 1080, 1080, 1, 0));  /* GPU can't beat the window */
    CHECK(videoopts_ssavailable(3, 1920, 1080, 4096, 1, 0));   /* clamped, still larger */
    CHECK(!videoopts_ssavailable(4, 1920, 1080, 16384, 1, 0));

    /* names */
    CHECK(!strcmp(videoopts_ssname(0), "off") && !strcmp(videoopts_ssname(3), "8k"));
    CHECK(videoopts_ssfromname("4K") == SS_4K && videoopts_ssfromname("off") == SS_OFF);
    CHECK(videoopts_ssfromname("16k") == -1 && videoopts_ssfromname("") == -1);
    CHECK(!strcmp(videoopts_filtername(2), "trilinear"));
    CHECK(videoopts_filterfromname("Bilinear") == TEXFILTER_BILINEAR);
    CHECK(videoopts_filterfromname("bogus") == -1);

    /* save tracking: only the touched rows take the in-effect values */
    memset(&saved, 0, sizeof(saved));
    saved.renderer = 0; saved.displaymode = 1; saved.width = 640; saved.height = 480;
    saved.vsync = 1; saved.brightness = 2; saved.contrast = 1.0; saved.saturation = 1.0;
    inuse = saved;
    inuse.renderer = 3;  /* -renderer 3: in effect, not saved */
    inuse.displaymode = 0;  /* Alt+Enter */
    inuse.width = 1920; inuse.height = 1080;
    inuse.vsync = 0;
    inuse.contrast = 1.3;
    videoopts_begin(&v, &saved);
    videoopts_saveinto(&v, &inuse, &dest);
    CHECK(!memcmp(&dest, &saved, sizeof(saved)));
    videoopts_touch(&v, VROW_CONTRAST);
    videoopts_touch(&v, VROW_RESOLUTION);
    CHECK(videoopts_changed(&v, VROW_CONTRAST) && !videoopts_changed(&v, VROW_RENDERER));
    videoopts_saveinto(&v, &inuse, &dest);
    CHECK(dest.contrast == 1.3 && dest.width == 1920 && dest.height == 1080);
    CHECK(dest.renderer == 0 && dest.displaymode == 1 && dest.vsync == 1);
    videoopts_touch(&v, 99);  /* ignored */
    CHECK(v.changed == ((1u << VROW_CONTRAST) | (1u << VROW_RESOLUTION)));
    videoopts_begin(&v, &saved);  /* a fresh page starts clean */
    CHECK(v.changed == 0);

    if (fails)
        return 1;
    printf("video_opts: ok\n");
    return 0;
}
