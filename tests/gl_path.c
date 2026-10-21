/* Renderer 3's draw path: gl_choosepath and the setting's names. */

#include <stdio.h>

#include "glext.h"

static int fails;

static void check(const char *what, int got, int want)
{
    if (got != want) {
        printf("FAIL %s: %d (expected %d)\n", what, got, want);
        fails++;
    }
}

static void checksize(const char *what, int32_t w, int32_t h, int32_t wantw, int32_t wanth, int want)
{
    int32_t ow = -1, oh = -1;
    int got = gl_internalsize(w, h, &ow, &oh);

    if ((got != want) || (ow != wantw) || (oh != wanth)) {
        printf("FAIL %s: %dx%d, %d (expected %dx%d, %d)\n", what, (int) ow, (int) oh, got,
               (int) wantw, (int) wanth, want);
        fails++;
    }
}

static void checkrect(int32_t w, int32_t h, int32_t dw, int32_t dh, int32_t x, int32_t y, int32_t rw, int32_t rh)
{
    int32_t r[4];

    gl_fitrect(w, h, dw, dh, r);
    if ((r[0] != x) || (r[1] != y) || (r[2] != rw) || (r[3] != rh)) {
        printf("FAIL %dx%d in %dx%d: %d,%d %dx%d (expected %d,%d %dx%d)\n", (int) w, (int) h, (int) dw, (int) dh,
               (int) r[0], (int) r[1], (int) r[2], (int) r[3], (int) x, (int) y, (int) rw, (int) rh);
        fails++;
    }
}

int main(void)
{
    int r;

    /* a 1.1 driver: immediate whatever is asked */
    for (r = GLPATH_AUTO; r < GLPATH_COUNT; r++)
        check("1.1", gl_choosepath(r, 1, 1, 0, 0, 0), GLPATH_IMMEDIATE);

    /* 1.5 without GLSL: vertex buffers at best */
    check("1.5 auto", gl_choosepath(GLPATH_AUTO, 1, 5, 1, 0, 0), GLPATH_VBO);
    check("1.5 shader", gl_choosepath(GLPATH_SHADER, 1, 5, 1, 0, 0), GLPATH_VBO);
    check("1.5 vbo", gl_choosepath(GLPATH_VBO, 1, 5, 1, 0, 0), GLPATH_VBO);
    check("1.5 immediate", gl_choosepath(GLPATH_IMMEDIATE, 1, 5, 1, 0, 0), GLPATH_IMMEDIATE);

    /* 2.1 with everything */
    check("2.1 auto", gl_choosepath(GLPATH_AUTO, 2, 1, 1, 1, 1), GLPATH_SHADER);
    check("2.1 shader", gl_choosepath(GLPATH_SHADER, 2, 1, 1, 1, 1), GLPATH_SHADER);
    check("2.1 vbo", gl_choosepath(GLPATH_VBO, 2, 1, 1, 1, 1), GLPATH_VBO);
    check("2.1 immediate", gl_choosepath(GLPATH_IMMEDIATE, 2, 1, 1, 1, 1), GLPATH_IMMEDIATE);
    check("4.6 auto", gl_choosepath(GLPATH_AUTO, 4, 6, 1, 1, 1), GLPATH_SHADER);

    /* the shader path needs all of GL 2.1, GLSL and FBOs */
    check("2.0 auto", gl_choosepath(GLPATH_AUTO, 2, 0, 1, 1, 1), GLPATH_VBO);
    check("2.1 no FBO", gl_choosepath(GLPATH_AUTO, 2, 1, 1, 1, 0), GLPATH_VBO);
    check("2.1 no GLSL", gl_choosepath(GLPATH_AUTO, 2, 1, 1, 0, 1), GLPATH_VBO);
    check("bad request", gl_choosepath(42, 2, 1, 1, 1, 1), GLPATH_SHADER);

    /* names */
    check("shader", gl_pathfromname("shader"), GLPATH_SHADER);
    check("Shader", gl_pathfromname("Shader"), GLPATH_SHADER);
    check("VBO", gl_pathfromname("VBO"), GLPATH_VBO);
    check("immediate", gl_pathfromname("immediate"), GLPATH_IMMEDIATE);
    check("auto", gl_pathfromname("auto"), GLPATH_AUTO);
    check("bogus", gl_pathfromname("bogus"), GLPATH_AUTO);
    check("shaders", gl_pathfromname("shaders"), GLPATH_AUTO);
    check("empty", gl_pathfromname(""), GLPATH_AUTO);
    for (r = GLPATH_AUTO; r < GLPATH_COUNT; r++)
        check(gl_pathname(r), gl_pathfromname(gl_pathname(r)), r);
    check("name 9", gl_pathname(9) == gl_pathname(GLPATH_AUTO), 1);

    /* the internal size: both or neither, at least 320x200 */
    checksize("0x0", 0, 0, 0, 0, 0);
    checksize("320x200", 320, 200, 320, 200, 0);
    checksize("640x400", 640, 400, 640, 400, 0);
    checksize("320x0", 320, 0, 0, 0, 1);
    checksize("0x200", 0, 200, 0, 0, 1);
    checksize("5x5", 5, 5, 320, 200, 2);
    checksize("100x400", 100, 400, 320, 400, 2);
    checksize("800x10", 800, 10, 800, 200, 2);

    /* where it shows: the largest scale that fits, square pixels, centred */
    checkrect(320, 200, 2560, 1600, 0, 0, 2560, 1600);
    checkrect(640, 400, 2560, 1600, 0, 0, 2560, 1600);
    checkrect(320, 200, 1920, 1080, 96, 0, 1728, 1080);
    checkrect(640, 480, 2560, 1600, 213, 0, 2133, 1600);
    checkrect(320, 200, 1000, 1000, 0, 187, 1000, 625);
    checkrect(320, 240, 1280, 800, 106, 0, 1067, 800);
    checkrect(1280, 800, 1280, 800, 0, 0, 1280, 800);
    checkrect(1920, 1200, 1280, 800, 0, 0, 1280, 800);

    if (fails)
        return 1;
    printf("gl_path: ok\n");
    return 0;
}
