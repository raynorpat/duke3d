/* Reader and writer for the map editor's build.cfg. Engine-free. */

#include "buildcfg.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

static const char *const key_names[BUILDCFG_NUMKEYS] = {
    "keyforward", "keybackward", "keyturnleft", "keyturnright", "keyrun",
    "keystrafe", "keyfire", "keyuse", "keystandhigh", "keystandlow",
    "keylookup", "keylookdown", "keystrafeleft", "keystraferight",
    "key2dmode", "keyviewcycle", "key2dzoomin", "key2dzoomout", "keychat"
};

static const unsigned char default_keys[BUILDCFG_NUMKEYS] = {
    0xc8, 0xd0, 0xcb, 0xcd, 0x2a, 0x9d, 0x1d, 0x39, 0x1e, 0x2c,
    0xd1, 0xc9, 0x33, 0x34, 0x9c, 0x1c, 0x0d, 0x0c, 0x0f
};

void buildcfg_defaults(buildcfg_t *c)
{
    c->renderer = 0;
    c->xdim3d = 640;
    c->ydim3d = 480;
    c->fullscreen = 1;
    c->gltexmiplevel = 0;
    c->mousesensitivity = 1.0;
    memcpy(c->keys, default_keys, sizeof(c->keys));
}

static int32_t clamp_int(long v, long lo, long hi)
{
    return (int32_t)(v < lo ? lo : v > hi ? hi : v);
}

/* Parses all of s as a number in the given base; 0 on failure. */
static int parse_long(const char *s, int base, long *out)
{
    char *end;
    errno = 0;
    *out = strtol(s, &end, base);
    return end != s && *end == '\0' && errno == 0;
}

static void apply(buildcfg_t *c, const char *name, const char *val)
{
    long v;
    int i;

    if (!strcmp(name, "mousesensitivity")) {
        char *end;
        double d = strtod(val, &end);
        if (end == val || *end != '\0' || d != d)
            return;
        c->mousesensitivity = d < 0.1 ? 0.1 : d > 10.0 ? 10.0 : d;
        return;
    }
    for (i = 0; i < BUILDCFG_NUMKEYS; i++) {
        if (!strcmp(name, key_names[i])) {
            if (!parse_long(val, 16, &v))
                return;
            c->keys[i] = (v >= 0x01 && v <= 0xff) ? (unsigned char)v : default_keys[i];
            return;
        }
    }
    if (!parse_long(val, 10, &v))
        return;
    if (!strcmp(name, "renderer")) c->renderer = clamp_int(v, 0, 3);
    else if (!strcmp(name, "xdim3d")) c->xdim3d = clamp_int(v, 320, 7680);
    else if (!strcmp(name, "ydim3d")) c->ydim3d = clamp_int(v, 200, 4320);
    else if (!strcmp(name, "fullscreen")) c->fullscreen = v != 0;
    else if (!strcmp(name, "gltexmiplevel")) c->gltexmiplevel = clamp_int(v, 0, 15);
}

static char *trim(char *s)
{
    char *e;
    while (isspace((unsigned char)*s)) s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) *--e = '\0';
    return s;
}

int buildcfg_read(buildcfg_t *c, const char *path)
{
    char line[256];
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;
    buildcfg_defaults(c);
    while (fgets(line, sizeof(line), f)) {
        char *eq, *name, *val;
        name = trim(line);
        if (*name == ';' || *name == '#')
            continue;
        eq = strchr(name, '=');
        if (!eq)
            continue;
        *eq = '\0';
        name = trim(name);
        val = trim(eq + 1);
        if (*name && *val)
            apply(c, name, val);
    }
    fclose(f);
    return 0;
}

int buildcfg_write(const buildcfg_t *c, const char *path)
{
    char tmp[1024];
    FILE *f;
    int i, ok;

    if (snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int)sizeof(tmp))
        return -1;
    f = fopen(tmp, "w");
    if (!f)
        return -1;
    fprintf(f, "; Build editor configuration\n");
    fprintf(f, "renderer = %d\n", (int)c->renderer);
    fprintf(f, "xdim3d = %d\n", (int)c->xdim3d);
    fprintf(f, "ydim3d = %d\n", (int)c->ydim3d);
    fprintf(f, "fullscreen = %d\n", (int)c->fullscreen);
    fprintf(f, "gltexmiplevel = %d\n", (int)c->gltexmiplevel);
    fprintf(f, "mousesensitivity = %g\n", c->mousesensitivity);
    for (i = 0; i < BUILDCFG_NUMKEYS; i++)
        fprintf(f, "%s = %02x\n", key_names[i], c->keys[i]);
    ok = ferror(f) == 0;
    if (fclose(f) != 0)
        ok = 0;
    if (ok) {
#ifdef _WIN32
        remove(path); /* rename won't replace an existing file here */
#endif
        ok = rename(tmp, path) == 0;
    }
    if (!ok) {
        remove(tmp);
        return -1;
    }
    return 0;
}

int buildcfg_import_setupdat(buildcfg_t *c, const char *path)
{
    unsigned char buf[8 + BUILDCFG_NUMKEYS];
    size_t n, i;
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;
    n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    for (i = 8; i < n; i++) {
        unsigned char k = buf[i];
        c->keys[i - 8] = k ? k : default_keys[i - 8];
    }
    return 0;
}

int buildcfg_path(char *out, size_t outlen)
{
#ifdef _WIN32
    char dir[1024];

    /* absolute, so a later chdir (the editor's load menu) doesn't move it */
    if (!_getcwd(dir, sizeof(dir)))
        return -1;
    return snprintf(out, outlen, "%s\\build.cfg", dir) < (int)outlen ? 0 : -1;
#else
    const char *home = getenv("HOME");
    char dir[1024];
    int n;

    if (!home)
        return -1;
#ifdef __APPLE__
    snprintf(dir, sizeof(dir), "%s/Library/", home);
    mkdir(dir, S_IRWXU);
    snprintf(dir, sizeof(dir), "%s/Library/Application Support/", home);
    mkdir(dir, S_IRWXU);
    n = snprintf(dir, sizeof(dir), "%s/Library/Application Support/Duke Nukem 3D/", home);
#else
    n = snprintf(dir, sizeof(dir), "%s/.duke3d/", home);
#endif
    if (n < 0 || n >= (int)sizeof(dir))
        return -1;
    if (mkdir(dir, S_IRWXU) == -1 && errno != EEXIST) {
        fprintf(stderr, "Couldn't create preferences directory: %s\n", strerror(errno));
        return -1;
    }
    if (snprintf(out, outlen, "%sbuild.cfg", dir) >= (int)outlen)
        return -1;
    return 0;
#endif
}
