/* build.cfg reader and writer: defaults, clamping, bad lines, setup.dat. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <process.h>
#define mkdir(p, m) _mkdir(p)
#define rmdir _rmdir
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include "buildcfg.h"
#include "build/glext.h"

static int fails;

#define CHECK(cond) do { if (!(cond)) { \
    printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); fails++; } } while (0)

static void put(const char *path, const void *data, size_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f) { printf("FAIL cannot write %s\n", path); exit(1); }
    fwrite(data, 1, n, f);
    fclose(f);
}

static void putstr(const char *path, const char *s) { put(path, s, strlen(s)); }

static int same(const buildcfg_t *a, const buildcfg_t *b)
{
    return a->renderer == b->renderer && a->xdim3d == b->xdim3d &&
           a->ydim3d == b->ydim3d && a->fullscreen == b->fullscreen &&
           a->gltexmiplevel == b->gltexmiplevel &&
           a->gldrawpath == b->gldrawpath && a->glinternalwidth == b->glinternalwidth &&
           a->glinternalheight == b->glinternalheight &&
           a->mousesensitivity == b->mousesensitivity &&
           !memcmp(a->keys, b->keys, sizeof(a->keys));
}

int main(void)
{
    const char *tmp = getenv("TMPDIR");
    char dir[512], cfg[600], dat[600], text[4096], out[1024];
#ifndef _WIN32
    char home[600];
#endif
    buildcfg_t d, c;
    FILE *f;
    size_t n;

    snprintf(dir, sizeof(dir), "%s/build_cfg_test_%d", tmp && *tmp ? tmp : "/tmp", (int)getpid());
    mkdir(dir, 0700);
    snprintf(cfg, sizeof(cfg), "%s/build.cfg", dir);
    snprintf(dat, sizeof(dat), "%s/setup.dat", dir);

    buildcfg_defaults(&d);
    CHECK(d.keys[0] == 0xc8 && d.keys[18] == 0x0f);
    CHECK(d.keys[12] == 0x33 && d.keys[13] == 0x34);  /* strafe: , and . */

    /* round trip */
    CHECK(buildcfg_write(&d, cfg) == 0);
    memset(&c, 0, sizeof(c));
    CHECK(buildcfg_read(&c, cfg) == 0);
    CHECK(same(&c, &d));
    f = fopen(cfg, "r");
    n = f ? fread(text, 1, sizeof(text) - 1, f) : 0;
    if (f) fclose(f);
    text[n] = '\0';
    CHECK(strstr(text, "renderer = 0"));
    CHECK(strstr(text, "xdim3d = 640"));
    CHECK(strstr(text, "fullscreen = 1"));
    CHECK(strstr(text, "keyforward = c8"));
    CHECK(strstr(text, "gldrawpath = auto"));
    CHECK(strstr(text, "glinternalwidth = 0"));
    CHECK(d.gldrawpath == GLPATH_AUTO && d.glinternalwidth == 0 && d.glinternalheight == 0);

    /* draw path in any case, written back lowercase */
    putstr(cfg, "gldrawpath = Shader\nglinternalwidth = 320\nglinternalheight = 200\n");
    CHECK(buildcfg_read(&c, cfg) == 0);
    CHECK(c.gldrawpath == GLPATH_SHADER && c.glinternalwidth == 320 && c.glinternalheight == 200);
    CHECK(buildcfg_write(&c, cfg) == 0);
    f = fopen(cfg, "r");
    n = f ? fread(text, 1, sizeof(text) - 1, f) : 0;
    if (f) fclose(f);
    text[n] = '\0';
    CHECK(strstr(text, "gldrawpath = shader"));
    CHECK(strstr(text, "glinternalheight = 200"));

    /* clamping */
    putstr(cfg, "renderer = 9\nxdim3d = 99999\ngltexmiplevel = -3\n"
                "mousesensitivity = 50\nkeyfire = 0\ngldrawpath = bogus\n"
                "glinternalwidth = -5\nglinternalheight = 99999\n");
    CHECK(buildcfg_read(&c, cfg) == 0);
    CHECK(c.renderer == 3 && c.xdim3d == 7680 && c.gltexmiplevel == 0);
    CHECK(c.gldrawpath == GLPATH_AUTO && c.glinternalwidth == 0 && c.glinternalheight == 4320);
    CHECK(c.mousesensitivity == 10.0 && c.keys[6] == 0x1d);

    /* garbage */
    putstr(cfg, "renderer\n= 5\nxdim3d = abc\r\n; comment\r\nunknownkey = 1\r\n"
                "ydim3d = 600\r\nfullscreen = 0\r\n");
    CHECK(buildcfg_read(&c, cfg) == 0);
    CHECK(c.renderer == d.renderer && c.xdim3d == 640);
    CHECK(c.ydim3d == 600 && c.fullscreen == 0);

    /* missing file leaves the struct alone */
    c = d;
    c.renderer = 2;
    snprintf(out, sizeof(out), "%s/nope.cfg", dir);
    CHECK(buildcfg_read(&c, out) == -1 && c.renderer == 2);

    /* setup.dat */
    {
        unsigned char b[27] = {0};
        int i;
        for (i = 0; i < 19; i++) b[8 + i] = (unsigned char)(0x11 + i);
        put(dat, b, 27);
        c = d;
        CHECK(buildcfg_import_setupdat(&c, dat) == 0);
        for (i = 0; i < 19; i++) CHECK(c.keys[i] == 0x11 + i);
        put(dat, b, 10);
        c = d;
        CHECK(buildcfg_import_setupdat(&c, dat) == 0);
        CHECK(c.keys[0] == 0x11 && c.keys[1] == 0x12 && c.keys[2] == d.keys[2]);
        snprintf(out, sizeof(out), "%s/nope.dat", dir);
        CHECK(buildcfg_import_setupdat(&c, out) == -1);
    }

    /* path under a temporary HOME */
#ifndef _WIN32
    snprintf(home, sizeof(home), "%s/home", dir);
    mkdir(home, 0700);
    setenv("HOME", home, 1);
    CHECK(buildcfg_path(out, sizeof(out)) == 0);
    CHECK(strncmp(out, home, strlen(home)) == 0);
    CHECK(buildcfg_write(&d, out) == 0);
    CHECK(buildcfg_read(&c, out) == 0 && same(&c, &d));
    remove(out);
    *strrchr(out, '/') = '\0';
    rmdir(out);
    snprintf(out, sizeof(out), "%s/Library/Application Support", home);
    rmdir(out);
    snprintf(out, sizeof(out), "%s/Library", home);
    rmdir(out);
    rmdir(home);
#endif

    remove(cfg);
    remove(dat);
    rmdir(dir);
    if (fails) return 1;
    printf("build_cfg: ok\n");
    return 0;
}
