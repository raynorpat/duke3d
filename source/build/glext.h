/*
 * Renderer 3's draw paths and the OpenGL entry points past 1.1, loaded at
 *  run time so the binary still runs on a 1.1 driver. The path names and
 *  gl_choosepath need no OpenGL: the game's and editor's settings use them.
 */
#ifndef GLEXT_H
#define GLEXT_H

#include <stdint.h>

	/* GL 1.1 immediate mode, GL 1.5 vertex buffers, GL 2.1 GLSL and FBOs. */
enum { GLPATH_AUTO, GLPATH_IMMEDIATE, GLPATH_VBO, GLPATH_SHADER, GLPATH_COUNT };

	/* The setting's name of a path: "auto", "immediate", "vbo" or "shader". */
static inline const char *gl_pathname(int path)
{
	static const char *const names[GLPATH_COUNT] = { "auto", "immediate", "vbo", "shader" };
	return names[(path > 0 && path < GLPATH_COUNT) ? path : GLPATH_AUTO];
}

	/* A path from its name in any case; anything else is GLPATH_AUTO. */
static inline int gl_pathfromname(const char *s)
{
	int path, i;

	for(path=GLPATH_COUNT-1;path>GLPATH_AUTO;path--)
	{
		const char *n = gl_pathname(path);
		for(i=0;n[i] && (s[i]|0x20) == n[i];i++);
		if (!n[i] && !s[i]) return path;
	}
	return GLPATH_AUTO;
}

	/* The path to draw with: the best the driver has, no better than
	 * requested (GLPATH_AUTO: the best). Shader needs GL 2.1 with GLSL, FBOs
	 * and vertex buffers; vbo needs vertex buffers. */
int gl_choosepath(int requested, int glmajor, int glminor, int hasvbo, int hasglsl, int hasfbo);

	/* The path asked for (set before setgamemode_gl) and the one in use. */
extern int32_t gldrawpathrequest, gldrawpath;
	/* The size renderer 3 draws at (0: the window's). Not used yet. */
extern int32_t glinternalwidth, glinternalheight;
	/* Test hook: 1 makes GLSL and FBOs unavailable, as if they failed; 2 makes
	 * the shader path's shader not compile, 3 its framebuffer incomplete. */
extern int32_t gl_forcefail;

#ifdef USE_OPENGL
#include "SDL_opengl.h"

	/* Loads the entry points below with the GL context current (NULL where
	 * the driver lacks them) and sets glinfo's vbo, glsl and fbo, then sets
	 * gldrawpath from gldrawpathrequest and prints it. exts is GL_EXTENSIONS. */
void gl_initdrawpath(const char *exts);

	/* Whole-word match of name in a GL_EXTENSIONS string. */
int gl_has_extension(const char *exts, const char *name);

	/* GL 1.5 or GL_ARB_vertex_buffer_object */
extern PFNGLGENBUFFERSPROC bglGenBuffers;
extern PFNGLDELETEBUFFERSPROC bglDeleteBuffers;
extern PFNGLBINDBUFFERPROC bglBindBuffer;
extern PFNGLBUFFERDATAPROC bglBufferData;
extern PFNGLBUFFERSUBDATAPROC bglBufferSubData;

	/* GL 1.3 or GL_ARB_multitexture */
extern PFNGLACTIVETEXTUREPROC bglActiveTexture;

	/* GL 2.0 */
extern PFNGLCREATESHADERPROC bglCreateShader;
extern PFNGLSHADERSOURCEPROC bglShaderSource;
extern PFNGLCOMPILESHADERPROC bglCompileShader;
extern PFNGLGETSHADERIVPROC bglGetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC bglGetShaderInfoLog;
extern PFNGLDELETESHADERPROC bglDeleteShader;
extern PFNGLCREATEPROGRAMPROC bglCreateProgram;
extern PFNGLATTACHSHADERPROC bglAttachShader;
extern PFNGLBINDATTRIBLOCATIONPROC bglBindAttribLocation;
extern PFNGLLINKPROGRAMPROC bglLinkProgram;
extern PFNGLGETPROGRAMIVPROC bglGetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC bglGetProgramInfoLog;
extern PFNGLDELETEPROGRAMPROC bglDeleteProgram;
extern PFNGLUSEPROGRAMPROC bglUseProgram;
extern PFNGLGETUNIFORMLOCATIONPROC bglGetUniformLocation;
extern PFNGLUNIFORM1IPROC bglUniform1i;
extern PFNGLUNIFORM1FPROC bglUniform1f;
extern PFNGLUNIFORM2FPROC bglUniform2f;
extern PFNGLUNIFORM4FPROC bglUniform4f;
extern PFNGLVERTEXATTRIBPOINTERPROC bglVertexAttribPointer;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC bglEnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC bglDisableVertexAttribArray;

	/* GL_ARB_framebuffer_object or GL_EXT_framebuffer_object */
extern PFNGLGENFRAMEBUFFERSPROC bglGenFramebuffers;
extern PFNGLDELETEFRAMEBUFFERSPROC bglDeleteFramebuffers;
extern PFNGLBINDFRAMEBUFFERPROC bglBindFramebuffer;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC bglFramebufferTexture2D;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC bglCheckFramebufferStatus;
extern PFNGLGENRENDERBUFFERSPROC bglGenRenderbuffers;
extern PFNGLDELETERENDERBUFFERSPROC bglDeleteRenderbuffers;
extern PFNGLBINDRENDERBUFFERPROC bglBindRenderbuffer;
extern PFNGLRENDERBUFFERSTORAGEPROC bglRenderbufferStorage;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC bglFramebufferRenderbuffer;
#endif

#endif
