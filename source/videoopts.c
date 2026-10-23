/* OPTIONS -> VIDEO helpers; see videoopts.h. */

#include "videoopts.h"

#include <stdlib.h>

#define DISPLAY_WINDOWED_ 0
#define DISPLAY_EXCLUSIVE_ 2
#define MAXDRAW_W 1600
#define MAXDRAW_H 1200

static const char *const ss_names[SS_COUNT] = { "off", "2k", "4k", "8k" };
static const char *const filter_names[TEXFILTER_COUNT] = { "nearest", "bilinear", "trilinear" };

static int fromname(const char *const *names, int count, const char *s)
{
    int i, j;
    for (i = 0; i < count; i++) {
        for (j = 0; names[i][j] && (s[j] | 0x20) == names[i][j]; j++)
            ;
        if (!names[i][j] && !s[j])
            return i;
    }
    return -1;
}

const char *videoopts_ssname(int level)
{
    return ss_names[(level > 0 && level < SS_COUNT) ? level : SS_OFF];
}

int videoopts_ssfromname(const char *s) { return fromname(ss_names, SS_COUNT, s); }

const char *videoopts_filtername(int filter)
{
    return filter_names[(filter > 0 && filter < TEXFILTER_COUNT) ? filter : TEXFILTER_NEAREST];
}

int videoopts_filterfromname(const char *s) { return fromname(filter_names, TEXFILTER_COUNT, s); }

static int cmpmode(const void *a, const void *b)
{
    const vmode_t *x = a, *y = b;
    return x->w != y->w ? x->w - y->w : x->h - y->h;
}

int videoopts_buildreslist(const vmode_t *modes, int nmodes, int desktopw, int desktoph,
                           int displaymode, vmode_t *out, int maxout)
{
    static const vmode_t classic[] = {
        { 320, 200 }, { 640, 400 }, { 640, 480 }, { 800, 600 },
        { 1024, 768 }, { 1280, 1024 }, { 1600, 1200 }
    };
    int n = 0, j, k;

    if (displaymode != DISPLAY_WINDOWED_ && displaymode != DISPLAY_EXCLUSIVE_)
        return 0;
    for (k = 0; k < nmodes + (int)(sizeof(classic) / sizeof(classic[0])); k++) {
        vmode_t m;
        if (k < nmodes) {
            m = modes[k];
        } else if (displaymode == DISPLAY_WINDOWED_) {
            m = classic[k - nmodes];
        } else
            break;
        if (m.w <= 0 || m.h <= 0)
            continue;
        if (displaymode == DISPLAY_WINDOWED_ && (m.w > desktopw || m.h > desktoph))
            continue;
        for (j = 0; j < n && !(out[j].w == m.w && out[j].h == m.h); j++)
            ;
        if (j < n || n >= maxout)
            continue;
        out[n++] = m;
    }
    qsort(out, n, sizeof(out[0]), cmpmode);
    return n;
}

void videoopts_drawsize(int reqw, int reqh, int renderer, int *outw, int *outh)
{
    long w = reqw, h = reqh;

    if (renderer < 3 && (w > MAXDRAW_W || h > MAXDRAW_H)) {
        /* the smaller scale of the two limits keeps the aspect */
        if (w * MAXDRAW_H > h * MAXDRAW_W) {
            h = (h * MAXDRAW_W + w / 2) / w;
            w = MAXDRAW_W;
        } else {
            w = (w * MAXDRAW_H + h / 2) / h;
            h = MAXDRAW_H;
        }
        if (w < 1) w = 1;
        if (h < 1) h = 1;
    }
    *outw = (int)w;
    *outh = (int)h;
}

int videoopts_ssheight(int level)
{
    static const int heights[SS_COUNT] = { 0, 1440, 2160, 4320 };
    return (level > 0 && level < SS_COUNT) ? heights[level] : 0;
}

void videoopts_sssize(int level, int winw, int winh, int maxfb, int *outw, int *outh)
{
    long w = winw, h = videoopts_ssheight(level);

    if (!h || winh <= 0) {
        *outw = winw;
        *outh = winh;
        return;
    }
    w = (w * h + winh / 2) / winh;
    if (maxfb > 0 && (w > maxfb || h > maxfb)) {
        if (w >= h) {
            h = (h * maxfb + w / 2) / w;
            w = maxfb;
        } else {
            w = (w * maxfb + h / 2) / h;
            h = maxfb;
        }
    }
    *outw = (int)w;
    *outh = (int)h;
}

int videoopts_ssavailable(int level, int winw, int winh, int maxfb, int shaderpath, int internalset)
{
    int w, h;

    if (level <= 0)
        return 1;
    if (level >= SS_COUNT || !shaderpath || internalset)
        return 0;
    videoopts_sssize(level, winw, winh, maxfb, &w, &h);
    return h > winh;
}

void videoopts_begin(videoopts_t *v, const videovals_t *saved)
{
    v->saved = *saved;
    v->changed = 0;
}

void videoopts_touch(videoopts_t *v, int row)
{
    if (row >= 0 && row < VROW_COUNT)
        v->changed |= 1u << row;
}

int videoopts_changed(const videoopts_t *v, int row)
{
    return row >= 0 && row < VROW_COUNT && (v->changed >> row & 1);
}

void videoopts_saveinto(const videoopts_t *v, const videovals_t *inuse, videovals_t *out)
{
    *out = v->saved;
    if (videoopts_changed(v, VROW_RENDERER)) out->renderer = inuse->renderer;
    if (videoopts_changed(v, VROW_DISPLAYMODE)) out->displaymode = inuse->displaymode;
    if (videoopts_changed(v, VROW_RESOLUTION)) { out->width = inuse->width; out->height = inuse->height; }
    if (videoopts_changed(v, VROW_VSYNC)) out->vsync = inuse->vsync;
    if (videoopts_changed(v, VROW_BRIGHTNESS)) out->brightness = inuse->brightness;
    if (videoopts_changed(v, VROW_CONTRAST)) out->contrast = inuse->contrast;
    if (videoopts_changed(v, VROW_SATURATION)) out->saturation = inuse->saturation;
    if (videoopts_changed(v, VROW_SUPERSAMPLING)) out->supersampling = inuse->supersampling;
    if (videoopts_changed(v, VROW_TEXFILTER)) out->texfilter = inuse->texfilter;
}
