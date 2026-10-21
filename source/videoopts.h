/*
 * Helpers for OPTIONS -> VIDEO: the resolution list, the draw size, the
 * supersampling choices and the tracking of which rows the player changed.
 * Engine- and SDL-free, so the tests link it alone.
 */
#ifndef VIDEOOPTS_H
#define VIDEOOPTS_H

typedef struct { int w, h; } vmode_t;

/* Supersampling is stored as a level, 0..3; the config files hold its name. */
enum { SS_OFF, SS_2K, SS_4K, SS_8K, SS_COUNT };

/* Texture filtering (renderer 3), by name in the config files. */
enum { TEXFILTER_NEAREST, TEXFILTER_BILINEAR, TEXFILTER_TRILINEAR, TEXFILTER_COUNT };

/* "off", "2k", "4k", "8k" */
const char *videoopts_ssname(int level);
/* A level from its name in any case; -1 for anything else. */
int videoopts_ssfromname(const char *s);
/* "nearest", "bilinear", "trilinear" */
const char *videoopts_filtername(int filter);
/* A filter from its name in any case; -1 for anything else. */
int videoopts_filterfromname(const char *s);

/*
 * The sizes the page lists for a display mode (DISPLAY_*, 0 windowed,
 * 1 borderless, 2 exclusive), sorted by width then height, without
 * duplicates; returns how many went into out (at most maxout).
 * Windowed: the SDL modes that fit the desktop plus the classic sizes that
 * fit. Exclusive: the SDL modes. Borderless: none (the row is greyed).
 */
int videoopts_buildreslist(const vmode_t *modes, int nmodes, int desktopw, int desktoph,
                           int displaymode, vmode_t *out, int maxout);

/* The size the 8-bit renderers (0-2) draw at for a requested one: capped to
 * 1600x1200, keeping the aspect. Renderer 3 draws at the size asked. */
void videoopts_drawsize(int reqw, int reqh, int renderer, int *outw, int *outh);

/* Supersampling's frame buffer height: 0, 1440, 2160 or 4320. */
int videoopts_ssheight(int level);

/* The frame buffer size for level at the window's aspect, clamped to maxfb
 * (the GPU's largest, 0: no limit). Level 0: the window's size. */
void videoopts_sssize(int level, int winw, int winh, int maxfb, int *outw, int *outh);

/* Whether the page offers a level: off always; the others need the shader
 * path and no internal size set, and must end up larger than the window. */
int videoopts_ssavailable(int level, int winw, int winh, int maxfb, int shaderpath, int internalset);

/*
 * Save tracking. The page shows the values in effect, which can differ from
 * the config's (-renderer, Alt+Enter, BUILD_WINDOWED, a fallback from
 * exclusive), and the config must only take the rows the player changed.
 * begin() keeps the config's values; touch() marks a row the player changed;
 * saveinto() gives the values to write: the config's, but for the touched
 * rows, which take the values in effect.
 */
enum { VROW_RENDERER, VROW_DISPLAYMODE, VROW_RESOLUTION, VROW_VSYNC, VROW_BRIGHTNESS,
       VROW_CONTRAST, VROW_SATURATION, VROW_SUPERSAMPLING, VROW_TEXFILTER, VROW_COUNT };

typedef struct {
    int renderer, displaymode, width, height, vsync, brightness;
    double contrast, saturation;
    int supersampling, texfilter;
} videovals_t;

typedef struct {
    videovals_t saved;
    unsigned changed;  /* bit per VROW_* */
} videoopts_t;

void videoopts_begin(videoopts_t *v, const videovals_t *saved);
void videoopts_touch(videoopts_t *v, int row);
int videoopts_changed(const videoopts_t *v, int row);
void videoopts_saveinto(const videoopts_t *v, const videovals_t *inuse, videovals_t *out);

#endif
