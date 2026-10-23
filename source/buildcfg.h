#ifndef BUILDCFG_H
#define BUILDCFG_H

#include <stddef.h>
#include <stdint.h>

#define BUILDCFG_NUMKEYS 19

typedef struct {
    int32_t renderer, xdim3d, ydim3d, displaymode, vsync, gltexmiplevel;  /* DISPLAY_* */
    double mousesensitivity;
    unsigned char keys[BUILDCFG_NUMKEYS];
    int32_t gldrawpath, glinternalwidth, glinternalheight;  /* GLPATH_*; 0: the window's */
    double glcontrast, glsaturation;  /* the shader path's; 1: as it is */
    int32_t supersampling, gltexturefilter;  /* SS_*, TEXFILTER_*: "off", "2k".. and "nearest".. in the file */
} buildcfg_t;

/* Fills c with the defaults; keys are build.c's buildkeys[] initializer. */
void buildcfg_defaults(buildcfg_t *c);

/* Reads path over the defaults and clamps. 0 read, -1 missing (c untouched). */
int buildcfg_read(buildcfg_t *c, const char *path);

/* Writes through a temp file and a rename. 0 ok, -1 error. */
int buildcfg_write(const buildcfg_t *c, const char *path);

/* Takes only the keys from a setup.dat. 0 ok, -1 missing. */
int buildcfg_import_setupdat(buildcfg_t *c, const char *path);

/* <config folder>/build.cfg, creating the folder. 0 ok, -1 error. */
int buildcfg_path(char *out, size_t outlen);

#endif
