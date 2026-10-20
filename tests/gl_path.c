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

    if (fails)
        return 1;
    printf("gl_path: ok\n");
    return 0;
}
