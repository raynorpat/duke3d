/**************************************************************************************************
"POLYMOST" code written by Ken Silverman
Ken Silverman's official web site: http://www.advsys.net/ken

Motivation:
When 3D Realms released the Duke Nukem 3D source code, I thought somebody would do a OpenGL or
Direct3D port. Well, after a few months passed, I saw no sign of somebody working on a true
hardware-accelerated port of Build, just people saying it wasn't possible. Eventually, I realized
the only way this was going to happen was for me to do it myself. First, I needed to port Build to
Windows. I could have done it myself, but instead I thought I'd ask my Australian buddy, Jonathon
Fowler, if he would upgrade his Windows port to my favorite compiler (MSVC) - which he did. Once
that was done, I was ready to start the "POLYMOST" project.

About:
This source file is basically a complete rewrite of the entire rendering part of the Build engine.
There are small pieces in ENGINE.C to activate this code, and other minor hacks in other source
files, but most of it is in here. If you're looking for polymost-related code in the other source
files, you should find most of them by searching for either "polymost" or "rendmode". Speaking of
rendmode, there are now 4 rendering modes in Build:

	rendmode 0: The original code I wrote from 1993-1997
	rendmode 1: Solid-color rendering: my debug code before I did texture mapping
	rendmode 2: Software rendering before I started the OpenGL code (Note: this is just a quick
						hack to make testing easier - it's not optimized to my usual standards!)
	rendmode 3: The OpenGL code

The original Build engine did hidden surface removal by using a vertical span buffer on the tops
and bottoms of walls. This worked nice back in the day, but it it's not suitable for a polygon
engine. So I decided to write a brand new hidden surface removal algorithm - using the same idea
as the original Build - but one that worked with vectors instead of already rasterized data.

Brief history:
06/20/2000: I release Build Source code
04/01/2003: 3D Realms releases Duke Nukem 3D source code
10/04/2003: Jonathon Fowler gets his Windows port working in Visual C
10/04/2003: I start writing POLYMOST.BAS, a new hidden surface removal algorithm for Build that
					works on a polygon level instead of spans.
10/16/2003: Ported POLYMOST.BAS to C inside JonoF KenBuild's ENGINE.C; later this code was split
					out of ENGINE.C and put in this file, POLYMOST.C.
12/10/2003: Started OpenGL code for POLYMOST (rendmode 3)
12/23/2003: 1st public release
01/01/2004: 2nd public release: fixed stray lines, status bar, mirrors, sky, and lots of other bugs.

----------------------------------------------------------------------------------------------------

Todo list (in approximate chronological order):

High priority:
	*   BOTH: Do accurate software sorting/chopping for sprites: drawing in wrong order is bad :/
	*   BOTH: Fix hall of mirrors near "zenith". Call polymost_drawrooms twice?
	* OPENGL: drawmapview()

Low priority:
	* SOFT6D: Do back-face culling of sprites during up/down/tilt transformation (top of drawpoly)
	* SOFT6D: Fix depth shading: use saturation&LUT
	* SOFT6D: Optimize using hyperbolic mapping (similar to KUBE algo)
	* SOFT6D: Slab6-style voxel sprites. How to accelerate? :/
	* OPENGL: KENBUILD: Write flipping code for floor mirrors
	*   BOTH: KENBUILD: Parallaxing sky modes 1&2
	*   BOTH: Masked/1-way walls don't clip correctly to sectors of intersecting ceiling/floor slopes
	*   BOTH: Editart x-center is not working correctly with Duke's camera/turret sprites
	*   BOTH: Get rid of horizontal line above Duke full-screen status bar
	*   BOTH: Combine ceilings/floors into a single triangle strip (should lower poly count by 2x)
	*   BOTH: Optimize/clean up texture-map setup equations

**************************************************************************************************/

static int32_t animateoffs(short tilenum, short fakevar);

#include <stdint.h>
#include <math.h> //<-important!
#ifdef USE_OPENGL
#include "SDL_opengl.h"
	//Drawing goes through glsub_*, which may batch vertices: GL state calls
	//here and in engine.c below draw the batch first.
#define GLSUB_WRAP
#include "glsubmit.h"
#include "glext.h"
#endif
typedef struct { float x, cy[2], fy[2]; int32_t n, p, tag, ctag, ftag; } vsptyp;
#define VSPMAX 4096 //<- careful!
static vsptyp vsp[VSPMAX];
static int32_t vcnt, gtag;

static double dxb1[MAXWALLSB], dxb2[MAXWALLSB];

#define SCISDIST 1.0 //1.0: Close plane clipping distance
#define USEZBUFFER 1 //1:use zbuffer (slow, nice sprite rendering), 0:no zbuffer (fast, bad sprite rendering)
#define LINTERPSIZ 4 //log2 of interpolation size. 4:pretty fast&acceptable quality, 0:best quality/slow!
#define DEPTHDEBUG 0 //1:render distance instead of texture, for debugging only!, 0:default
#define PI 3.14159265358979323

static double gyxscale, gxyaspect, gviewxrange, ghalfx, grhalfxdown10, grhalfxdown10x, ghoriz;
static double gcosang, gsinang, gcosang2, gsinang2;
static double gchang, gshang, gctang, gstang;
static float gtang = 0.0;
double guo, gux, guy; //Screen-based texture mapping parameters
double gvo, gvx, gvy;
double gdo, gdx, gdy;

#if (USEZBUFFER != 0)
int32_t *zbufmem = 0, zbufysiz = 0, zbufbpl = 0;
#endif
static int32_t zbufwrite = 1; //0: renderer 2's spans don't write the z-buffer

#ifdef USE_OPENGL
int32_t glredbluemode = 0;
static int32_t lastglredbluemode = 0, redblueclearcnt = 0;

static struct glfiltermodes {
	char *name;
	int32_t min,mag;
} glfiltermodes[] = {
	{"GL_NEAREST",GL_NEAREST,GL_NEAREST},
	{"GL_LINEAR",GL_LINEAR,GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST",GL_NEAREST_MIPMAP_NEAREST,GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST",GL_LINEAR_MIPMAP_NEAREST,GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR",GL_NEAREST_MIPMAP_LINEAR,GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR",GL_LINEAR_MIPMAP_LINEAR,GL_LINEAR}
};
#define numglfiltermodes (sizeof(glfiltermodes)/sizeof(glfiltermodes[0]))

int32_t glanisotropy = 1;            // 0 = maximum supported by card
int glusetexcompr = 0;
int32_t gltexfiltermode = 0;   // GL_NEAREST
int32_t gltexmaxsize = 0;      // 0 means autodetection on first run
int32_t gltexmiplevel = 0;		// discards this many mipmap levels
extern char nofog;
#endif

static inline void ftol (float f, int32_t *a)
{
	*a = (int32_t)lrintf(f); //rounds to nearest, as the x87 fistp it replaces
}

static inline void dtol (double d, int32_t *a)
{
	*a = (int32_t)lrint(d);
}

static inline int32_t imod (int32_t a, int32_t b)
{
	if (a >= 0) return(a%b);
	return(((a+1)%b)+b-1);
}

void drawline2d (float x0, float y0, float x1, float y1, char col)
{
	float f, dx, dy, fxres, fyres;
	int32_t e, inc, x, y;
	uint32_t up16;

	dx = x1-x0; dy = y1-y0; if ((dx == 0) && (dy == 0)) return;
	fxres = (float)xdimen; fyres = (float)ydimen;
		  if (x0 >= fxres) { if (x1 >= fxres) return; y0 += (fxres-x0)*dy/dx; x0 = fxres; }
	else if (x0 <      0) { if (x1 <      0) return; y0 += (    0-x0)*dy/dx; x0 =     0; }
		  if (x1 >= fxres) {                          y1 += (fxres-x1)*dy/dx; x1 = fxres; }
	else if (x1 <      0) {                          y1 += (    0-x1)*dy/dx; x1 =     0; }
		  if (y0 >= fyres) { if (y1 >= fyres) return; x0 += (fyres-y0)*dx/dy; y0 = fyres; }
	else if (y0 <      0) { if (y1 <      0) return; x0 += (    0-y0)*dx/dy; y0 =     0; }
		  if (y1 >= fyres) {                          x1 += (fyres-y1)*dx/dy; y1 = fyres; }
	else if (y1 <      0) {                          x1 += (    0-y1)*dx/dy; y1 =     0; }

	if (fabs(dx) > fabs(dy))
	{
		if (x0 > x1) { f = x0; x0 = x1; x1 = f; f = y0; y0 = y1; y1 = f; }
		y = (int32_t)(y0*65536.f)+32768;
		inc = (int32_t)(dy/dx*65536.f+.5f);
		x = (int32_t)(x0+.5); if (x < 0) { y -= inc*x; x = 0; } //if for safety
		e = (int32_t)(x1+.5); if (e > xdimen) e = xdimen;       //if for safety
		up16 = (ydimen<<16);
		for(;x<e;x++,y+=inc) if ((uint32_t)y < up16) *(char *)(ylookup[y>>16]+x+frameoffset) = col;
	}
	else
	{
		if (y0 > y1) { f = x0; x0 = x1; x1 = f; f = y0; y0 = y1; y1 = f; }
		x = (int32_t)(x0*65536.f)+32768;
		inc = (int32_t)(dx/dy*65536.f+.5f);
		y = (int32_t)(y0+.5); if (y < 0) { x -= inc*y; y = 0; } //if for safety
		e = (int32_t)(y1+.5); if (e > ydimen) e = ydimen;       //if for safety
		up16 = (xdimen<<16);
		for(;y<e;y++,x+=inc) if ((uint32_t)x < up16) *(char *)(ylookup[y]+(x>>16)+frameoffset) = col;
	}
}

#ifdef USE_OPENGL
typedef struct { unsigned char r, g, b, a; } coltype;

static void uploadtexture(int32_t doalloc, int32_t xsiz, int32_t ysiz, int32_t intexfmt, int32_t texfmt, coltype *pic, int32_t tsizx, int32_t tsizy, int32_t dameth);



//--------------------------------------------------------------------------------------------------
//TEXTURE MANAGEMENT: treats same texture with different .PAL as a separate texture. This makes the
//   max number of virtual textures very large (MAXTILES*256). Instead of allocating a handle for
//   every virtual texture, I use a cache where indexing is managed through a hash table.
//
//static unsigned int glpic[MAXTILES*256]; //NOTE:9MB! Use hash instead!
#define GLTEXCACHEADSIZ 4096 //Note: Must be power of 2
static int32_t gltexcachead[GLTEXCACHEADSIZ];
#define GLTEXMAX 65536
static unsigned int glpic[GLTEXMAX];
	//Glow texture of each cache slot (0: none): its texels at or above polymosttexfullbright
static GLuint glpicfb[GLTEXMAX];
int32_t polymosttexfullbright = 256; //first fullbright palette index (256: off)
uint32_t gl_rgbtexloads = 0, gl_rgbtexinvalidations = 0; //test hooks: gloadtile and gltexinvalidateall calls
uint32_t gl_transcopies = 0; //test hook: copies of the frame made for translucent draws
int32_t gl_transcopyall = 0; //test hook: copy all of the frame before every translucent draw
static int32_t gltexcacind[GLTEXMAX], gltexcacnext[GLTEXMAX], gltexcacnum = -1;

	//GL 2.1 tier (gldrawpath GLPATH_SHADER): everything draws classic's palette
	//   indices into a framebuffer that keeps the frame, as classic's does: the 3D
	//   view palookup[pal][shade][texel] with the shade worked out per pixel, 2D
	//   tiles the same with their own shade, and lines, text and clears the index
	//   itself. Showing it (polymost_present_shader) draws that into the window
	//   through the palette, faded as classic fades the 8-bit palette.
static GLuint glshprog = 0, glshtprog = 0, glshresolve = 0; //the index, translucent index and resolve programs
static GLuint glshdown = 0; //the resolve of a supersampled frame (linked on its own)
static int32_t glshdownfailed = 0; //it didn't compile or link: no supersampling this run
int32_t glshsupersampled = 0; //sdl_driver.c: the frame is supersampled, shown with glshdown
static GLuint glshfbo = 0, glshfbotex = 0, glshdepth = 0; //the framebuffer: an index in red, and depth
static int32_t glshfbox = 0, glshfboy = 0; //its size
static GLuint glshlookup = 0, glshpal = 0; //the palookups (a row per pal and shade) and the palette
static int32_t glshlookupdirty = 1; //makepalookup changed one
static palette_t glshpalnow[256]; //what glshpal holds
void getfadedpalette(palette_t *pal); //engine.c: the palette on screen
static int32_t glshbound = 0; //drawing goes into the framebuffer (until it's shown)
static int32_t glshprogon = 0; //with the index program (1) or the translucent one (2)
static int32_t glscene = 0; //the 3D view is drawing (its depth cleared, its distance shade on)
static int32_t glffcleared = 0; //1.1/1.5: this frame's depth is cleared
static GLuint glshdst = 0, glshtrans = 0; //a copy of the framebuffer (unit 3) and transluc (unit 2)
	//Translucent draws blend with the framebuffer's index under them, read from
	//   glshdst: if glshcopied, it holds the framebuffer but for what was drawn
	//   since, glshdirty's boxes (pixels, bottom-up: x0, y0, x1, y1). A copy is
	//   of all the frame: on macOS's driver each costs about the same whatever
	//   its size, so fewer copies are what's fast.
#define GLSHDIRTYMAX 64
static int32_t glshcopied = 0, glshdirty[GLSHDIRTYMAX][4], glshndirty = 0;
static GLuint glidxtex[MAXTILES][2]; //each tile's index texture, repeating and clamped (0: none)
static char glidxdirty[MAXTILES][2]; //its art changed
static float glskyshade = 0.f; //classic's distance shade of the parallaxed sky, while drawing it

	//Texture filtering on the GL 2.1 tier (gltexturefilter bilinear or
	//   trilinear): the frame holds colour instead of indices. Each texel of
	//   a tile is looked up as the index program does, at the pixel's shade
	//   (classic's shade tables, exactly), into glshrgb (the palette's colour
	//   of palookup[pal][shade][index], a row per pal and shade as in
	//   glshlookup), then filtered. Translucent draws blend at 0.66 and 0.33
	//   (as the 1.1/1.5 paths do), and the present fades the colours toward
	//   the fade's: approximations of classic's transluc and faded palette.
	//   Trilinear's mip levels (glidxmip, polymost_idxmip) hold four indices
	//   a texel, each looked up the same.
static GLuint glshcprog[2] = {0,0}; //the colour programs: bilinear, trilinear (0: not linked yet)
static GLuint glshcres = 0, glshcdown = 0; //the present of colour, and of a supersampled frame
static int32_t glshcfailed[4]; //bilinear's, trilinear's, the present's and the supersampled present's
	//   didn't link: 1 for the run, 2 a test hook's (until a mode set)
static int32_t glshcolor = 0; //the frame holds colour, drawn with this filter (0: indices)
static GLuint glshrgb = 0; //the palookups' colours (on unit 1, as glshlookup)
static palette_t glshrgbpal[256]; //the palette glshrgb was made with
static int32_t glshrgbdirty = 1; //the palookups changed since
static GLint glshcloc[2][3]; //the colour programs' tsize, msize and clampuv
static float glshcval[5] = {-1.f,-1.f,-1.f,-1.f,-1.f}; //what they're set to (-1: not known)
static GLuint glshmipon = 0; //the mip levels bound on unit 5
static GLuint glidxmip[MAXTILES][2]; //a tile's mip levels for trilinear (0: none)
static int32_t glidxsize[MAXTILES][2][4]; //its index texture's size, and its mip levels' texture's (0: none made)

void gloadtile(int32_t,int32_t,int32_t,int32_t,int32_t);
static int32_t gltexcache (int32_t dapicnum, int32_t dapalnum, int32_t dameth)
{
	int32_t i, j, k;

		//Generate unique index from any combination of input parameters
	i = (((dameth&4)<<6) + dapalnum)*MAXTILES + dapicnum;

	j = (i&(GLTEXCACHEADSIZ-1));
	for(k=gltexcachead[j];k>=0;k=gltexcacnext[k])
		if ((gltexcacind[k]&0x7fffffff) == i)
		{
			if (gltexcacind[k] < 0)
			{
				gltexcacind[k] &= 0x7fffffff;
				gloadtile(dapicnum,dapalnum,dameth,k,0); //reload tile (for animations)
			}
			return(k);
		}

	if (gltexcacnum >= GLTEXMAX) return(0); //safe way to handle too many virtual textures

	gltexcacind[gltexcacnum] = i;
	gltexcacnext[gltexcacnum] = gltexcachead[j]; gltexcachead[j] = gltexcacnum;
	gloadtile(dapicnum,dapalnum,dameth,gltexcacnum,1);
	gltexcacnum++;
	return(gltexcacnum-1);
}

void gltexinvalidate (int32_t dapicnum, int32_t dapalnum, int32_t dameth)
{
	int32_t i, j, k;

		if ((uint32_t)dapicnum < MAXTILES) glidxdirty[dapicnum][(dameth&4)>>2] = 1;

		//Generate unique index from any combination of input parameters
	i = (((dameth&4)<<6) + dapalnum)*MAXTILES + dapicnum;

	j = (i&(GLTEXCACHEADSIZ-1));
	for(k=gltexcachead[j];k>=0;k=gltexcacnext[k])
		if (gltexcacind[k] == i)
			{ gltexcacind[k] |= 0x80000000; return; }
}

	//Make all textures "dirty" so they reload, but not re-allocate
	//This should be much faster than polymost_glreset()
	//Use this for palette effects ... but not ones that change every frame!
void gltexinvalidateall ()
{
	int32_t i;
	gl_rgbtexinvalidations++;
	for(i=gltexcacnum-1;i>=0;i--) gltexcacind[i] |= 0x80000000;
}


void gltexapplyprops ()
{
	int32_t i;
	
	if (glinfo.maxanisotropy > 1.0)
	{
		if (glanisotropy <= 0 || glanisotropy > glinfo.maxanisotropy) glanisotropy = glinfo.maxanisotropy;
	}
	
	if (gltexfiltermode < 0) gltexfiltermode = 0;
	else if (gltexfiltermode >= numglfiltermodes) gltexfiltermode = numglfiltermodes-1;
	for(i=gltexcacnum-1;i>=0;i--) {
		glBindTexture(GL_TEXTURE_2D,glpic[i]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,glfiltermodes[gltexfiltermode].mag);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,glfiltermodes[gltexfiltermode].min);
		if (glinfo.maxanisotropy > 1.0)
			glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,glanisotropy);
		if (!glpicfb[i]) continue;
		glBindTexture(GL_TEXTURE_2D,glpicfb[i]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,glfiltermodes[gltexfiltermode].mag);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,glfiltermodes[gltexfiltermode].min);
		if (glinfo.maxanisotropy > 1.0)
			glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,glanisotropy);
	}
}

//--------------------------------------------------------------------------------------------------

static float glox1, gloy1, glox2, gloy2;
static int32_t fog_is_on = 0;

	//The palette's shade tables (what classic draws) as brightness against shade 0, in
	//   the colours GL is given: a vertex colour per shade. hist weights indices 0-239 (not
	//   the lit ones or 255). Returns the shade where a straight ramp with the same total
	//   brightness reaches black (1-s/z summed over shades 0 to z is (z+1)/2), -1 if none.
static float polymost_buildramp (const int32_t *hist, float *ramp, int32_t n)
{
	int32_t c, j, s;
	float sum, sum0 = 0.f, zero = -1.f;

	if (!palookup[0]) return(-1.f);
	for(s=0;s<n;s++)
	{
		sum = 0.f;
		for(c=0;c<240;c++)
			if (hist[c])
			{
				j = (int32_t)((unsigned char)palookup[0][(s<<8)+c]);
				sum += (float)hist[c]*(float)(curpalette[j].r+curpalette[j].g+curpalette[j].b);
			}
		if (!s) { sum0 = sum; if (sum0 <= 0.f) return(-1.f); }
		ramp[s] = sum/sum0; zero += ramp[s]*2.f;
	}
	return(zero);
}

	//The whole palette's ramp; glshaderampn = 0: not built
static float glshaderamp[256], glshadezero;
static int32_t glshaderampn = 0;

	//At a palette change (and on first use): the ramp of indices 0-239 alike
void polymost_buildshaderamp ()
{
	int32_t i, hist[240];

	for(i=0;i<240;i++) hist[i] = 1;
	glshaderampn = min(max((int32_t)numpalookups,1),256);
	glshadezero = polymost_buildramp(hist,glshaderamp,glshaderampn);
	if (glshadezero < 0.f) //no palette yet: linear
	{
		for(i=0;i<glshaderampn;i++) glshaderamp[i] = ((float)(glshaderampn-i))/((float)glshaderampn);
		glshadezero = (float)glshaderampn;
	}
}

	//Each tile's own ramp, weighted by its texels (built as its texture is made): the
	//   whole palette's is too bright for dark art, which rounds to black sooner
#define TILERAMPN 32
static float gltileramp[MAXTILES][TILERAMPN], gltilezero[MAXTILES]; //gltilezero <= 0: none

	//The vertex colour of a shade of a tile; *zero gets its ramp's black shade
static float polymost_shaderamp (int32_t picnum, int32_t shade, float *zero)
{
	if (((uint32_t)picnum < MAXTILES) && (gltilezero[picnum] > 0.f) && (numpalookups <= TILERAMPN))
	{
		*zero = gltilezero[picnum];
		return(gltileramp[picnum][min(max(shade,0),numpalookups-1)]);
	}
	if (!glshaderampn) polymost_buildshaderamp();
	*zero = glshadezero;
	return(glshaderamp[min(max(shade,0),glshaderampn-1)]);
}

	//Classic's distance shading (getpalookup with drawsprite's swall and globvis)
	//   adds (depth*viewingrange*xdimscale*globvis)>>51 shades, depth in map units,
	//   globvis = globalvisibility*(uchar)(vis+16)/16. Polymost's eye z is
	//   depth*viewingrange/(gxyaspect*gyxscale*2^30), so per unit of eye z it adds:
static float glfogshades = 0.f;
static float glfogstart = 0.f, glfogend = 1.f; //what GL has

static void polymost_fogvis (int32_t vis)
{
	glfogshades = (float)(gxyaspect*gyxscale*((double)xdimscale)*((double)globalvisibility)*
		((double)((unsigned char)(vis+16)))/(16.0*2097152.0));
}

	//Fogs a surface of a shade as classic darkens it, over its polymost_shaderamp
	//   colour: linear fog to black from where its shade passes 0 (a negative shade
	//   stays lit for a while) to where its ramp's straight line reaches black (zero)
static void polymost_setfog (int32_t shade, float zero)
{
	float start, end;

	if (glfogshades <= 0.f) { start = 1e9f; end = 2e9f; } //vis+16 wrapped to 0: no fog
	else
	{
		start = ((float)max(-shade,0))/glfogshades;
		end = (zero-(float)shade)/glfogshades;
		if (end <= start) end = start+1.f/glfogshades; //black already
	}
	if (start != glfogstart) { glFogf(GL_FOG_START,start); glfogstart = start; }
	if (end != glfogend) { glFogf(GL_FOG_END,end); glfogend = end; }
}

	//The index program: classic's shade is the surface's (r, a whole number) plus the
	//   whole shades of its distance, shades per unit of eye z (b) times its eye z, as
	//   getpalookup's shade+(davis>>8) (gl_Color isn't clamped); pal is g. 255 is
	//   see-through when a is 1 (masked), drawn as the others when it's 0 (opaque).
	//   The translucent program (glshtprog) is the same but for its end: a is 2 or 3,
	//   classic's two translucent levels, masked too, and it draws the index under
	//   it (from the copy, dest) and its own through transluc, as a.c's tvlineasm1
	//   does, transluc[(under<<8)+own] at 2 and transluc[(own<<8)+under] at 3.
static const char glshidxvert[] =
	"#version 120\n"
	"uniform float numshades;\n"
	"varying vec2 uv;\n"
	"varying float shade, dist, row, masked;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = ftransform();\n"
	"    uv = gl_MultiTexCoord0.st;\n"
	"    shade = gl_Color.r;\n"
	"    dist = abs((gl_ModelViewMatrix*gl_Vertex).z)*gl_Color.b;\n"
	"    row = gl_Color.g*numshades;\n"
	"    masked = gl_Color.a;\n"
	"}\n";
static const char glshidxfrag[] =
	"#version 120\n"
	"uniform sampler2D tile, lookup;\n"
	"uniform float numshades, rowscale;\n"
	"varying vec2 uv;\n"
	"varying float shade, dist, row, masked;\n"
	"void main()\n"
	"{\n"
	"    float i = texture2D(tile,uv).r;\n"
	"    if ((i > 254.5/255.0) && (masked > 0.5)) discard;\n"
	"    float s = clamp(floor(shade+0.5)+floor(dist),0.0,numshades-1.0);\n"
	"    gl_FragColor = vec4(texture2D(lookup,vec2((i*255.0+0.5)/256.0,(row+s+0.5)*rowscale)).r,0.0,0.0,1.0);\n"
	"}\n";
static const char glshtransfrag[] =
	"#version 120\n"
	"uniform sampler2D tile, lookup, trans, dest;\n"
	"uniform float numshades, rowscale;\n"
	"uniform vec2 invsize;\n"
	"varying vec2 uv;\n"
	"varying float shade, dist, row, masked;\n"
	"void main()\n"
	"{\n"
	"    float i = texture2D(tile,uv).r;\n"
	"    if (i > 254.5/255.0) discard;\n"
	"    float s = clamp(floor(shade+0.5)+floor(dist),0.0,numshades-1.0);\n"
	"    float c = texture2D(lookup,vec2((i*255.0+0.5)/256.0,(row+s+0.5)*rowscale)).r;\n"
	"    float d = texture2D(dest,gl_FragCoord.xy*invsize).r;\n"
	"    gl_FragColor = vec4(texture2D(trans,((masked > 2.5) ? vec2(d,c) : vec2(c,d))*(255.0/256.0)+0.5/256.0).r,0.0,0.0,1.0);\n"
	"}\n";
	//The present: each index through the palette
static const char glshresvert[] =
	"#version 120\n"
	"void main()\n"
	"{\n"
	"    gl_Position = ftransform();\n"
	"    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
	"}\n";
static const char glshresfrag[] =
	"#version 120\n"
	"uniform sampler2D frame, pal;\n"
	"void main()\n"
	"{\n"
	"    float i = texture2D(frame,gl_TexCoord[0].st).r;\n"
	"    gl_FragColor = vec4(texture2D(pal,vec2((i*255.0+0.5)/256.0,0.5)).rgb,1.0);\n"
	"}\n";
	//The present of a supersampled frame (larger than the window): each pixel the
	//   mean of the texels it covers through the palette, each weighted by how
	//   much of it is covered (scale: texels per pixel, origin: where it shows)
static const char glshdownfrag[] =
	"#version 120\n"
	"#define GLSHDOWNTAPS 16\n" //texels per axis: a scale of at most GLSHDOWNMAXSCALE
	"uniform sampler2D frame, pal;\n"
	"uniform vec2 scale, origin, invsize;\n"
	"void main()\n"
	"{\n"
	"    vec2 a = (gl_FragCoord.xy-origin-0.5)*scale, b = a+scale;\n"
	"    vec3 sum = vec3(0.0);\n"
	"    float x, y = floor(a.y), wy, i;\n"
	"    for(int j=0;j<GLSHDOWNTAPS;j++)\n"
	"    {\n"
	"        if (y >= b.y) break;\n"
	"        wy = min(y+1.0,b.y)-max(y,a.y);\n"
	"        x = floor(a.x);\n"
	"        for(int k=0;k<GLSHDOWNTAPS;k++)\n"
	"        {\n"
	"            if (x >= b.x) break;\n"
	"            i = texture2D(frame,(vec2(x,y)+0.5)*invsize).r;\n"
	"            sum += (min(x+1.0,b.x)-max(x,a.x))*wy*texture2D(pal,vec2((i*255.0+0.5)/256.0,0.5)).rgb;\n"
	"            x += 1.0;\n"
	"        }\n"
	"        y += 1.0;\n"
	"    }\n"
	"    gl_FragColor = vec4(sum/(scale.x*scale.y),1.0);\n"
	"}\n";
	//The colour programs (texture filtering), with the index program's vertex
	//   shader: tap is index i's colour at the pixel's shade, alpha its cover
	//   (0 for a see-through 255 when masked); level0 is the four texels of
	//   the tile (tsize big) around uv blended (bilinear). Trilinear blends
	//   two levels, picked as GL picks them from how many texels a pixel
	//   covers: level l >= 1 is in mips (msize big) at x off, each texel the
	//   mean of its four indices; clampuv: the tile doesn't repeat.
	//   Translucent draws (masked 2, 3) are blended at 0.66 and 0.33.
#define GLSHCOLORFRAG \
	"uniform sampler2D tile, rgb, mips;\n" \
	"uniform float numshades, rowscale, clampuv;\n" \
	"uniform vec2 tsize, msize;\n" \
	"varying vec2 uv;\n" \
	"varying float shade, dist, row, masked;\n" \
	"float rowt, see;\n" \
	"vec4 tap(float i)\n" \
	"{\n" \
	"    if ((i > 254.5/255.0) && (see > 0.5)) return vec4(0.0);\n" \
	"    return vec4(texture2D(rgb,vec2((i*255.0+0.5)/256.0,rowt)).rgb,1.0);\n" \
	"}\n" \
	"vec4 level0()\n" \
	"{\n" \
	"    vec2 t = uv*tsize-0.5, f = fract(t), b = (floor(t)+0.5)/tsize, d = 1.0/tsize;\n" \
	"    return mix(mix(tap(texture2D(tile,b).r),tap(texture2D(tile,b+vec2(d.x,0.0)).r),f.x),\n" \
	"               mix(tap(texture2D(tile,b+vec2(0.0,d.y)).r),tap(texture2D(tile,b+d).r),f.x),f.y);\n" \
	"}\n" \
	"#ifdef TRILINEAR\n" \
	"vec4 mtexel(vec2 p, vec2 sz, float off)\n" \
	"{\n" \
	"    p = (clampuv > 0.5) ? clamp(p,vec2(0.0),sz-1.0) : mod(p,sz);\n" \
	"    vec4 i = texture2D(mips,(vec2(off,0.0)+p+0.5)/msize);\n" \
	"    return (tap(i.r)+tap(i.g)+tap(i.b)+tap(i.a))*0.25;\n" \
	"}\n" \
	"vec4 level(float l)\n" \
	"{\n" \
	"    if (l < 0.5) return level0();\n" \
	"    float off = 0.0;\n" \
	"    vec2 sz = tsize;\n" \
	"    for(int k=1;k<16;k++)\n" \
	"    {\n" \
	"        sz = max(floor(sz*0.5),1.0);\n" \
	"        if (float(k) >= l) break;\n" \
	"        off += sz.x;\n" \
	"    }\n" \
	"    vec2 t = uv*sz-0.5, f = fract(t), b = floor(t);\n" \
	"    return mix(mix(mtexel(b,sz,off),mtexel(b+vec2(1.0,0.0),sz,off),f.x),\n" \
	"               mix(mtexel(b+vec2(0.0,1.0),sz,off),mtexel(b+1.0,sz,off),f.x),f.y);\n" \
	"}\n" \
	"#endif\n" \
	"void main()\n" \
	"{\n" \
	"    float s = clamp(floor(shade+0.5)+floor(dist),0.0,numshades-1.0);\n" \
	"    rowt = (row+s+0.5)*rowscale;\n" \
	"    see = (masked > 0.5) ? 1.0 : 0.0;\n" \
	"#ifdef TRILINEAR\n" \
	"    vec2 dx = dFdx(uv*tsize), dy = dFdy(uv*tsize);\n" \
	"    float lod = 0.5*log2(max(dot(dx,dx),dot(dy,dy))), top = floor(log2(max(tsize.x,tsize.y))+0.001);\n" \
	"    vec4 c;\n" \
	"    if ((lod <= 0.0) || (top < 1.0)) c = level0();\n" \
	"    else\n" \
	"    {\n" \
	"        lod = min(lod,top);\n" \
	"        float l = floor(lod);\n" \
	"        c = mix(level(l),level(min(l+1.0,top)),lod-l);\n" \
	"    }\n" \
	"#else\n" \
	"    vec4 c = level0();\n" \
	"#endif\n" \
	"    if (c.a < 0.5) discard;\n" \
	"    gl_FragColor = vec4(c.rgb/c.a,(masked > 2.5) ? 0.33 : ((masked > 1.5) ? 0.66 : 1.0));\n" \
	"}\n"
static const char glshbilfrag[] = "#version 120\n" GLSHCOLORFRAG;
static const char glshtrifrag[] = "#version 120\n#define TRILINEAR\n" GLSHCOLORFRAG;
	//The present of colour: faded toward fade.rgb by fade.a (as getfadedpalette
	//   moves each colour), contrast and saturation (cs) as gl_colorcorrect,
	//   darkened by bright; a supersampled frame box-filtered as glshdownfrag
#define GLSHCOLORPOST \
	"uniform vec4 fade;\n" \
	"uniform vec2 cs;\n" \
	"uniform float bright;\n" \
	"vec3 post(vec3 c)\n" \
	"{\n" \
	"    c = mix(c,fade.rgb,fade.a);\n" \
	"    float l = dot(c,vec3(0.2126,0.7152,0.0722));\n" \
	"    return clamp((l+cs.y*(c-l)-0.5)*cs.x+0.5,0.0,1.0);\n" \
	"}\n"
static const char glshcresfrag[] =
	"#version 120\n"
	"uniform sampler2D frame;\n"
	GLSHCOLORPOST
	"void main()\n"
	"{\n"
	"    gl_FragColor = vec4(post(texture2D(frame,gl_TexCoord[0].st).rgb)*bright,1.0);\n"
	"}\n";
static const char glshcdownfrag[] =
	"#version 120\n"
	"#define GLSHDOWNTAPS 16\n"
	"uniform sampler2D frame;\n"
	"uniform vec2 scale, origin, invsize;\n"
	GLSHCOLORPOST
	"void main()\n"
	"{\n"
	"    vec2 a = (gl_FragCoord.xy-origin-0.5)*scale, b = a+scale;\n"
	"    vec3 sum = vec3(0.0);\n"
	"    float x, y = floor(a.y), wy;\n"
	"    for(int j=0;j<GLSHDOWNTAPS;j++)\n"
	"    {\n"
	"        if (y >= b.y) break;\n"
	"        wy = min(y+1.0,b.y)-max(y,a.y);\n"
	"        x = floor(a.x);\n"
	"        for(int k=0;k<GLSHDOWNTAPS;k++)\n"
	"        {\n"
	"            if (x >= b.x) break;\n"
	"            sum += (min(x+1.0,b.x)-max(x,a.x))*wy*post(texture2D(frame,(vec2(x,y)+0.5)*invsize).rgb);\n"
	"            x += 1.0;\n"
	"        }\n"
	"        y += 1.0;\n"
	"    }\n"
	"    gl_FragColor = vec4(sum*bright/(scale.x*scale.y),1.0);\n"
	"}\n";
static const char glshbroken[] = "#version 120\nvoid main() { broken }\n"; //gl_forcefail 2

static GLuint polymost_shcompile (GLenum type, const char *src)
{
	GLuint s;
	GLint ok = 0;
	char log[1024];

	s = bglCreateShader(type);
	bglShaderSource(s,1,(const GLchar **)&src,NULL);
	bglCompileShader(s);
	bglGetShaderiv(s,GL_COMPILE_STATUS,&ok);
	if (ok) return(s);
	log[0] = 0; bglGetShaderInfoLog(s,sizeof(log),NULL,log);
	printf("OpenGL shader didn't compile: %s\n",log);
	bglDeleteShader(s);
	return(0);
}

	//A program of two shaders, its samplers tex0 and tex1 on texture units 0 and 1 (0: failed)
static GLuint polymost_shlink (const char *vs, const char *fs, const char *tex0, const char *tex1)
{
	GLuint v, f, p = 0;
	GLint ok = 0;
	char log[1024];

	v = polymost_shcompile(GL_VERTEX_SHADER,vs);
	f = polymost_shcompile(GL_FRAGMENT_SHADER,fs);
	if ((v) && (f))
	{
		p = bglCreateProgram();
		bglAttachShader(p,v); bglAttachShader(p,f);
		bglLinkProgram(p);
		bglGetProgramiv(p,GL_LINK_STATUS,&ok);
		if (!ok)
		{
			log[0] = 0; bglGetProgramInfoLog(p,sizeof(log),NULL,log);
			printf("OpenGL shader didn't link: %s\n",log);
			bglDeleteProgram(p); p = 0;
		}
	}
	if (v) bglDeleteShader(v);
	if (f) bglDeleteShader(f);
	if (p)
	{
		bglUseProgram(p);
		bglUniform1i(bglGetUniformLocation(p,tex0),0);
		bglUniform1i(bglGetUniformLocation(p,tex1),1);
		bglUseProgram(0);
	}
	return(p);
}

	//Drops what the GL 2.1 tier made (its context current)
static void polymost_shaderfree ()
{
	int32_t i;

	glsub_flush();
	if (glshbound) { bglBindFramebuffer(GL_FRAMEBUFFER,0); glshbound = 0; }
	glscene = glshprogon = glffcleared = 0;
	if ((glshprog) || (glshtprog) || (glshresolve) || (glshdown)) bglUseProgram(0);
	for(i=0;i<2;i++)
		if (glshcprog[i]) { bglUseProgram(0); bglDeleteProgram(glshcprog[i]); glshcprog[i] = 0; }
	if (glshcres) { bglUseProgram(0); bglDeleteProgram(glshcres); glshcres = 0; }
	if (glshcdown) { bglUseProgram(0); bglDeleteProgram(glshcdown); glshcdown = 0; }
	if (glshrgb) { glDeleteTextures(1,&glshrgb); glshrgb = 0; }
	glshrgbdirty = 1;
	glshcolor = 0;
	glshmipon = 0;
	for(i=0;i<4;i++) if (glshcfailed[i] == 2) glshcfailed[i] = 0; //the test hooks', until a mode set
	if (glshprog) { bglDeleteProgram(glshprog); glshprog = 0; }
	if (glshtprog) { bglDeleteProgram(glshtprog); glshtprog = 0; }
	if (glshresolve) { bglDeleteProgram(glshresolve); glshresolve = 0; }
	if (glshdown) { bglDeleteProgram(glshdown); glshdown = 0; }
	if (glshfbo) { bglDeleteFramebuffers(1,&glshfbo); glshfbo = 0; }
	if (glshdepth) { bglDeleteRenderbuffers(1,&glshdepth); glshdepth = 0; }
	if (glshfbotex) { glDeleteTextures(1,&glshfbotex); glshfbotex = 0; }
	if (glshdst) { glDeleteTextures(1,&glshdst); glshdst = 0; }
	if (glshtrans) { glDeleteTextures(1,&glshtrans); glshtrans = 0; }
	glshcopied = 0;
	if (glshlookup) { glDeleteTextures(1,&glshlookup); glshlookup = 0; }
	if (glshpal) { glDeleteTextures(1,&glshpal); glshpal = 0; }
	glshfbox = glshfboy = 0;
	glshlookupdirty = 1;
}

	//The supersampled present's program, linked on its own when first wanted (a
	//   driver that can't link its loops loses supersampling, not the tier).
	//   1: it's ready. gl_forcefail 5 (a test hook) breaks it, without
	//   making the failure stick.
int32_t polymost_supersample_ready ()
{
	if ((!glinfo.glsl) || (glshdownfailed)) return(0);
	if ((glshdown) && (gl_forcefail != 5)) return(1);
	glsub_flush();
	if (gl_forcefail == 5)
	{
		GLuint p = polymost_shlink(glshresvert,glshbroken,"frame","pal");
		if (p) bglDeleteProgram(p);
		printf("OpenGL supersampling is unavailable: its shader doesn't compile or link.\n");
		glshprogon = 0;
		return(0);
	}
	glshdown = polymost_shlink(glshresvert,glshdownfrag,"frame","pal");
	glshprogon = 0; //polymost_shlink leaves no program in use
	if (!glshdown)
	{
		glshdownfailed = 1;
		printf("OpenGL supersampling is unavailable: its shader doesn't compile or link.\n");
		return(0);
	}
	return(1);
}

	//The GL 2.1 tier can't draw: says why and draws on the vbo path from now on
static void polymost_shaderfail (const char *why)
{
	printf("OpenGL path shader failed (%s); using vbo.\n",why);
	polymost_shaderfree();
	gldrawpath = GLPATH_VBO;
	glsub_uninit(); glsub_init(); //vertices into a buffer, as the vbo path sends them
	gltexinvalidateall(); //palette changes didn't retexture on the shader path
	polymost_buildshaderamp();
}

	//Colour program k (0 bilinear, 1 trilinear, 2 the present, 3 the
	//   supersampled present), linked on its own when first wanted; one that
	//   doesn't link is unavailable for the run, the others not. 1: ready.
	//   Test hooks break some until a mode set: gl_forcefail 6 all but the
	//   supersampled present, 7 trilinear, 8 the supersampled present.
static int32_t polymost_clink (int32_t k)
{
	static const char *const fs[4] = { glshbilfrag, glshtrifrag, glshcresfrag, glshcdownfrag };
	static const char *const what[4] = { "bilinear texture filtering", "trilinear texture filtering",
		"texture filtering", "texture filtering while supersampling" };
	static const char *const locname[3] = { "tsize", "msize", "clampuv" };
	GLuint *prog = (k < 2) ? &glshcprog[k] : ((k == 2) ? &glshcres : &glshcdown), p;
	int32_t i, broken;

	broken = ((gl_forcefail == 6) && (k < 3)) || ((gl_forcefail == 7) && (k == 1)) || ((gl_forcefail == 8) && (k == 3));
	if (glshcfailed[k]) return(0);
	if ((*prog) && (!broken)) return(1);
	glsub_flush();
	if (broken)
	{
		p = polymost_shlink(glshresvert,glshbroken,"frame","");
		if (p) bglDeleteProgram(p);
	}
	else *prog = polymost_shlink((k < 2)?glshidxvert:glshresvert,fs[k],(k < 2)?"tile":"frame",(k < 2)?"rgb":"");
	glshprogon = 0; //polymost_shlink leaves no program in use
	if ((broken) || (!*prog))
	{
		glshcfailed[k] = broken ? 2 : 1;
		printf("OpenGL %s is unavailable: its shader doesn't compile or link.\n",what[k]);
		return(0);
	}
	if (k < 2)
	{
		bglUseProgram(*prog);
		bglUniform1i(bglGetUniformLocation(*prog,"mips"),5);
		bglUseProgram(0);
		for(i=0;i<3;i++) glshcloc[k][i] = bglGetUniformLocation(*prog,locname[i]);
		glshlookupdirty = 1; //(its numshades and rowscale)
	}
	return(1);
}

	//Filter f's programs (and the present's; a supersampled frame's too while
	//   supersampled: without it filtering isn't offered then, and the frame is
	//   drawn nearest, supersampled). f when they're ready, 0 when not.
static int32_t polymost_colorready (int32_t f)
{
	if ((f <= 0) || (f > 2) || (!glinfo.glsl)) return(0);
	if ((!polymost_clink(f-1)) || (!polymost_clink(2))) return(0);
	if ((glshsupersampled) && (!polymost_clink(3))) return(0);
	return(f);
}

	//sdl_driver.c: 1 if filter f can draw on the GL 2.1 tier (its shaders link)
int32_t polymost_filter_ready (int32_t f)
{
	return(polymost_colorready(f) == f);
}

	//The program glshprogon says is in use
static GLuint polymost_curprog ()
{
	if (!glshprogon) return(0);
	if (glshcolor) return(glshcprog[glshcolor-1]);
	return((glshprogon == 2) ? glshtprog : glshprog);
}

	//The palookups' colours (glshrgb): curpalette[palookup[pal][shade][index]],
	//   its rows as polymost_uploadlookup's
static void polymost_uploadrgb ()
{
	unsigned char *buf, *b;
	const palette_t *c;
	int32_t p, n, i, rows;

	for(n=MAXPALOOKUPS;(n > 1) && (!palookup[n-1]);n--);
	rows = n*numpalookups;
	buf = (unsigned char *)malloc(rows*256*3);
	if (!buf) return;
	for(p=0,b=buf;p<n;p++)
		for(i=0;i<(numpalookups<<8);i++,b+=3)
		{
			c = &curpalette[(unsigned char)(palookup[p]?palookup[p]:palookup[0])[i]];
			b[0] = c->r; b[1] = c->g; b[2] = c->b;
		}
	glsub_flush();
	bglActiveTexture(GL_TEXTURE1); //(glBindTexture): the wrapper's binding is unit 0's
	if (!glshrgb) glGenTextures(1,&glshrgb);
	(glBindTexture)(GL_TEXTURE_2D,glshrgb);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,256,rows,0,GL_RGB,GL_UNSIGNED_BYTE,buf);
	bglActiveTexture(GL_TEXTURE0);
	free(buf);
	memcpy(glshrgbpal,curpalette,sizeof(glshrgbpal));
	glshrgbdirty = 0;
}

	//The frame changes what it holds (glshcolor to f): from indices to colour
	//   or back it starts black (index 0, or colour); the programs are set again
static void polymost_colorswitch (int32_t f)
{
	int32_t i;

	glsub_flush();
	if ((!f) != (!glshcolor))
	{
		if (!glshbound) bglBindFramebuffer(GL_FRAMEBUFFER,glshfbo);
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT);
		if (!glshbound) bglBindFramebuffer(GL_FRAMEBUFFER,0);
		glshcopied = 0;
	}
	if (glshprogon) { bglUseProgram(0); glshprogon = 0; }
	glshcolor = f;
	for(i=0;i<5;i++) glshcval[i] = -1.f;
	glshmipon = 0;
}

	//palookup[pal][shade][index] as a texture 256 wide, a row per pal and shade (a pal
	//   without palookups takes pal 0's, as drawpoly does). 0: more than a texture holds.
static int32_t polymost_uploadlookup ()
{
	unsigned char *buf;
	int32_t p, n, rows;
	GLint maxsize = 0;

	for(n=MAXPALOOKUPS;(n > 1) && (!palookup[n-1]);n--);
	rows = n*numpalookups;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxsize);
	if (rows > maxsize) return(0);
	buf = (unsigned char *)malloc(rows<<8);
	if (!buf) return(0);
	for(p=0;p<n;p++)
		memcpy(&buf[(p*numpalookups)<<8],palookup[p]?palookup[p]:palookup[0],numpalookups<<8);
	glsub_flush();
	bglActiveTexture(GL_TEXTURE1); //(glBindTexture): the wrapper's binding is unit 0's
	if (!glshlookup) glGenTextures(1,&glshlookup);
	(glBindTexture)(GL_TEXTURE_2D,glshlookup);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE8,256,rows,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,buf);
	bglActiveTexture(GL_TEXTURE0);
	free(buf);
	bglUseProgram(glshtprog);
	bglUniform1f(bglGetUniformLocation(glshtprog,"numshades"),(float)numpalookups);
	bglUniform1f(bglGetUniformLocation(glshtprog,"rowscale"),1.f/(float)rows);
	bglUseProgram(glshprog);
	bglUniform1f(bglGetUniformLocation(glshprog,"numshades"),(float)numpalookups);
	bglUniform1f(bglGetUniformLocation(glshprog,"rowscale"),1.f/(float)rows);
	for(p=0;p<2;p++) //texture filtering's
		if (glshcprog[p])
		{
			bglUseProgram(glshcprog[p]);
			bglUniform1f(bglGetUniformLocation(glshcprog[p],"numshades"),(float)numpalookups);
			bglUniform1f(bglGetUniformLocation(glshcprog[p],"rowscale"),1.f/(float)rows);
		}
	if ((glshprogon != 1) || (glshcolor)) bglUseProgram(polymost_curprog());
	glshlookupdirty = 0;
	glshrgbdirty = 1;
	return(1);
}

	//Makes what the GL 2.1 tier draws with, as needed; a failure falls back to the
	//   vbo path. 1: ready to draw indices.
static int32_t polymost_shaderready ()
{
	GLenum status;
	int32_t f;

	if ((gldrawpath != GLPATH_SHADER) || (!palookup[0]) || (numpalookups <= 0)) return(0);
	if (!glshprog)
	{
		glsub_flush();
		glshprog = polymost_shlink(glshidxvert,(gl_forcefail == 2)?glshbroken:glshidxfrag,"tile","lookup");
		glshtprog = polymost_shlink(glshidxvert,glshtransfrag,"tile","lookup");
		glshresolve = polymost_shlink(glshresvert,glshresfrag,"frame","pal");
		if ((!glshprog) || (!glshtprog) || (!glshresolve)) { polymost_shaderfail("its shaders don't compile or link"); return(0); }
		bglUseProgram(glshtprog);
		bglUniform1i(bglGetUniformLocation(glshtprog,"trans"),2);
		bglUniform1i(bglGetUniformLocation(glshtprog,"dest"),3);
		bglUseProgram(0); glshprogon = 0;
	}
	if ((!glshtrans) && (transluc)) //the translucency table, transluc[(y<<8)+x]
	{
		glsub_flush();
		bglActiveTexture(GL_TEXTURE2);
		glGenTextures(1,&glshtrans);
		(glBindTexture)(GL_TEXTURE_2D,glshtrans);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE8,256,256,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,transluc);
		glPixelStorei(GL_UNPACK_ALIGNMENT,4);
		bglActiveTexture(GL_TEXTURE0);
	}
	if ((!glshfbo) || (glshfbox != xres) || (glshfboy != yres))
	{
		glsub_flush();
		if (glshfbo) { bglDeleteFramebuffers(1,&glshfbo); glshfbo = 0; }
		if (glshdepth) { bglDeleteRenderbuffers(1,&glshdepth); glshdepth = 0; }
		if (glshfbotex) { glDeleteTextures(1,&glshfbotex); glshfbotex = 0; }
		if (glshdst) { glDeleteTextures(1,&glshdst); glshdst = 0; }
		glshcopied = 0;
		glshfbox = xres; glshfboy = yres;
		bglActiveTexture(GL_TEXTURE3); //the copy translucent draws read, the same size
		glGenTextures(1,&glshdst);
		(glBindTexture)(GL_TEXTURE_2D,glshdst);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,xres,yres,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
		bglActiveTexture(GL_TEXTURE0);
		bglUseProgram(glshtprog);
		bglUniform2f(bglGetUniformLocation(glshtprog,"invsize"),1.f/(float)xres,1.f/(float)yres);
		bglUseProgram(0); glshprogon = 0;
		glGenTextures(1,&glshfbotex);
		glBindTexture(GL_TEXTURE_2D,glshfbotex);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,xres,yres,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
		bglGenRenderbuffers(1,&glshdepth);
		bglBindRenderbuffer(GL_RENDERBUFFER,glshdepth);
		bglRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,xres,yres);
		bglBindRenderbuffer(GL_RENDERBUFFER,0);
		bglGenFramebuffers(1,&glshfbo);
		bglBindFramebuffer(GL_FRAMEBUFFER,glshfbo);
		if (gl_forcefail != 3) //the test hook leaves it with nothing attached
		{
			bglFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,glshfbotex,0);
			bglFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,glshdepth);
		}
		status = bglCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status == GL_FRAMEBUFFER_COMPLETE) //a new frame starts as index 0
		{
			glClearColor(0,0,0,0);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		}
		bglBindFramebuffer(GL_FRAMEBUFFER,0);
		glshbound = 0;
		if (status != GL_FRAMEBUFFER_COMPLETE) { polymost_shaderfail("its framebuffer is incomplete"); return(0); }
	}
	f = polymost_colorready(gltexturefilter); //texture filtering: colour
	if ((glshlookupdirty) && (!polymost_uploadlookup()))
		{ polymost_shaderfail("its palookups don't fit a texture"); return(0); }
	if (f != glshcolor) polymost_colorswitch(f);
	if ((glshcolor) && ((glshrgbdirty) || (memcmp(glshrgbpal,curpalette,sizeof(glshrgbpal)))))
		polymost_uploadrgb();
	return(1);
}

	//Points drawing at the framebuffer (GL 2.1 tier), ending the 3D view, with the
	//   index program (prog) or without (fixed function, as lines and text draw).
	//   0: not on that tier, so GL draws RGB as before.
static int32_t polymost_shaderdraw (int32_t prog)
{
	if (!polymost_shaderready()) return(0);
	glscene = 0;
	if (!prog) glshcopied = 0; //lines, text, clears: drawn anywhere
	if (!glshbound)
	{
		glsub_flush();
		bglBindFramebuffer(GL_FRAMEBUFFER,glshfbo);
		glshbound = 1;
	}
	if (prog != glshprogon)
	{
		glsub_flush();
		if (prog) //the palookups on unit 1 (the present binds the palette there)
		{
			bglActiveTexture(GL_TEXTURE1);
			(glBindTexture)(GL_TEXTURE_2D,glshcolor?glshrgb:glshlookup);
			bglActiveTexture(GL_TEXTURE0);
		}
		bglUseProgram(prog?(glshcolor?glshcprog[glshcolor-1]:glshprog):0);
		glshprogon = prog;
	}
	return(1);
}

	//Starts the 3D view drawing indices into the framebuffer, its depth cleared if
	//   clear. Its colour isn't: a hole in the level shows what was there, as in
	//   classic (clearview's colour, or the last frame).
static void polymost_beginframe_shader (int32_t clear)
{
	if ((glscene) || (!polymost_shaderdraw(1))) return;
	if (clear) glClear(GL_DEPTH_BUFFER_BIT);
	glscene = 1;
}

	//A 2D colour, palette index col: on the GL 2.1 tier the index itself (but
	//   for texture filtering's colour)
static void polymost_2dcolor (int32_t col)
{
	if ((glshbound) && (!glshcolor)) glsub_color(((float)col)/255.f,0.f,0.f,1.f);
	else glsub_color(curpalette[col].r/255.f,curpalette[col].g/255.f,curpalette[col].b/255.f,1);
}

	//Between frames (after nextpage's replay): the window's framebuffer is bound
	//   again, as for reading the screen. The next drawing binds the frame's.
void polymost_unbind_shader ()
{
	glffcleared = 0;
	if (!glshbound) return;
	glsub_flush();
	bglUseProgram(0);
	bglBindFramebuffer(GL_FRAMEBUFFER,0);
	glscene = glshbound = glshprogon = 0;
}

	//The box (framebuffer pixels, bottom-up: x0, y0, x1, y1) a polygon drawpoly
	//   draws in covers (px,py: n points of the view, or of the screen for 2D);
	//   behind: some of it is behind the eye, so all the view
static void polymost_shbox (const double *px, const double *py, int32_t n, int32_t behind, int32_t *b)
{
	double x0, y0, x1, y1, ox, oy, f;
	int32_t i;

	if (glscene) { ox = (double)windowx1; oy = (double)(yres-windowy1); }
	else { ox = 0; oy = (double)ydim; }
	if (behind) { x0 = 0; y0 = 0; x1 = (double)xdimen; y1 = (double)ydimen; }
	else
	{
		x0 = x1 = px[0]; y0 = y1 = py[0];
		for(i=1;i<n;i++)
		{
			if (px[i] < x0) x0 = px[i];
			if (px[i] > x1) x1 = px[i];
			if (py[i] < y0) y0 = py[i];
			if (py[i] > y1) y1 = py[i];
		}
	}
	if (grhalfxdown10x < 0) { f = x0; x0 = (double)xdimen-x1; x1 = (double)xdimen-f; } //a mirror's view is flipped
	x0 = ox+x0-1; x1 = ox+x1+1; f = oy-y1-1; y1 = oy-y0+1; y0 = f;
	b[0] = (x0 <= 0) ? 0 : ((x0 >= glshfbox) ? glshfbox : (int32_t)x0);
	b[1] = (y0 <= 0) ? 0 : ((y0 >= glshfboy) ? glshfboy : (int32_t)y0);
	b[2] = (x1 <= 0) ? 0 : ((x1 >= glshfbox) ? glshfbox : (int32_t)ceil(x1));
	b[3] = (y1 <= 0) ? 0 : ((y1 >= glshfboy) ? glshfboy : (int32_t)ceil(y1));
}

static int32_t polymost_shoverlap (const int32_t *a, const int32_t *b)
	{ return((a[0] < b[2]) && (b[0] < a[2]) && (a[1] < b[3]) && (b[1] < a[3])); }

	//Before a translucent draw in box b: copies the framebuffer to glshdst
	//   unless glshdst holds it already there (nothing drawn there since its copy)
static void polymost_transread (const int32_t *b)
{
	int32_t i;

	if (!gl_transcopyall)
	{
		if ((b[0] >= b[2]) || (b[1] >= b[3])) return; //off the frame
		if (glshcopied)
		{
			for(i=glshndirty-1;i>=0;i--) if (polymost_shoverlap(b,glshdirty[i])) break;
			if (i < 0) return;
		}
	}
	glsub_flush(); //what's batched is in the copy
	bglActiveTexture(GL_TEXTURE3);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,glshfbox,glshfboy);
	bglActiveTexture(GL_TEXTURE0);
	glshcopied = 1; glshndirty = 0;
	gl_transcopies++;
}

	//Draws with the index program (1) or the translucent one (2); with colour
	//   (texture filtering) one program draws both, translucent ones blended
static void polymost_shprog (int32_t prog)
{
	if (glshcolor) prog = 1;
	if (glshprogon == prog) return;
	glsub_flush();
	bglUseProgram(glshcolor ? glshcprog[glshcolor-1] : ((prog == 2) ? glshtprog : glshprog));
	glshprogon = prog;
}

	//Drawing colour: the colour program's tile (pic's index texture, clamped
	//   or not, as polymost_idxtex made it): its size, and for trilinear its
	//   mip levels (on unit 5)
static void polymost_shtile (int32_t pic, int32_t clamp)
{
	int32_t *z;
	GLint *loc;

	if (!glshcolor) return;
	clamp = (clamp != 0);
	z = glidxsize[pic][clamp];
	loc = glshcloc[glshcolor-1];
	if (((float)z[0] != glshcval[0]) || ((float)z[1] != glshcval[1]))
	{
		glsub_flush();
		glshcval[0] = (float)z[0]; glshcval[1] = (float)z[1];
		bglUniform2f(loc[0],glshcval[0],glshcval[1]);
	}
	if (glshcolor != 2) return;
	if (((float)z[2] != glshcval[2]) || ((float)z[3] != glshcval[3]) || ((float)clamp != glshcval[4]))
	{
		glsub_flush();
		glshcval[2] = (float)z[2]; glshcval[3] = (float)z[3]; glshcval[4] = (float)clamp;
		bglUniform2f(loc[1],glshcval[2],glshcval[3]);
		bglUniform1f(loc[2],glshcval[4]);
	}
	if (glidxmip[pic][clamp] != glshmipon)
	{
		glsub_flush();
		bglActiveTexture(GL_TEXTURE5);
		(glBindTexture)(GL_TEXTURE_2D,glidxmip[pic][clamp]);
		bglActiveTexture(GL_TEXTURE0);
		glshmipon = glidxmip[pic][clamp];
	}
}

	//After a draw into the framebuffer in box b: glshdst no longer holds it there
static void polymost_drawn (const int32_t *b)
{
	if (!glshcopied) return;
	if (glshndirty >= GLSHDIRTYMAX) { glshcopied = 0; return; }
	memcpy(glshdirty[glshndirty++],b,sizeof(glshdirty[0]));
}

	//Draws the bound texture over the whole window, its rows bottom-up
static void polymost_shaderquad ()
{
	glsub_color(1.f,1.f,1.f,1.f);
	glsub_begin(GL_TRIANGLE_FAN);
	glsub_texcoord(0.f,1.f); glsub_vertex(0.f,0.f,0);
	glsub_texcoord(1.f,1.f); glsub_vertex((float)xres,0.f,0);
	glsub_texcoord(1.f,0.f); glsub_vertex((float)xres,(float)yres,0);
	glsub_texcoord(0.f,0.f); glsub_vertex(0.f,(float)yres,0);
	glsub_end();
	glsub_flush();
}

	//How the colour present fades (texture filtering): toward f's r, g, b
	//   (the fade's colour, brightened as the palette) by f[3], as
	//   getfadedpalette moves each colour of the palette
static void polymost_colorfade (float *f)
{
	int32_t i, c;
	unsigned char *rgb = &palfadergb.r;

	for(i=0;i<3;i++)
	{
		c = britable[curbrightness][(rgb[i]>>2)&63];
		f[i] = (float)((c<<2)|(c>>4))/255.f;
	}
	f[3] = (!palfadedelta) ? 0.f : (((palfadedelta == 255) ? 64.f : (float)(palfadedelta>>2))/64.f);
}

	//The palette the GL 2.1 tier shows its indices through: the one on screen
	//   (getfadedpalette), with the contrast and saturation set
static void polymost_shownpalette (palette_t *pal)
{
	int32_t i;

	getfadedpalette(pal);
	for(i=0;i<256;i++) gl_colorcorrect(&pal[i].r,glcontrast,glsaturation);
}

	//Shows the frame on the GL 2.1 tier: draws the framebuffer's indices into the
	//   window (dw by dh) through polymost_shownpalette (a frame larger than r,
	//   supersampled, box-filtered down: glshdownfrag), darkened by dark/256
	//   (the VIDEO page's fade), for the
	//   swap, at r (x, y, width, height: gl_fitrect's), black around it. The
	//   framebuffer keeps them. 0: not on that tier (or nothing drawn yet).
int32_t polymost_present_shader (const int32_t *r, int32_t dw, int32_t dh, int32_t dark)
{
	palette_t pal[256];
	unsigned char rgb[768];
	int32_t i;

	if ((gldrawpath != GLPATH_SHADER) || (!glshfbo)) return(0);
	setpolymost2dview(); //the 2D view and its state, no 3D view or program
	if (!glshbound) return(0); //the tier failed
	glsub_flush();
	bglBindFramebuffer(GL_FRAMEBUFFER,0);
	glshbound = 0;
	if (!glshcolor) //(colour is faded by its program)
	{
	polymost_shownpalette(pal);
	if (dark)
		for(i=0;i<256;i++)
		{
			pal[i].r = (unsigned char)((pal[i].r*(256-dark))>>8);
			pal[i].g = (unsigned char)((pal[i].g*(256-dark))>>8);
			pal[i].b = (unsigned char)((pal[i].b*(256-dark))>>8);
		}
	bglActiveTexture(GL_TEXTURE1);
	if ((!glshpal) || (memcmp(glshpalnow,pal,sizeof(glshpalnow))))
	{
		memcpy(glshpalnow,pal,sizeof(glshpalnow));
		for(i=0;i<256;i++) { rgb[i*3] = pal[i].r; rgb[i*3+1] = pal[i].g; rgb[i*3+2] = pal[i].b; }
		if (!glshpal) glGenTextures(1,&glshpal);
		(glBindTexture)(GL_TEXTURE_2D,glshpal);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,256,1,0,GL_RGB,GL_UNSIGNED_BYTE,rgb);
	}
	else (glBindTexture)(GL_TEXTURE_2D,glshpal);
	bglActiveTexture(GL_TEXTURE0);
	}
	if ((r[0]) || (r[1]) || (r[2] != dw) || (r[3] != dh)) //a smaller frame, scaled up: the bars
	{
		glViewport(0,0,dw,dh);
		glClearColor(0,0,0,0);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	glViewport(r[0],r[1],r[2],r[3]);
	if (glshcolor) //colour (texture filtering): faded, corrected and darkened by its program
	{
		float f[4];
		GLuint p = ((glshsupersampled) && (glshcdown) && (polymost_supersample_ready())) ? glshcdown : glshcres;

		bglUseProgram(p);
		if (p == glshcdown) //scaled down
		{
			bglUniform2f(bglGetUniformLocation(p,"scale"),(float)xres/(float)r[2],(float)yres/(float)r[3]);
			bglUniform2f(bglGetUniformLocation(p,"origin"),(float)r[0],(float)r[1]);
			bglUniform2f(bglGetUniformLocation(p,"invsize"),1.f/(float)xres,1.f/(float)yres);
		}
		polymost_colorfade(f);
		bglUniform4f(bglGetUniformLocation(p,"fade"),f[0],f[1],f[2],f[3]);
		bglUniform2f(bglGetUniformLocation(p,"cs"),(float)glcontrast,(float)glsaturation);
		bglUniform1f(bglGetUniformLocation(p,"bright"),(float)(256-dark)/256.f);
	}
	else if ((glshsupersampled) && (polymost_supersample_ready())) //scaled down
	{
		bglUseProgram(glshdown);
		bglUniform2f(bglGetUniformLocation(glshdown,"scale"),(float)xres/(float)r[2],(float)yres/(float)r[3]);
		bglUniform2f(bglGetUniformLocation(glshdown,"origin"),(float)r[0],(float)r[1]);
		bglUniform2f(bglGetUniformLocation(glshdown,"invsize"),1.f/(float)xres,1.f/(float)yres);
	}
	else bglUseProgram(glshresolve);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,glshfbotex);
	polymost_shaderquad();
	bglUseProgram(0);
	glDisable(GL_TEXTURE_2D);
	glViewport(0,0,xres,yres); //setpolymost2dview's, which it keeps
	return(1);
}

	//The frame as polymost_present_shader shows it: RGB, top row first, xres by
	//   yres, in a malloc'd buffer. NULL: not on the GL 2.1 tier (or no memory).
unsigned char *polymost_capture_shader ()
{
	palette_t pal[256], *c;
	unsigned char *idx, *rgb, *p;
	int32_t x, y;

	if ((gldrawpath != GLPATH_SHADER) || (!glshfbo)) return(NULL);
	idx = (unsigned char *)malloc(xres*yres*(glshcolor?3:1)); //(colour's RGB)
	rgb = (unsigned char *)malloc(xres*yres*3);
	if ((!idx) || (!rgb)) { free(idx); free(rgb); return(NULL); }
	glsub_flush();
	if (!glshbound) bglBindFramebuffer(GL_FRAMEBUFFER,glshfbo);
	glPixelStorei(GL_PACK_ALIGNMENT,1);
	glReadPixels(0,0,xres,yres,glshcolor?GL_RGB:GL_RED,GL_UNSIGNED_BYTE,idx);
	glPixelStorei(GL_PACK_ALIGNMENT,4);
	if (!glshbound) bglBindFramebuffer(GL_FRAMEBUFFER,0);
	if (glshcolor) //colour (texture filtering): faded and corrected as glshcresfrag does
	{
		float f[4], v[3], l;
		int32_t i;

		polymost_colorfade(f);
		for(y=0,p=rgb;y<yres;y++)
			for(x=0;x<xres;x++,p+=3)
			{
				memcpy(p,&idx[((yres-1-y)*xres+x)*3],3);
				if ((f[3] == 0.f) && (glcontrast == 1.0) && (glsaturation == 1.0)) continue;
				for(i=0;i<3;i++) v[i] = (float)p[i]/255.f+(f[i]-(float)p[i]/255.f)*f[3];
				l = 0.2126f*v[0]+0.7152f*v[1]+0.0722f*v[2];
				for(i=0;i<3;i++)
				{
					v[i] = (l+(float)glsaturation*(v[i]-l)-0.5f)*(float)glcontrast+0.5f;
					p[i] = (unsigned char)(min(max(v[i],0.f),1.f)*255.f+0.5f);
				}
			}
		free(idx);
		return(rgb);
	}
	polymost_shownpalette(pal);
	for(y=0,p=rgb;y<yres;y++) //the framebuffer's rows are bottom-up
		for(x=0;x<xres;x++,p+=3)
		{
			c = &pal[idx[(yres-1-y)*xres+x]];
			p[0] = c->r; p[1] = c->g; p[2] = c->b;
		}
	free(idx);
	return(rgb);
}

	//After a mode set: makes what the GL 2.1 tier draws with now, so a failure
	//   falls back to the vbo path before anything is drawn
void polymost_prepare_shader ()
{
	polymost_shaderready();
}

	//A window resize, before polymost_glreset: takes the frame's texture from the
	//   GL 2.1 tier for polymost_putframe_shader after. 0: none.
GLuint polymost_takeframe_shader ()
{
	GLuint tex;

	if ((gldrawpath != GLPATH_SHADER) || (!glshfbotex)) return(0);
	glsub_flush();
	if (glshbound) { bglBindFramebuffer(GL_FRAMEBUFFER,0); glshbound = 0; }
	glscene = 0;
	tex = glshfbotex; glshfbotex = 0;
	return(tex);
}

	//...and draws it into the new framebuffer, stretched to its size, then drops it
void polymost_putframe_shader (GLuint tex)
{
	setpolymost2dview();
	if (glshbound)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,tex);
		polymost_shaderquad();
		glDisable(GL_TEXTURE_2D);
	}
	glDeleteTextures(1,&tex);
}

	//Of four indices c (255: see-through), the one nearest their mean colour
	//   (in the base palette), or 255 if three or four are
static int32_t polymost_idxpick (const int32_t *c)
{
	int32_t i, k, n = 0, m[3] = {0,0,0}, d, e, best = 255, bestd = 0x7fffffff;

	for(i=0;i<4;i++)
		if (c[i] != 255) { n++; for(k=0;k<3;k++) m[k] += palette[c[i]*3+k]; }
	if (n < 2) return(255);
	for(i=0;i<4;i++)
	{
		if (c[i] == 255) continue;
		for(d=0,k=0;k<3;k++) { e = palette[c[i]*3+k]*n-m[k]; d += e*e; }
		if (d < bestd) { bestd = d; best = c[i]; }
	}
	return(best);
}

	//Trilinear's mip levels of a tile's indices (pic, xsiz by ysiz): levels 1
	//   and on side by side in one texture, each after the last, each half the
	//   last's size (at least 1) as GL's are. A texel's four indices (r, g, b,
	//   a) are one from each of the four texels of the level above it covers:
	//   that texel's index at level 1, deeper polymost_idxpick of its four.
	//   sz gets the texture's size (1x1 when the tile has no levels: none made).
static void polymost_idxmip (GLuint *tex, const unsigned char *pic, int32_t xsiz, int32_t ysiz, int32_t *sz)
{
	unsigned char *buf, *rep;
	int32_t w, h, nw, nh, aw, ah, x, y, i, off, c[4];

	sz[0] = sz[1] = 1;
	ah = max(ysiz>>1,1);
	for(aw=0,w=xsiz,h=ysiz;(w > 1) || (h > 1);) { w = max(w>>1,1); h = max(h>>1,1); aw += w; }
	if (!aw) return;
	buf = (unsigned char *)calloc(aw*ah,4);
	rep = (unsigned char *)malloc(xsiz*ysiz);
	if ((!buf) || (!rep)) { free(buf); free(rep); return; }
	memcpy(rep,pic,xsiz*ysiz);
	for(off=0,w=xsiz,h=ysiz;(w > 1) || (h > 1);off+=nw,w=nw,h=nh)
	{
		nw = max(w>>1,1); nh = max(h>>1,1);
		for(y=0;y<nh;y++)
			for(x=0;x<nw;x++)
			{
				c[0] = rep[(y*2)*w+x*2];
				c[1] = rep[(y*2)*w+min(x*2+1,w-1)];
				c[2] = rep[min(y*2+1,h-1)*w+x*2];
				c[3] = rep[min(y*2+1,h-1)*w+min(x*2+1,w-1)];
				for(i=0;i<4;i++) buf[(y*aw+off+x)*4+i] = (unsigned char)c[i];
			}
		for(y=0;y<nh;y++) //the level's own indices, for the next
			for(x=0;x<nw;x++)
			{
				for(i=0;i<4;i++) c[i] = buf[(y*aw+off+x)*4+i];
				rep[y*nw+x] = (unsigned char)polymost_idxpick(c);
			}
	}
	glGenTextures(1,tex);
	glBindTexture(GL_TEXTURE_2D,*tex);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,aw,ah,0,GL_RGBA,GL_UNSIGNED_BYTE,buf);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	free(buf);
	free(rep);
	sz[0] = aw; sz[1] = ah;
}

	//A tile's index texture (GL 2.1 tier): its palette indices as they are, nearest,
	//   no mipmaps; 255 (see-through) where a clamped one is padded. A tile without
	//   art gets a 1x1 index 0, as drawpoly writes only its depth (the mirror hack).
static GLuint polymost_idxtex (int32_t dapic, int32_t clamp)
{
	GLuint *tex;
	unsigned char *pic;
	int32_t x, y, x2, y2, xsiz, ysiz, tsizx, tsizy, mip;

	clamp = (clamp != 0);
	tex = &glidxtex[dapic][clamp];
	mip = (glshcolor == 2); //trilinear: its mip levels too
	if ((*tex) && (!glidxdirty[dapic][clamp]) && ((!mip) || (glidxsize[dapic][clamp][2]))) return(*tex);
	glidxdirty[dapic][clamp] = 0;

	tsizx = tilesizx[dapic]; tsizy = tilesizy[dapic];
	if (!waloff[dapic]) tsizx = tsizy = 1;
	if (!glinfo.texnpot) {
		for(xsiz=1;xsiz<tsizx;xsiz+=xsiz);
		for(ysiz=1;ysiz<tsizy;ysiz+=ysiz);
	} else {
		xsiz = max(1,tsizx);
		ysiz = max(1,tsizy);
	}
	pic = (unsigned char *)malloc(xsiz*ysiz);
	if (!pic) return(*tex);
	if (!waloff[dapic]) pic[0] = 0;
	else
	{
		for(y=0;y<ysiz;y++)
		{
			if (y < tsizy) y2 = y; else y2 = y-tsizy;
			for(x=0;x<xsiz;x++)
			{
				if ((clamp) && ((x >= tsizx) || (y >= tsizy))) { pic[y*xsiz+x] = 255; continue; }
				if (x < tsizx) x2 = x; else x2 = x-tsizx;
				pic[y*xsiz+x] = *(unsigned char *)(waloff[dapic]+x2*tsizy+y2);
			}
		}
	}
	if (!*tex) glGenTextures(1,tex);
	glBindTexture(GL_TEXTURE_2D,*tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE8,xsiz,ysiz,0,GL_LUMINANCE,GL_UNSIGNED_BYTE,pic);
	glPixelStorei(GL_UNPACK_ALIGNMENT,4);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,clamp?GL_CLAMP_TO_EDGE:GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,clamp?GL_CLAMP_TO_EDGE:GL_REPEAT);
	glidxsize[dapic][clamp][0] = xsiz; glidxsize[dapic][clamp][1] = ysiz;
	if (glidxmip[dapic][clamp]) //made from the art before
	{
		if (glshmipon == glidxmip[dapic][clamp]) glshmipon = 0;
		glDeleteTextures(1,&glidxmip[dapic][clamp]); glidxmip[dapic][clamp] = 0;
	}
	glidxsize[dapic][clamp][2] = glidxsize[dapic][clamp][3] = 0;
	if (mip) polymost_idxmip(&glidxmip[dapic][clamp],pic,xsiz,ysiz,&glidxsize[dapic][clamp][2]);
	free(pic);
	return(*tex);
}

	//Classic's parallaxed sky is a sector's shade plus a distance shade that's the
	//   same everywhere (parascan's swplc, globalpisibility)
static float polymost_skyshade (int32_t vis)
{
	int32_t v = globalpisibility;

	if (vis) v = mulscale4(v,(int32_t)((unsigned char)(vis+16)));
	return((float)(mulscale16(mulscale16(xdimscale,viewingrange),v)>>8));
}

	//Use this for both initialization and uninitialization of OpenGL.
void polymost_glreset ()
{
	int32_t i, j;

	if (gltexcacnum) //Reset if this is -1 (meaning 1st texture call ever), or > 0 (textures in memory)
	{
		memset(gltexcachead,-1,sizeof(gltexcachead));
		if (gltexcacnum < 0) gltexcacnum = 0;
		else
		{
			do {
				gltexcacnum--; glDeleteTextures(1,&glpic[gltexcacnum]);
				if (glpicfb[gltexcacnum]) { glDeleteTextures(1,&glpicfb[gltexcacnum]); glpicfb[gltexcacnum] = 0; }
			} while (gltexcacnum > 0);
		}
	}
	for(i=0;i<MAXTILES;i++)
		for(j=0;j<2;j++)
		{
			if (glidxtex[i][j]) { glDeleteTextures(1,&glidxtex[i][j]); glidxtex[i][j] = 0; }
			if (glidxmip[i][j]) { glDeleteTextures(1,&glidxmip[i][j]); glidxmip[i][j] = 0; }
			glidxsize[i][j][2] = glidxsize[i][j][3] = 0;
		}
	polymost_shaderfree();
	gltexmaxsize = 0; //a new context may have another GL_MAX_TEXTURE_SIZE
	glox1 = -1;
	gloy1 = 0; //not -1, so setpolymost2dview sets its view again (the drawable may have resized)
}

void resizeglcheck ()
{
	float m[4][4];

	if (glredbluemode < lastglredbluemode) {
		glox1 = -1;
		glColorMask(1,1,1,1);
	} else if (glredbluemode != lastglredbluemode) {
		redblueclearcnt = 0;
	}
	lastglredbluemode = glredbluemode;

	if ((glox1 != windowx1) || (gloy1 != windowy1) || (glox2 != windowx2) || (gloy2 != windowy2))
	{
		float col[4];

		glox1 = windowx1; gloy1 = windowy1;
		glox2 = windowx2; gloy2 = windowy2;

		glViewport(windowx1,yres-(windowy2+1),windowx2-windowx1+1,windowy2-windowy1+1);

		glMatrixMode(GL_PROJECTION);
		memset(m,0,sizeof(m));
		m[0][0] = (float)ydimen; m[0][2] = 1.0;
		m[1][1] = (float)xdimen; m[1][2] = 1.0;
		m[2][2] = 1.0; m[2][3] = (float)ydimen;
		m[3][2] =-1.0;
		glLoadMatrixf(&m[0][0]);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		if (!nofog) {
		glEnable(GL_FOG);
		fog_is_on = 1;
		glFogi(GL_FOG_MODE,GL_LINEAR); //polymost_setfog sets start and end
		glFogf(GL_FOG_START,0.0); glfogstart = 0.f;
		glFogf(GL_FOG_END,1.0); glfogend = 1.f;
		col[0] = 0; col[1] = 0; col[2] = 0; col[3] = 0; //range:0 to 1
		glFogfv(GL_FOG_COLOR,col); //default is 0,0,0,0
		}

		glEnable(GL_TEXTURE_2D);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}
}

void fixtransparency (coltype *dapic, int32_t daxsiz, int32_t daysiz, int32_t daxsiz2, int32_t daysiz2, int32_t dameth)
{
	coltype *wpptr;
	int32_t j, x, y, r, g, b, dox, doy, naxsiz2;

	dox = daxsiz2-1; doy = daysiz2-1;
	if (dameth&4) { dox = min(dox,daxsiz); doy = min(doy,daysiz); }
				else { daxsiz = daxsiz2; daysiz = daysiz2; } //Make repeating textures duplicate top/left parts

	daxsiz--; daysiz--; naxsiz2 = -daxsiz2; //Hacks for optimization inside loop

		//Set transparent pixels to average color of neighboring opaque pixels
		//Doing this makes bilinear filtering look much better for masked textures (I.E. sprites)
	for(y=doy;y>=0;y--)
	{
		wpptr = &dapic[y*daxsiz2+dox];
		for(x=dox;x>=0;x--,wpptr--)
		{
			if (wpptr->a) continue;
			r = g = b = j = 0;
			if ((x>     0) && (wpptr[     -1].a)) { r += (int32_t)wpptr[     -1].r; g += (int32_t)wpptr[     -1].g; b += (int32_t)wpptr[     -1].b; j++; }
			if ((x<daxsiz) && (wpptr[     +1].a)) { r += (int32_t)wpptr[     +1].r; g += (int32_t)wpptr[     +1].g; b += (int32_t)wpptr[     +1].b; j++; }
			if ((y>     0) && (wpptr[naxsiz2].a)) { r += (int32_t)wpptr[naxsiz2].r; g += (int32_t)wpptr[naxsiz2].g; b += (int32_t)wpptr[naxsiz2].b; j++; }
			if ((y<daysiz) && (wpptr[daxsiz2].a)) { r += (int32_t)wpptr[daxsiz2].r; g += (int32_t)wpptr[daxsiz2].g; b += (int32_t)wpptr[daxsiz2].b; j++; }
			switch(j)
			{
				case 1: wpptr->r =   r            ; wpptr->g =   g            ; wpptr->b =   b            ; break;
				case 2: wpptr->r = ((r   +  1)>>1); wpptr->g = ((g   +  1)>>1); wpptr->b = ((b   +  1)>>1); break;
				case 3: wpptr->r = ((r*85+128)>>8); wpptr->g = ((g*85+128)>>8); wpptr->b = ((b*85+128)>>8); break;
				case 4: wpptr->r = ((r   +  2)>>2); wpptr->g = ((g   +  2)>>2); wpptr->b = ((b   +  2)>>2); break;
				default: break;
			}
		}
	}
}

static void uploadtexture(int32_t doalloc, int32_t xsiz, int32_t ysiz, int32_t intexfmt, int32_t texfmt, coltype *pic, int32_t tsizx, int32_t tsizy, int32_t dameth)
{
	coltype *wpptr, *rpptr;
	int32_t x2, y2, j, js=0, x3, y3, y, x, r, g, b, a, k;

	if (gltexmaxsize <= 0) {
		GLint i = 0;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &i);
		if (!i) gltexmaxsize = 6;   // 2^6 = 64 == default GL max texture size
		else {
			gltexmaxsize = 0;
			for (; i>1; i>>=1) gltexmaxsize++;
		}
	}

	js = max(0,min(gltexmaxsize-1,gltexmiplevel));
	gltexmiplevel = js;
	while ((xsiz>>js) > (1<<gltexmaxsize) || (ysiz>>js) > (1<<gltexmaxsize)) js++;
		//Keep a level to upload: a texture smaller than 2^js has no level js
	while ((js > 0) && !(xsiz>>js) && !(ysiz>>js)) js--;

	if (js == 0) {
		if (doalloc)
			glTexImage2D(GL_TEXTURE_2D,0,intexfmt,xsiz,ysiz,0,texfmt,GL_UNSIGNED_BYTE,pic); //loading 1st time
		else
			glTexSubImage2D(GL_TEXTURE_2D,0,0,0,xsiz,ysiz,texfmt,GL_UNSIGNED_BYTE,pic); //overwrite old texture
	}

#if 0
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGBA8,xsiz,ysiz,texfmt,GL_UNSIGNED_BYTE,pic); //Needs C++ to link?
#elif 1
	x2 = xsiz; y2 = ysiz;
	for(j=1;(x2 > 1) || (y2 > 1);j++)
	{
		//x3 = ((x2+1)>>1); y3 = ((y2+1)>>1);
		x3 = max(1, x2 >> 1); y3 = max(1, y2 >> 1);		// this came from the GL_ARB_texture_non_power_of_two spec
		for(y=0;y<y3;y++)
		{
			wpptr = &pic[y*x3]; rpptr = &pic[(y<<1)*x2];
			for(x=0;x<x3;x++,wpptr++,rpptr+=2)
			{
				r = g = b = a = k = 0;
				if  (rpptr[0].a)                  { r += (int32_t)rpptr[0].r; g += (int32_t)rpptr[0].g; b += (int32_t)rpptr[0].b; a += (int32_t)rpptr[0].a; k++; }
				if ((x+x+1 < x2) && (rpptr[1].a)) { r += (int32_t)rpptr[1].r; g += (int32_t)rpptr[1].g; b += (int32_t)rpptr[1].b; a += (int32_t)rpptr[1].a; k++; }
				if (y+y+1 < y2)
				{
					if ((rpptr[x2].a)                  ) { r += (int32_t)rpptr[x2  ].r; g += (int32_t)rpptr[x2  ].g; b += (int32_t)rpptr[x2  ].b; a += (int32_t)rpptr[x2  ].a; k++; }
					if ((x+x+1 < x2) && (rpptr[x2+1].a)) { r += (int32_t)rpptr[x2+1].r; g += (int32_t)rpptr[x2+1].g; b += (int32_t)rpptr[x2+1].b; a += (int32_t)rpptr[x2+1].a; k++; }
				}
				switch(k)
				{
					case 0:
					case 1: wpptr->r = r; wpptr->g = g; wpptr->b = b; wpptr->a = a; break;
					case 2: wpptr->r = ((r+1)>>1); wpptr->g = ((g+1)>>1); wpptr->b = ((b+1)>>1); wpptr->a = ((a+1)>>1); break;
					case 3: wpptr->r = ((r*85+128)>>8); wpptr->g = ((g*85+128)>>8); wpptr->b = ((b*85+128)>>8); wpptr->a = ((a*85+128)>>8); break;
					case 4: wpptr->r = ((r+2)>>2); wpptr->g = ((g+2)>>2); wpptr->b = ((b+2)>>2); wpptr->a = ((a+2)>>2); break;
					default: break;
				}
				//if (wpptr->a) wpptr->a = 255;
			}
		}
		if (tsizx >= 0) fixtransparency(pic,(tsizx+(1<<j)-1)>>j,(tsizy+(1<<j)-1)>>j,x3,y3,dameth);
		if (j >= js) {
			if (doalloc)
				glTexImage2D(GL_TEXTURE_2D,j-js,intexfmt,x3,y3,0,texfmt,GL_UNSIGNED_BYTE,pic); //loading 1st time
			else
				glTexSubImage2D(GL_TEXTURE_2D,j-js,0,0,x3,y3,texfmt,GL_UNSIGNED_BYTE,pic); //overwrite old texture
		}
		x2 = x3; y2 = y3;
	}
#endif
}

void gloadtile (int32_t dapic, int32_t dapal, int32_t dameth, int32_t gltexind, int32_t doalloc)
{
	coltype *pic, *wpptr, *fbpic;
	int32_t j, x, y, x2, y2, xsiz, ysiz, dacol, tsizx, tsizy, hasfullbright = 0;
	int32_t texfmt = GL_RGBA, intexfmt = GL_RGBA8;
	int32_t hist[256];

	memset(hist,0,sizeof(hist));
	gl_rgbtexloads++;

	if (doalloc) glGenTextures(1,&glpic[gltexind]);  //# of textures (make OpenGL allocate structure)
	glBindTexture(GL_TEXTURE_2D,glpic[gltexind]);

	tsizx = tilesizx[dapic];
	tsizy = tilesizy[dapic];
	if (!glinfo.texnpot) {
		for(xsiz=1;xsiz<tsizx;xsiz+=xsiz);
		for(ysiz=1;ysiz<tsizy;ysiz+=ysiz);
	} else {
		xsiz = max(1,tsizx);
		ysiz = max(1,tsizy);
	}

	pic = (coltype *)malloc(xsiz*ysiz*sizeof(coltype));
	if (!pic) return;
	fbpic = (coltype *)calloc(xsiz*ysiz,sizeof(coltype)); //fullbright texels, transparent elsewhere; NULL: no glow layer

	if (!waloff[dapic])
	{
			//Force invalid textures to draw something - an almost purely transparency texture
			//This allows the Z-buffer to be updated for mirrors (which are invalidated textures)
		pic[0].r = pic[0].g = pic[0].b = 0; pic[0].a = 1;
		tsizx = tsizy = 1;
	}
	else
	{
		for(y=0;y<ysiz;y++)
		{
			if (y < tsizy) y2 = y; else y2 = y-tsizy;
			wpptr = &pic[y*xsiz];
			for(x=0;x<xsiz;x++,wpptr++)
			{
				if ((dameth&4) && ((x >= tsizx) || (y >= tsizy))) //Clamp texture
					{ wpptr->r = wpptr->g = wpptr->b = wpptr->a = 0; continue; }
				if (x < tsizx) x2 = x; else x2 = x-tsizx;
				dacol = (int32_t)(*(unsigned char *)(waloff[dapic]+x2*tsizy+y2));
				if ((x < tsizx) && (y < tsizy)) hist[dacol]++;
				if (dacol == 255)
				{
					wpptr->r = curpalette[255].r;
					wpptr->g = curpalette[255].g;
					wpptr->b = curpalette[255].b;
					wpptr->a = 0;
				}
				else
				{
					j = (int32_t)((unsigned char)palookup[dapal][dacol]);
					wpptr->r = curpalette[j].r;
					wpptr->g = curpalette[j].g;
					wpptr->b = curpalette[j].b;
					wpptr->a = 255;
					if ((j >= polymosttexfullbright) && (j < 255) && (fbpic))
						{ fbpic[wpptr-pic] = *wpptr; hasfullbright = 1; }
				}
			}
		}
	}

	if ((waloff[dapic]) && (numpalookups <= TILERAMPN))
		gltilezero[dapic] = polymost_buildramp(hist,gltileramp[dapic],numpalookups);
	fixtransparency(pic,tsizx,tsizy,xsiz,ysiz,dameth);

	
	uploadtexture(doalloc,xsiz,ysiz,intexfmt,texfmt,pic,tsizx,tsizy,dameth);

	if (gltexfiltermode < 0) gltexfiltermode = 0;
	else if (gltexfiltermode >= numglfiltermodes) gltexfiltermode = numglfiltermodes-1;
 //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,glfiltermodes[gltexfiltermode].mag);

 //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
 //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
 //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR); //Trilinear filter
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,glfiltermodes[gltexfiltermode].min);

	if (glinfo.maxanisotropy > 1.0)
	{
		if (glanisotropy <= 0 || glanisotropy > glinfo.maxanisotropy) glanisotropy = glinfo.maxanisotropy;
		glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,glanisotropy);
	}

	if (!(dameth&4))
	{
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	}
	else
	{     //For sprite textures, clamping looks better than wrapping
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP);
	}

	if (pic) free(pic);

	if (hasfullbright)
	{
		x = !glpicfb[gltexind];
		if (x) glGenTextures(1,&glpicfb[gltexind]);
		glBindTexture(GL_TEXTURE_2D,glpicfb[gltexind]);
		fixtransparency(fbpic,tsizx,tsizy,xsiz,ysiz,dameth);
		uploadtexture(x,xsiz,ysiz,intexfmt,texfmt,fbpic,tsizx,tsizy,dameth);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,glfiltermodes[gltexfiltermode].mag);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,glfiltermodes[gltexfiltermode].min);
		if (glinfo.maxanisotropy > 1.0)
			glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAX_ANISOTROPY_EXT,glanisotropy);
		x = (dameth&4) ? (glinfo.clamptoedge?GL_CLAMP_TO_EDGE:GL_CLAMP) : GL_REPEAT;
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,x);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,x);
		glBindTexture(GL_TEXTURE_2D,glpic[gltexind]);
	}
	else if (glpicfb[gltexind])
		{ glDeleteTextures(1,&glpicfb[gltexind]); glpicfb[gltexind] = 0; } //a reload found none
	free(fbpic);
}
#endif

	//(dpx,dpy) specifies an n-sided polygon. The polygon must be a convex clockwise loop.
	//    n must be <= 8 (assume clipping can double number of vertices)
	//method: 0:solid, 1:masked(255 is transparent), 2:transluscent #1, 3:transluscent #2
	//    +4 means it's a sprite, so wraparound isn't needed
	//    +8 (METH_LAYERS) draws the tile's fullbright texels again over it, unfogged (OpenGL only)
#define METH_LAYERS 8
static int32_t pow2xsplit = 0, skyclamphack = 0;

#ifdef USE_OPENGL
	//Switches to the glow pass: full colour, no fog, blended, no Z writes
static void polymost_glowbegin (GLuint fbtex, float alpha)
{
	glsub_color(1.f,1.f,1.f,alpha);
	if (fog_is_on) glDisable(GL_FOG);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glBindTexture(GL_TEXTURE_2D,fbtex);
}

	//Restores what the base pass set
static void polymost_glowend (GLuint basetex, float *pc, int32_t method)
{
	if (fog_is_on) glEnable(GL_FOG);
	glDepthMask(GL_TRUE);
	if (!(method&3)) glDisable(GL_BLEND);
	glsub_color(pc[0],pc[1],pc[2],pc[3]);
	glBindTexture(GL_TEXTURE_2D,basetex);
}
#endif

void drawpoly (double *dpx, double *dpy, int32_t n, int32_t method)
{
	#define PI 3.14159265358979323
	double ngdx, ngdy, ngdo, ngux, nguy, nguo, ngvx, ngvy, ngvo, dp, up, vp, rdp, du0, du1, dui, duj;
	double ngdx2, ngux2, ngvx2;
	double f, r, ox, oy, oz, ox2, oy2, oz2, dd[16], uu[16], vv[16], px[16], py[16], uoffs;
	int32_t i, j, k, x, y, z, nn, ix0, ix1, mini, maxi, tsizx, tsizy, tsizxm1, tsizym1, ltsizy;
	int32_t xx, yy, xi, d0, u0, v0, d1, u1, v1, xmodnice, ymulnice, dorot;
	char dacol, *walptr, *palptr, *vidp, *vide;
	int32_t layers = method&METH_LAYERS, behind = 0;

	method &= ~METH_LAYERS;
	if (n == 3)
	{
		if ((dpx[0]-dpx[1])*(dpy[2]-dpy[1]) >= (dpx[2]-dpx[1])*(dpy[0]-dpy[1])) return; //for triangle
	}
	else
	{
		f = 0; //f is area of polygon / 2
		for(i=n-2,j=n-1,k=0;k<n;i=j,j=k,k++) f += (dpx[i]-dpx[k])*dpy[j];
		if (f <= 0) return;
	}

		//Load texture (globalpicnum)
	if ((uint32_t)globalpicnum >= MAXTILES) globalpicnum = 0;
	setgotpic(globalpicnum);
	tsizx = tilesizx[globalpicnum];
	tsizy = tilesizy[globalpicnum];
	if (!palookup[globalpal]) globalpal = 0;
	if (!waloff[globalpicnum])
	{
		loadtile(globalpicnum);
		if (!waloff[globalpicnum])
		{
			if (rendmode != 3) return;
			tsizx = tsizy = 1; method = 1; //Hack to update Z-buffer for invalid mirror textures
		}
	}
	walptr = (char *)waloff[globalpicnum];

	j = 0; dorot = ((gchang != 1.0) || (gctang != 1.0));
	if (dorot)
	{
		for(i=0;i<n;i++)
		{
			ox = dpx[i]-ghalfx;
			oy = dpy[i]-ghoriz;
			oz = ghalfx;

				//Up/down rotation
			ox2 = ox;
			oy2 = oy*gchang - oz*gshang;
			oz2 = oy*gshang + oz*gchang;

				//Tilt rotation
			ox = ox2*gctang - oy2*gstang;
			oy = ox2*gstang + oy2*gctang;
			oz = oz2;

			if (oz < SCISDIST) { if (rendmode != 3) return; behind = 1; } //annoying hack to avoid bugs in software rendering

			r = ghalfx / oz;

			dd[j] = (dpx[i]*gdx + dpy[i]*gdy + gdo)*r;
			uu[j] = (dpx[i]*gux + dpy[i]*guy + guo)*r;
			vv[j] = (dpx[i]*gvx + dpy[i]*gvy + gvo)*r;

			px[j] = ox*r + ghalfx;
			py[j] = oy*r + ghoriz;
			if ((!j) || (px[j] != px[j-1]) || (py[j] != py[j-1])) j++;
		}
	}
	else
	{
		for(i=0;i<n;i++)
		{
			px[j] = dpx[i];
			py[j] = dpy[i];
			if ((!j) || (px[j] != px[j-1]) || (py[j] != py[j-1])) j++;
		}
	}
	while ((j >= 3) && (px[j-1] == px[0]) && (py[j-1] == py[0])) j--;
	if (j < 3) return;
	n = j;

#ifdef USE_OPENGL
	if (rendmode == 3)
	{
		float hackscx, hackscy, pc[4], zero;
		GLuint basetex, fbtex;
		int32_t shb[4];

		if (skyclamphack) method |= 4;
		if (glshbound) //indices: no glow pass, as classic's shade tables keep lit texels lit
			{ basetex = polymost_idxtex(globalpicnum,method&4); fbtex = 0; }
		else
		{
			k = gltexcache(globalpicnum,globalpal,method&4);
			basetex = glpic[k]; fbtex = layers ? glpicfb[k] : 0;
		}
		glBindTexture(GL_TEXTURE_2D,basetex);

		hackscx = 1.0; hackscy = 1.0;

		if (!glinfo.texnpot) {
			for(xx=1;xx<tsizx;xx+=xx);
			for(yy=1;yy<tsizy;yy+=yy);
			ox2 = (double)1.0/(double)xx;
			oy2 = (double)1.0/(double)yy;
		} else {
			xx = max(1,tsizx); ox2 = (double)1.0/(double)xx;
			yy = max(1,tsizy); oy2 = (double)1.0/(double)yy;
		}

			//Indices don't blend: translucent ones read the index under them
			//   (colour, for texture filtering, blends them)
		if ((glshbound) && (glshcolor)) { if ((method&3) >= 2) glEnable(GL_BLEND); else glDisable(GL_BLEND); }
		else if ((glshbound) || (!(method&3))) glDisable(GL_BLEND); else glEnable(GL_BLEND);

		//glEnable(GL_ALPHA_TEST);
		//glAlphaFunc(GL_GREATER,0.5);

		if (!dorot)
		{
			for(i=n-1;i>=0;i--)
			{
				dd[i] = px[i]*gdx + py[i]*gdy + gdo;
				uu[i] = px[i]*gux + py[i]*guy + guo;
				vv[i] = px[i]*gvx + py[i]*gvy + gvo;
			}
		}

		if (glshbound) //shade, pal, shades per unit of eye z (3D only), and masked or translucent, for the shader
		{
			polymost_shbox(px,py,n,behind,shb);
			k = method&3; if ((k >= 2) && (!glshtrans)) k = 1;
			if ((k >= 2) && (!glshcolor)) polymost_transread(shb);
			polymost_shprog((k >= 2) ? 2 : 1);
			polymost_shtile(globalpicnum,method&4);
			pc[0] = pc[1] = pc[2] = pc[3] = 1.f;
			glsub_color((float)globalshade+glskyshade,(float)globalpal,((glscene) && (fog_is_on))?glfogshades:0.f,
				(float)k);
			if (!waloff[globalpicnum]) glColorMask(0,0,0,0); //the mirror hack: only depth
		}
		else
		{
		f = polymost_shaderamp(globalpicnum,globalshade,&zero);
		if (fog_is_on) polymost_setfog(globalshade,zero);
		pc[0] = pc[1] = pc[2] = f;
		switch(method&3)
		{
			case 0: pc[3] = 1.0; break;
			case 1: pc[3] = 1.0; break;
			case 2: pc[3] = 0.66; break;
			case 3: pc[3] = 0.33; break;
		}
		glsub_color(pc[0],pc[1],pc[2],pc[3]);
		}

			//Hack for walls&masked walls which use textures that are not a power of 2
		if ((pow2xsplit) && (tsizx != xx))
		{
			if (!dorot)
			{
				ngdx = gdx; ngdy = gdy; ngdo = gdo+(ngdx+ngdy)*.5;
				ngux = gux; nguy = guy; nguo = guo+(ngux+nguy)*.5;
				ngvx = gvx; ngvy = gvy; ngvo = gvo+(ngvx+ngvy)*.5;
			}
			else
			{
				ox = py[1]-py[2]; oy = py[2]-py[0]; oz = py[0]-py[1];
				r = 1.0 / (ox*px[0] + oy*px[1] + oz*px[2]);
				ngdx = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
				ngux = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
				ngvx = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
				ox = px[2]-px[1]; oy = px[0]-px[2]; oz = px[1]-px[0];
				ngdy = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
				nguy = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
				ngvy = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
				ox = px[0]-.5; oy = py[0]-.5; //.5 centers texture nicely
				ngdo = dd[0] - ox*ngdx - oy*ngdy;
				nguo = uu[0] - ox*ngux - oy*nguy;
				ngvo = vv[0] - ox*ngvx - oy*ngvy;
			}

			ngux *= hackscx; nguy *= hackscx; nguo *= hackscx;
			ngvx *= hackscy; ngvy *= hackscy; ngvo *= hackscy;
			uoffs = ((double)(xx-tsizx)*.5);
			ngux -= ngdx*uoffs;
			nguy -= ngdy*uoffs;
			nguo -= ngdo*uoffs;

				//Find min&max u coordinates (du0...du1)
			for(i=0;i<n;i++)
			{
				ox = px[i]; oy = py[i];
				f = (ox*ngux + oy*nguy + nguo) / (ox*ngdx + oy*ngdy + ngdo);
				if (!i) { du0 = du1 = f; continue; }
					  if (f < du0) du0 = f;
				else if (f > du1) du1 = f;
			}

			f = 1.0/(double)tsizx;
			ix0 = (int32_t)floor(du0*f);
			ix1 = (int32_t)floor(du1*f);
			if (!isinf(f)) for(;ix0<=ix1;ix0++)
			{
				du0 = (double)((ix0  )*tsizx); // + uoffs;
				du1 = (double)((ix0+1)*tsizx); // + uoffs;

				i = 0; nn = 0;
				duj = (px[i]*ngux + py[i]*nguy + nguo) / (px[i]*ngdx + py[i]*ngdy + ngdo);
				do
				{
					j = i+1; if (j == n) j = 0;

					dui = duj; duj = (px[j]*ngux + py[j]*nguy + nguo) / (px[j]*ngdx + py[j]*ngdy + ngdo);

					if ((du0 <= dui) && (dui <= du1)) { uu[nn] = px[i]; vv[nn] = py[i]; nn++; }
					if (duj <= dui)
					{
						if ((du1 < duj) != (du1 < dui))
						{
								//ox*(ngux-ngdx*du1) + oy*(nguy-ngdy*du1) + (nguo-ngdo*du1) = 0
								//(px[j]-px[i])*f + px[i] = ox
								//(py[j]-py[i])*f + py[i] = oy

								///Solve for f
								//((px[j]-px[i])*f + px[i])*(ngux-ngdx*du1) +
								//((py[j]-py[i])*f + py[i])*(nguy-ngdy*du1) + (nguo-ngdo*du1) = 0

							f = -(       px[i] *(ngux-ngdx*du1) +  py[i]       *(nguy-ngdy*du1) + (nguo-ngdo*du1)) /
								  ((px[j]-px[i])*(ngux-ngdx*du1) + (py[j]-py[i])*(nguy-ngdy*du1));
							uu[nn] = (px[j]-px[i])*f + px[i];
							vv[nn] = (py[j]-py[i])*f + py[i]; nn++;
						}
						if ((du0 < duj) != (du0 < dui))
						{
							f = -(       px[i] *(ngux-ngdx*du0) +        py[i] *(nguy-ngdy*du0) + (nguo-ngdo*du0)) /
								  ((px[j]-px[i])*(ngux-ngdx*du0) + (py[j]-py[i])*(nguy-ngdy*du0));
							uu[nn] = (px[j]-px[i])*f + px[i];
							vv[nn] = (py[j]-py[i])*f + py[i]; nn++;
						}
					}
					else
					{
						if ((du0 < duj) != (du0 < dui))
						{
							f = -(       px[i] *(ngux-ngdx*du0) +        py[i] *(nguy-ngdy*du0) + (nguo-ngdo*du0)) /
								  ((px[j]-px[i])*(ngux-ngdx*du0) + (py[j]-py[i])*(nguy-ngdy*du0));
							uu[nn] = (px[j]-px[i])*f + px[i];
							vv[nn] = (py[j]-py[i])*f + py[i]; nn++;
						}
						if ((du1 < duj) != (du1 < dui))
						{
							f = -(       px[i] *(ngux-ngdx*du1) +  py[i]       *(nguy-ngdy*du1) + (nguo-ngdo*du1)) /
								  ((px[j]-px[i])*(ngux-ngdx*du1) + (py[j]-py[i])*(nguy-ngdy*du1));
							uu[nn] = (px[j]-px[i])*f + px[i];
							vv[nn] = (py[j]-py[i])*f + py[i]; nn++;
						}
					}
					i = j;
				} while (i);
				if (nn < 3) continue;

				for(k=0;k<=(fbtex!=0);k++)
				{
					if (k) polymost_glowbegin(fbtex,pc[3]);
					glsub_begin(GL_TRIANGLE_FAN);
					for(i=0;i<nn;i++)
					{
						ox = uu[i]; oy = vv[i];
						dp = ox*ngdx + oy*ngdy + ngdo;
						up = ox*ngux + oy*nguy + nguo;
						vp = ox*ngvx + oy*ngvy + ngvo;
						r = 1.0/dp; glsub_texcoord((up*r-du0+uoffs)*ox2,vp*r*oy2);
						glsub_vertex((ox-ghalfx)*r*grhalfxdown10x,(ghoriz-oy)*r*grhalfxdown10,r*(1.0/1024.0));
					}
					glsub_end();
					if (k) polymost_glowend(basetex,pc,method);
				}
			}
		}
		else
		{
			ox2 *= hackscx; oy2 *= hackscy;
			for(k=0;k<=(fbtex!=0);k++)
			{
				if (k) polymost_glowbegin(fbtex,pc[3]);
				glsub_begin(GL_TRIANGLE_FAN);
				for(i=0;i<n;i++)
				{
					r = 1.0/dd[i]; glsub_texcoord(uu[i]*r*ox2,vv[i]*r*oy2);
					glsub_vertex((px[i]-ghalfx)*r*grhalfxdown10x,(ghoriz-py[i])*r*grhalfxdown10,r*(1.0/1024.0));
				}
				glsub_end();
				if (k) polymost_glowend(basetex,pc,method);
			}
		}
		if ((glshbound) && (!waloff[globalpicnum])) glColorMask(1,1,1,1);
		if (glshbound) polymost_drawn(shb);
		return;
	}
#endif

	if (rendmode == 2)
	{
#if (USEZBUFFER != 0)
		if ((!zbufmem) || (zbufbpl != bytesperline) || (zbufysiz != ydim))
		{
			zbufbpl = bytesperline;
			zbufysiz = ydim;
			zbufmem = (int32_t *)realloc(zbufmem,zbufbpl*zbufysiz*4);
		}
#endif
		if ((!transluc)) method = (method&~3)+1; //In case translucent table doesn't exist

		if (!dorot)
		{
			ngdx = gdx; ngdy = gdy; ngdo = gdo+(ngdx+ngdy)*.5;
			ngux = gux; nguy = guy; nguo = guo+(ngux+nguy)*.5;
			ngvx = gvx; ngvy = gvy; ngvo = gvo+(ngvx+ngvy)*.5;
		}
		else
		{
				//General equations:
				//dd[i] = (px[i]*gdx + py[i]*gdy + gdo)
				//uu[i] = (px[i]*gux + py[i]*guy + guo)/dd[i]
				//vv[i] = (px[i]*gvx + py[i]*gvy + gvo)/dd[i]
				//
				//px[0]*gdx + py[0]*gdy + 1*gdo = dd[0]
				//px[1]*gdx + py[1]*gdy + 1*gdo = dd[1]
				//px[2]*gdx + py[2]*gdy + 1*gdo = dd[2]
				//
				//px[0]*gux + py[0]*guy + 1*guo = uu[0]*dd[0] (uu[i] premultiplied by dd[i] above)
				//px[1]*gux + py[1]*guy + 1*guo = uu[1]*dd[1]
				//px[2]*gux + py[2]*guy + 1*guo = uu[2]*dd[2]
				//
				//px[0]*gvx + py[0]*gvy + 1*gvo = vv[0]*dd[0] (vv[i] premultiplied by dd[i] above)
				//px[1]*gvx + py[1]*gvy + 1*gvo = vv[1]*dd[1]
				//px[2]*gvx + py[2]*gvy + 1*gvo = vv[2]*dd[2]
			ox = py[1]-py[2]; oy = py[2]-py[0]; oz = py[0]-py[1];
			r = 1.0 / (ox*px[0] + oy*px[1] + oz*px[2]);
			ngdx = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
			ngux = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
			ngvx = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
			ox = px[2]-px[1]; oy = px[0]-px[2]; oz = px[1]-px[0];
			ngdy = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
			nguy = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
			ngvy = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
			ox = px[0]-.5; oy = py[0]-.5; //.5 centers texture nicely
			ngdo = dd[0] - ox*ngdx - oy*ngdy;
			nguo = uu[0] - ox*ngux - oy*nguy;
			ngvo = vv[0] - ox*ngvx - oy*ngvy;
		}
		palptr = &palookup[globalpal][min(max(globalshade,0),numpalookups-1)<<8]; //<-need to make shade not static!

		tsizxm1 = tsizx-1; xmodnice = (!(tsizxm1&tsizx));
		tsizym1 = tsizy-1; ymulnice = (!(tsizym1&tsizy));
		if ((method&4) && (!xmodnice)) //Sprites don't need a mod on texture coordinates
			{ xmodnice = 1; for(tsizxm1=1;tsizxm1<tsizx;tsizxm1=(tsizxm1<<1)+1); }
		if (!ymulnice) { for(tsizym1=1;tsizym1+1<tsizy;tsizym1=(tsizym1<<1)+1); }
		ltsizy = (picsiz[globalpicnum]>>4);
	}
	else
	{
		dacol = palookup[0][(int32_t)(*(char *)(waloff[globalpicnum]))+(min(max(globalshade,0),numpalookups-1)<<8)];
	}

	if (grhalfxdown10x < 0) //Hack for mirrors
	{
		for(i=((n-1)>>1);i>=0;i--)
		{
			r = px[i]; px[i] = ((double)xdimen)-px[n-1-i]; px[n-1-i] = ((double)xdimen)-r;
			r = py[i]; py[i] = py[n-1-i]; py[n-1-i] = r;
		}
		ngdo += ((double)xdimen)*ngdx; ngdx = -ngdx;
		nguo += ((double)xdimen)*ngux; ngux = -ngux;
		ngvo += ((double)xdimen)*ngvx; ngvx = -ngvx;
	}

	ngdx2 = ngdx*(1<<LINTERPSIZ);
	ngux2 = ngux*(1<<LINTERPSIZ);
	ngvx2 = ngvx*(1<<LINTERPSIZ);

	mini = (py[0] >= py[1]); maxi = 1-mini;
	for(z=2;z<n;z++)
	{
		if (py[z] < py[mini]) mini = z;
		if (py[z] > py[maxi]) maxi = z;
	}

	i = maxi; dtol(py[i],&yy); if (yy > ydimen) yy = ydimen;
	do
	{
		j = i+1; if (j == n) j = 0;
		dtol(py[j],&y); if (y < 0) y = 0;
		if (y < yy)
		{
			f = (px[j]-px[i])/(py[j]-py[i]); dtol(f*16384.0,&xi);
			dtol((((double)yy-.5-py[j])*f + px[j])*16384.0+8192.0,&x);
			for(;yy>y;yy--,x-=xi) lastx[yy-1] = (x>>14);
		}
		i = j;
	} while (i != mini);
	do
	{
		j = i+1; if (j == n) j = 0;
		dtol(py[j],&yy); if (yy > ydimen) yy = ydimen;
		if (y < yy)
		{
			f = (px[j]-px[i])/(py[j]-py[i]); dtol(f*16384.0,&xi);
			dtol((((double)y+.5-py[j])*f + px[j])*16384.0+8192.0,&x);
			for(;y<yy;y++,x+=xi)
			{
				ix0 = lastx[y]; if (ix0 < 0) ix0 = 0;
				ix1 = (x>>14); if (ix1 > xdimen) ix1 = xdimen;
				if (ix0 < ix1)
				{
					if (rendmode == 1)
						memset((void *)(ylookup[y]+ix0+frameoffset),dacol,ix1-ix0);
					else
					{
						vidp = (char *)(ylookup[y]+frameoffset+ix0);
						dp = ngdx*(double)ix0 + ngdy*(double)y + ngdo;
						up = ngux*(double)ix0 + nguy*(double)y + nguo;
						vp = ngvx*(double)ix0 + ngvy*(double)y + ngvo;
						rdp = 65536.0/dp; dp += ngdx2;
						dtol(   rdp,&d0);
						dtol(up*rdp,&u0); up += ngux2;
						dtol(vp*rdp,&v0); vp += ngvx2;
						rdp = 65536.0/dp;

						switch (method&3)
						{
							case 0:
								if (xmodnice&ymulnice) //both u&v texture sizes are powers of 2 :)
								{
									for(xx=ix0;xx<ix1;xx+=(1<<LINTERPSIZ))
									{
										dtol(   rdp,&d1); dp += ngdx2; d1 = ((d1-d0)>>LINTERPSIZ);
										dtol(up*rdp,&u1); up += ngux2; u1 = ((u1-u0)>>LINTERPSIZ);
										dtol(vp*rdp,&v1); vp += ngvx2; v1 = ((v1-v0)>>LINTERPSIZ);
										rdp = 65536.0/dp; vide = &vidp[min(ix1-xx,1<<LINTERPSIZ)];
										while (vidp < vide)
										{
#if (DEPTHDEBUG == 0)
#if (USEZBUFFER != 0)
											zbufmem[vidp-(char *)frameplace] = d0+16384; //+ hack so wall&floor sprites don't disappear
#endif
											vidp[0] = palptr[(unsigned char)walptr[(((u0>>16)&tsizxm1)<<ltsizy) + ((v0>>16)&tsizym1)]]; //+((d0>>13)&0x3f00)];
#else
											vidp[0] = ((d0>>16)&255);
#endif
											d0 += d1; u0 += u1; v0 += v1; vidp++;
										} while (vidp < vide);
									}
								}
								else
								{
									for(xx=ix0;xx<ix1;xx+=(1<<LINTERPSIZ))
									{
										dtol(   rdp,&d1); dp += ngdx2; d1 = ((d1-d0)>>LINTERPSIZ);
										dtol(up*rdp,&u1); up += ngux2; u1 = ((u1-u0)>>LINTERPSIZ);
										dtol(vp*rdp,&v1); vp += ngvx2; v1 = ((v1-v0)>>LINTERPSIZ);
										rdp = 65536.0/dp; vide = &vidp[min(ix1-xx,1<<LINTERPSIZ)];
										while (vidp < vide)
										{
#if (DEPTHDEBUG == 0)
#if (USEZBUFFER != 0)
											if (zbufwrite) zbufmem[vidp-(char *)frameplace] = d0;
#endif
											vidp[0] = palptr[(unsigned char)walptr[imod(u0>>16,tsizx)*tsizy + ((v0>>16)&tsizym1)]]; //+((d0>>13)&0x3f00)];
#else
											vidp[0] = ((d0>>16)&255);
#endif
											d0 += d1; u0 += u1; v0 += v1; vidp++;
										} while (vidp < vide);
									}
								}
								break;
							case 1:
								if (xmodnice) //both u&v texture sizes are powers of 2 :)
								{
									for(xx=ix0;xx<ix1;xx+=(1<<LINTERPSIZ))
									{
										dtol(   rdp,&d1); dp += ngdx2; d1 = ((d1-d0)>>LINTERPSIZ);
										dtol(up*rdp,&u1); up += ngux2; u1 = ((u1-u0)>>LINTERPSIZ);
										dtol(vp*rdp,&v1); vp += ngvx2; v1 = ((v1-v0)>>LINTERPSIZ);
										rdp = 65536.0/dp; vide = &vidp[min(ix1-xx,1<<LINTERPSIZ)];
										while (vidp < vide)
										{
											dacol = walptr[(((u0>>16)&tsizxm1)*tsizy) + ((v0>>16)&tsizym1)];
#if (DEPTHDEBUG == 0)
#if (USEZBUFFER != 0)
											if ((dacol != 255) && (d0 <= zbufmem[vidp-(char *)frameplace]))
											{
												if (zbufwrite) zbufmem[vidp-(char *)frameplace] = d0;
												vidp[0] = palptr[((int32_t)dacol)]; //+((d0>>13)&0x3f00)];
											}
#else
											if (dacol != 255) vidp[0] = palptr[((int32_t)dacol)]; //+((d0>>13)&0x3f00)];
#endif
#else
											if ((dacol != 255) && (vidp[0] > (d0>>16))) vidp[0] = ((d0>>16)&255);
#endif
											d0 += d1; u0 += u1; v0 += v1; vidp++;
										} while (vidp < vide);
									}
								}
								else
								{
									for(xx=ix0;xx<ix1;xx+=(1<<LINTERPSIZ))
									{
										dtol(   rdp,&d1); dp += ngdx2; d1 = ((d1-d0)>>LINTERPSIZ);
										dtol(up*rdp,&u1); up += ngux2; u1 = ((u1-u0)>>LINTERPSIZ);
										dtol(vp*rdp,&v1); vp += ngvx2; v1 = ((v1-v0)>>LINTERPSIZ);
										rdp = 65536.0/dp; vide = &vidp[min(ix1-xx,1<<LINTERPSIZ)];
										while (vidp < vide)
										{
											dacol = walptr[imod(u0>>16,tsizx)*tsizy + ((v0>>16)&tsizym1)];
#if (DEPTHDEBUG == 0)
#if (USEZBUFFER != 0)
											if ((dacol != 255) && (d0 <= zbufmem[vidp-(char *)frameplace]))
											{
												if (zbufwrite) zbufmem[vidp-(char *)frameplace] = d0;
												vidp[0] = palptr[((int32_t)dacol)]; //+((d0>>13)&0x3f00)];
											}
#else
											if (dacol != 255) vidp[0] = palptr[((int32_t)dacol)]; //+((d0>>13)&0x3f00)];
#endif
#else
											if ((dacol != 255) && (vidp[0] > (d0>>16))) vidp[0] = ((d0>>16)&255);
#endif
											d0 += d1; u0 += u1; v0 += v1; vidp++;
										} while (vidp < vide);
									}
								}
								break;
							case 2: //Transluscence #1
								for(xx=ix0;xx<ix1;xx+=(1<<LINTERPSIZ))
								{
									dtol(   rdp,&d1); dp += ngdx2; d1 = ((d1-d0)>>LINTERPSIZ);
									dtol(up*rdp,&u1); up += ngux2; u1 = ((u1-u0)>>LINTERPSIZ);
									dtol(vp*rdp,&v1); vp += ngvx2; v1 = ((v1-v0)>>LINTERPSIZ);
									rdp = 65536.0/dp; vide = &vidp[min(ix1-xx,1<<LINTERPSIZ)];
									while (vidp < vide)
									{
										dacol = walptr[imod(u0>>16,tsizx)*tsizy + ((v0>>16)&tsizym1)];
										//dacol = walptr[(((u0>>16)&tsizxm1)<<ltsizy) + ((v0>>16)&tsizym1)];
#if (DEPTHDEBUG == 0)
#if (USEZBUFFER != 0)
										if ((dacol != 255) && (d0 <= zbufmem[vidp-(char *)frameplace]))
										{
											if (zbufwrite) zbufmem[vidp-(char *)frameplace] = d0;
											vidp[0] = transluc[(((int32_t)vidp[0])<<8)+((int32_t)palptr[((int32_t)dacol)])]; //+((d0>>13)&0x3f00)])];
										}
#else
										if (dacol != 255)
											vidp[0] = transluc[(((int32_t)vidp[0])<<8)+((int32_t)palptr[((int32_t)dacol)])]; //+((d0>>13)&0x3f00)])];
#endif
#else
										if ((dacol != 255) && (vidp[0] > (d0>>16))) vidp[0] = ((d0>>16)&255);
#endif
										d0 += d1; u0 += u1; v0 += v1; vidp++;
									} while (vidp < vide);
								}
								break;
							case 3: //Transluscence #2
								for(xx=ix0;xx<ix1;xx+=(1<<LINTERPSIZ))
								{
									dtol(   rdp,&d1); dp += ngdx2; d1 = ((d1-d0)>>LINTERPSIZ);
									dtol(up*rdp,&u1); up += ngux2; u1 = ((u1-u0)>>LINTERPSIZ);
									dtol(vp*rdp,&v1); vp += ngvx2; v1 = ((v1-v0)>>LINTERPSIZ);
									rdp = 65536.0/dp; vide = &vidp[min(ix1-xx,1<<LINTERPSIZ)];
									while (vidp < vide)
									{
										dacol = walptr[imod(u0>>16,tsizx)*tsizy + ((v0>>16)&tsizym1)];
										//dacol = walptr[(((u0>>16)&tsizxm1)<<ltsizy) + ((v0>>16)&tsizym1)];
#if (DEPTHDEBUG == 0)
#if (USEZBUFFER != 0)
										if ((dacol != 255) && (d0 <= zbufmem[vidp-(char *)frameplace]))
										{
											if (zbufwrite) zbufmem[vidp-(char *)frameplace] = d0;
											vidp[0] = transluc[((int32_t)vidp[0])+(((int32_t)palptr[((int32_t)dacol)/*+((d0>>13)&0x3f00)*/])<<8)];
										}
#else
										if (dacol != 255)
											vidp[0] = transluc[((int32_t)vidp[0])+(((int32_t)palptr[((int32_t)dacol)/*+((d0>>13)&0x3f00)*/])<<8)];
#endif
#else
										if ((dacol != 255) && (vidp[0] > (d0>>16))) vidp[0] = ((d0>>16)&255);
#endif
										d0 += d1; u0 += u1; v0 += v1; vidp++;
									} while (vidp < vide);
								}
								break;
						}
					}
				}
			}
		}
		i = j;
	} while (i != maxi);

	if (rendmode == 1)
	{
		for(i=0,j=n-1;i<n;j=i,i++) drawline2d(px[i],py[i],px[j],py[j],31); //hopefully color index 31 is white

		//ox = 0; oy = 0;
		//for(i=0;i<n;i++) { ox += px[i]; oy += py[i]; }
		//ox /= (double)n; oy /= (double)n;
		//for(i=0,j=n-1;i<n;j=i,i++) drawline2d(px[i]+(ox-px[i])*.125,py[i]+(oy-py[i])*.125,px[j]+(ox-px[j])*.125,py[j]+(oy-py[j])*.125,31);
	}
}

	//Init viewport boundary (must be 4 point convex loop):
	//      (px[0],py[0]).----.(px[1],py[1])
	//                  /      \
	//                /          \
	// (px[3],py[3]).--------------.(px[2],py[2])
	//
void initmosts (double *px, double *py, int32_t n)
{
	int32_t i, j, k, imin;

	vcnt = 1; //0 is dummy solid node

	if (n < 3) return;
	imin = (px[1] < px[0]);
	for(i=n-1;i>=2;i--) if (px[i] < px[imin]) imin = i;


	vsp[vcnt].x = px[imin];
	vsp[vcnt].cy[0] = vsp[vcnt].fy[0] = py[imin];
	vcnt++;
	i = imin+1; if (i >= n) i = 0;
	j = imin-1; if (j < 0) j = n-1;
	do
	{
		if (px[i] < px[j])
		{
			if ((vcnt > 1) && (px[i] <= vsp[vcnt-1].x)) vcnt--;
			vsp[vcnt].x = px[i];
			vsp[vcnt].cy[0] = py[i];
			k = j+1; if (k >= n) k = 0;
				//(px[k],py[k])
				//(px[i],?)
				//(px[j],py[j])
			vsp[vcnt].fy[0] = (px[i]-px[k])*(py[j]-py[k])/(px[j]-px[k]) + py[k];
			vcnt++;
			i++; if (i >= n) i = 0;
		}
		else if (px[j] < px[i])
		{
			if ((vcnt > 1) && (px[j] <= vsp[vcnt-1].x)) vcnt--;
			vsp[vcnt].x = px[j];
			vsp[vcnt].fy[0] = py[j];
			k = i-1; if (k < 0) k = n-1;
				//(px[k],py[k])
				//(px[j],?)
				//(px[i],py[i])
			vsp[vcnt].cy[0] = (px[j]-px[k])*(py[i]-py[k])/(px[i]-px[k]) + py[k];
			vcnt++;
			j--; if (j < 0) j = n-1;
		}
		else
		{
			if ((vcnt > 1) && (px[i] <= vsp[vcnt-1].x)) vcnt--;
			vsp[vcnt].x = px[i];
			vsp[vcnt].cy[0] = py[i];
			vsp[vcnt].fy[0] = py[j];
			vcnt++;
			i++; if (i >= n) i = 0; if (i == j) break;
			j--; if (j < 0) j = n-1;
		}
	} while (i != j);
	if (px[i] > vsp[vcnt-1].x)
	{
		vsp[vcnt].x = px[i];
		vsp[vcnt].cy[0] = vsp[vcnt].fy[0] = py[i];
		vcnt++;
	}


	for(i=0;i<vcnt;i++)
	{
		vsp[i].cy[1] = vsp[i+1].cy[0]; vsp[i].ctag = i;
		vsp[i].fy[1] = vsp[i+1].fy[0]; vsp[i].ftag = i;
		vsp[i].n = i+1; vsp[i].p = i-1;
	}
	vsp[vcnt-1].n = 0; vsp[0].p = vcnt-1;
	gtag = vcnt;

		//VSPMAX-1 is dummy empty node
	for(i=vcnt;i<VSPMAX;i++) { vsp[i].n = i+1; vsp[i].p = i-1; }
	vsp[VSPMAX-1].n = vcnt; vsp[vcnt].p = VSPMAX-1;
}

void vsdel (int32_t i)
{
	int32_t pi, ni;
		//Delete i
	pi = vsp[i].p;
	ni = vsp[i].n;
	vsp[ni].p = pi;
	vsp[pi].n = ni;

		//Add i to empty list
	vsp[i].n = vsp[VSPMAX-1].n;
	vsp[i].p = VSPMAX-1;
	vsp[vsp[VSPMAX-1].n].p = i;
	vsp[VSPMAX-1].n = i;
}

int32_t vsinsaft (int32_t i)
{
	int32_t r;
		//i = next element from empty list
	r = vsp[VSPMAX-1].n;
	vsp[vsp[r].n].p = VSPMAX-1;
	vsp[VSPMAX-1].n = vsp[r].n;

	vsp[r] = vsp[i]; //copy i to r

		//insert r after i
	vsp[r].p = i; vsp[r].n = vsp[i].n;
	vsp[vsp[i].n].p = r; vsp[i].n = r;

	return(r);
}

int32_t testvisiblemost (float x0, float x1)
{
	int32_t i, newi;

	for(i=vsp[0].n;i;i=newi)
	{
		newi = vsp[i].n;
		if ((x0 < vsp[newi].x) && (vsp[i].x < x1) && (vsp[i].ctag >= 0)) return(1);
	}
	return(0);
}

static int32_t domostlayers = METH_LAYERS; //0 while drawing the parallaxed sky: it has no glow pass
void domost (float x0, float y0, float x1, float y1)
{
	double dpx[4], dpy[4];
	float d, f, n, t, slop, dx, dx0, dx1, nx, nx0, ny0, nx1, ny1;
	float spx[4], spy[4], cy[2], cv[2];
	int32_t i, j, k, z, ni, vcnt, scnt, newi, dir, spt[4];

	if (x0 < x1)
	{
		dir = 1; //clip dmost (floor)
		y0 -= .01; y1 -= .01;
	}
	else
	{
		if (x0 == x1) return;
		f = x0; x0 = x1; x1 = f;
		f = y0; y0 = y1; y1 = f;
		dir = 0; //clip umost (ceiling)
		//y0 += .01; y1 += .01; //necessary?
	}

	slop = (y1-y0)/(x1-x0);
	for(i=vsp[0].n;i;i=newi)
	{
		newi = vsp[i].n; nx0 = vsp[i].x; nx1 = vsp[newi].x;
		if ((x0 >= nx1) || (nx0 >= x1) || (vsp[i].ctag <= 0)) continue;
		dx = nx1-nx0;
		cy[0] = vsp[i].cy[0]; cv[0] = vsp[i].cy[1]-cy[0];
		cy[1] = vsp[i].fy[0]; cv[1] = vsp[i].fy[1]-cy[1];

		scnt = 0;

			//Test if left edge requires split (x0,y0) (nx0,cy(0)),<dx,cv(0)>
		if ((x0 > nx0) && (x0 < nx1))
		{
			t = (x0-nx0)*cv[dir] - (y0-cy[dir])*dx;
			if (((!dir) && (t < 0)) || ((dir) && (t > 0)))
				{ spx[scnt] = x0; spy[scnt] = y0; spt[scnt] = -1; scnt++; }
		}

			//Test for intersection on umost (j == 0) and dmost (j == 1)
		for(j=0;j<2;j++)
		{
			d = (y0-y1)*dx - (x0-x1)*cv[j];
			n = (y0-cy[j])*dx - (x0-nx0)*cv[j];
			if ((fabs(n) <= fabs(d)) && (d*n >= 0) && (d != 0))
			{
				t = n/d; nx = (x1-x0)*t + x0;
				if ((nx > nx0) && (nx < nx1))
				{
					spx[scnt] = nx; spy[scnt] = (y1-y0)*t + y0;
					spt[scnt] = j; scnt++;
				}
			}
		}

			//Nice hack to avoid full sort later :)
		if ((scnt >= 2) && (spx[scnt-1] < spx[scnt-2]))
		{
			f = spx[scnt-1]; spx[scnt-1] = spx[scnt-2]; spx[scnt-2] = f;
			f = spy[scnt-1]; spy[scnt-1] = spy[scnt-2]; spy[scnt-2] = f;
			j = spt[scnt-1]; spt[scnt-1] = spt[scnt-2]; spt[scnt-2] = j;
		}

			//Test if right edge requires split
		if ((x1 > nx0) && (x1 < nx1))
		{
			t = (x1-nx0)*cv[dir] - (y1-cy[dir])*dx;
			if (((!dir) && (t < 0)) || ((dir) && (t > 0)))
				{ spx[scnt] = x1; spy[scnt] = y1; spt[scnt] = -1; scnt++; }
		}

		vsp[i].tag = vsp[newi].tag = -1;
		for(z=0;z<=scnt;z++,i=vcnt)
		{
			if (z < scnt)
			{
				vcnt = vsinsaft(i);
				t = (spx[z]-nx0)/dx;
				vsp[i].cy[1] = t*cv[0] + cy[0];
				vsp[i].fy[1] = t*cv[1] + cy[1];
				vsp[vcnt].x = spx[z];
				vsp[vcnt].cy[0] = vsp[i].cy[1];
				vsp[vcnt].fy[0] = vsp[i].fy[1];
				vsp[vcnt].tag = spt[z];
			}

			ni = vsp[i].n; if (!ni) continue; //this 'if' fixes many bugs!
			dx0 = vsp[i].x; if (x0 > dx0) continue;
			dx1 = vsp[ni].x; if (x1 < dx1) continue;
			ny0 = (dx0-x0)*slop + y0;
			ny1 = (dx1-x0)*slop + y0;

				//      dx0           dx1
				//       ³             ³
				//----------------------------
				//     t0+=0         t1+=0
				//   vsp[i].cy[0]  vsp[i].cy[1]
				//============================
				//     t0+=1         t1+=3
				//============================
				//   vsp[i].fy[0]    vsp[i].fy[1]
				//     t0+=2         t1+=6
				//
				//     ny0 ?         ny1 ?

			k = 1+3;
			if ((vsp[i].tag == 0) || (ny0 <= vsp[i].cy[0]+.01)) k--;
			if ((vsp[i].tag == 1) || (ny0 >= vsp[i].fy[0]-.01)) k++;
			if ((vsp[ni].tag == 0) || (ny1 <= vsp[i].cy[1]+.01)) k -= 3;
			if ((vsp[ni].tag == 1) || (ny1 >= vsp[i].fy[1]-.01)) k += 3;

			if (!dir)
			{
				switch(k)
				{
					case 1: case 2:
						dpx[0] = dx0; dpy[0] = vsp[i].cy[0];
						dpx[1] = dx1; dpy[1] = vsp[i].cy[1];
						dpx[2] = dx0; dpy[2] = ny0; drawpoly(dpx,dpy,3,domostlayers);
						vsp[i].cy[0] = ny0; vsp[i].ctag = gtag; break;
					case 3: case 6:
						dpx[0] = dx0; dpy[0] = vsp[i].cy[0];
						dpx[1] = dx1; dpy[1] = vsp[i].cy[1];
						dpx[2] = dx1; dpy[2] = ny1; drawpoly(dpx,dpy,3,domostlayers);
						vsp[i].cy[1] = ny1; vsp[i].ctag = gtag; break;
					case 4: case 5: case 7:
						dpx[0] = dx0; dpy[0] = vsp[i].cy[0];
						dpx[1] = dx1; dpy[1] = vsp[i].cy[1];
						dpx[2] = dx1; dpy[2] = ny1;
						dpx[3] = dx0; dpy[3] = ny0; drawpoly(dpx,dpy,4,domostlayers);
						vsp[i].cy[0] = ny0; vsp[i].cy[1] = ny1; vsp[i].ctag = gtag; break;
					case 8:
						dpx[0] = dx0; dpy[0] = vsp[i].cy[0];
						dpx[1] = dx1; dpy[1] = vsp[i].cy[1];
						dpx[2] = dx1; dpy[2] = vsp[i].fy[1];
						dpx[3] = dx0; dpy[3] = vsp[i].fy[0]; drawpoly(dpx,dpy,4,domostlayers);
						vsp[i].ctag = vsp[i].ftag = -1; break;
					default: break;
				}
			}
			else
			{
				switch(k)
				{
					case 7: case 6:
						dpx[0] = dx0; dpy[0] = ny0;
						dpx[1] = dx1; dpy[1] = vsp[i].fy[1];
						dpx[2] = dx0; dpy[2] = vsp[i].fy[0]; drawpoly(dpx,dpy,3,domostlayers);
						vsp[i].fy[0] = ny0; vsp[i].ftag = gtag; break;
					case 5: case 2:
						dpx[0] = dx0; dpy[0] = vsp[i].fy[0];
						dpx[1] = dx1; dpy[1] = ny1;
						dpx[2] = dx1; dpy[2] = vsp[i].fy[1]; drawpoly(dpx,dpy,3,domostlayers);
						vsp[i].fy[1] = ny1; vsp[i].ftag = gtag; break;
					case 4: case 3: case 1:
						dpx[0] = dx0; dpy[0] = ny0;
						dpx[1] = dx1; dpy[1] = ny1;
						dpx[2] = dx1; dpy[2] = vsp[i].fy[1];
						dpx[3] = dx0; dpy[3] = vsp[i].fy[0]; drawpoly(dpx,dpy,4,domostlayers);
						vsp[i].fy[0] = ny0; vsp[i].fy[1] = ny1; vsp[i].ftag = gtag; break;
					case 0:
						dpx[0] = dx0; dpy[0] = vsp[i].cy[0];
						dpx[1] = dx1; dpy[1] = vsp[i].cy[1];
						dpx[2] = dx1; dpy[2] = vsp[i].fy[1];
						dpx[3] = dx0; dpy[3] = vsp[i].fy[0]; drawpoly(dpx,dpy,4,domostlayers);
						vsp[i].ctag = vsp[i].ftag = -1; break;
					default: break;
				}
			}
		}
	}

	gtag++;

		//Combine neighboring vertical strips with matching collinear top&bottom edges
		//This prevents x-splits from propagating through the entire scan
	i = vsp[0].n;
	while (i)
	{
		ni = vsp[i].n;
		if ((vsp[i].cy[0] >= vsp[i].fy[0]) && (vsp[i].cy[1] >= vsp[i].fy[1])) { vsp[i].ctag = vsp[i].ftag = -1; }
		if ((vsp[i].ctag == vsp[ni].ctag) && (vsp[i].ftag == vsp[ni].ftag))
			{ vsp[i].cy[1] = vsp[ni].cy[1]; vsp[i].fy[1] = vsp[ni].fy[1]; vsdel(ni); }
		else i = ni;
	}
}

static void polymost_scansector (int32_t sectnum);

static void polymost_drawalls (int32_t bunch)
{
	sectortype *sec, *nextsec;
	walltype *wal, *wal2, *nwal;
	double ox, oy, oz, ox2, oy2, px[3], py[3], dd[3], uu[3], vv[3];
	double fx, fy, x0, x1, y0, y1, cy0, cy1, fy0, fy1, xp0, yp0, xp1, yp1, ryp0, ryp1, nx0, ny0, nx1, ny1;
	double t, r, t0, t1, ocy0, ocy1, ofy0, ofy1, oxp0, oyp0, ft[4];
	double oguo, ogux, oguy;
	int32_t i, x, y, z, cz, fz, wallnum, sectnum, nextsectnum;

	sectnum = thesector[bunchfirst[bunch]]; sec = &sector[sectnum];

#ifdef USE_OPENGL
	if (!nofog) {
	if (rendmode == 3)
		polymost_fogvis(sec->visibility);
	}
#endif

		//DRAW WALLS SECTION!
	for(z=bunchfirst[bunch];z>=0;z=p2[z])
	{
		wallnum = thewall[z]; wal = &wall[wallnum]; wal2 = &wall[wal->point2];
		nextsectnum = wal->nextsector; nextsec = &sector[nextsectnum];

			//Offset&Rotate 3D coordinates to screen 3D space
		x = wal->x-globalposx; y = wal->y-globalposy;
		xp0 = (double)y*gcosang  - (double)x*gsinang;
		yp0 = (double)x*gcosang2 + (double)y*gsinang2;
		x = wal2->x-globalposx; y = wal2->y-globalposy;
		xp1 = (double)y*gcosang  - (double)x*gsinang;
		yp1 = (double)x*gcosang2 + (double)y*gsinang2;

		oxp0 = xp0; oyp0 = yp0;

			//Clip to close parallel-screen plane
		if (yp0 < SCISDIST)
		{
			if (yp1 < SCISDIST) continue;
			t0 = (SCISDIST-yp0)/(yp1-yp0); xp0 = (xp1-xp0)*t0+xp0; yp0 = SCISDIST;
			nx0 = (wal2->x-wal->x)*t0+wal->x;
			ny0 = (wal2->y-wal->y)*t0+wal->y;
		}
		else { t0 = 0.f; nx0 = wal->x; ny0 = wal->y; }
		if (yp1 < SCISDIST)
		{
			t1 = (SCISDIST-oyp0)/(yp1-oyp0); xp1 = (xp1-oxp0)*t1+oxp0; yp1 = SCISDIST;
			nx1 = (wal2->x-wal->x)*t1+wal->x;
			ny1 = (wal2->y-wal->y)*t1+wal->y;
		}
		else { t1 = 1.f; nx1 = wal2->x; ny1 = wal2->y; }

		ryp0 = 1.f/yp0; ryp1 = 1.f/yp1;

			//Generate screen coordinates for front side of wall
		x0 = ghalfx*xp0*ryp0 + ghalfx;
		x1 = ghalfx*xp1*ryp1 + ghalfx;
		if (x1 <= x0) continue;

		ryp0 *= gyxscale; ryp1 *= gyxscale;

		getzsofslope(sectnum,nx0,ny0,&cz,&fz);
		cy0 = ((float)(cz-globalposz))*ryp0 + ghoriz;
		fy0 = ((float)(fz-globalposz))*ryp0 + ghoriz;
		getzsofslope(sectnum,nx1,ny1,&cz,&fz);
		cy1 = ((float)(cz-globalposz))*ryp1 + ghoriz;
		fy1 = ((float)(fz-globalposz))*ryp1 + ghoriz;


		globalpicnum = sec->floorpicnum; globalshade = sec->floorshade; globalpal = (int32_t)((unsigned char)sec->floorpal);
		globalorientation = sec->floorstat;
		if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,sectnum);
		if (!(globalorientation&1))
		{
				//(singlobalang/-16384*(sx-ghalfx) + 0*(sy-ghoriz) + (cosviewingrangeglobalang/16384)*ghalfx)*d + globalposx    = u*16
				//(cosglobalang/ 16384*(sx-ghalfx) + 0*(sy-ghoriz) + (sinviewingrangeglobalang/16384)*ghalfx)*d + globalposy    = v*16
				//(                  0*(sx-ghalfx) + 1*(sy-ghoriz) + (                             0)*ghalfx)*d + globalposz/16 = (sec->floorz/16)
			if (!(globalorientation&64))
				{ ft[0] = globalposx; ft[1] = globalposy; ft[2] = cosglobalang; ft[3] = singlobalang; }
			else
			{
					//relative alignment
				fx = (double)(wall[wall[sec->wallptr].point2].x-wall[sec->wallptr].x);
				fy = (double)(wall[wall[sec->wallptr].point2].y-wall[sec->wallptr].y);
				r = 1.0/sqrt(fx*fx+fy*fy); fx *= r; fy *= r;
				ft[2] = cosglobalang*fx + singlobalang*fy;
				ft[3] = singlobalang*fx - cosglobalang*fy;
				ft[0] = ((double)(globalposx-wall[sec->wallptr].x))*fx + ((double)(globalposy-wall[sec->wallptr].y))*fy;
				ft[1] = ((double)(globalposy-wall[sec->wallptr].y))*fx - ((double)(globalposx-wall[sec->wallptr].x))*fy;
				if (!(globalorientation&4)) globalorientation ^= 32; else globalorientation ^= 16;
			}
			gdx = 0;
			gdy = gxyaspect; if (!(globalorientation&2)) gdy /= (double)(sec->floorz-globalposz);
			gdo = -ghoriz*gdy;
			if (globalorientation&8) { ft[0] /= 8; ft[1] /= -8; ft[2] /= 2097152; ft[3] /= 2097152; }
									  else { ft[0] /= 16; ft[1] /= -16; ft[2] /= 4194304; ft[3] /= 4194304; }
			gux = (double)ft[3]*((double)viewingrange)/-65536.0;
			gvx = (double)ft[2]*((double)viewingrange)/-65536.0;
			guy = (double)ft[0]*gdy; gvy = (double)ft[1]*gdy;
			guo = (double)ft[0]*gdo; gvo = (double)ft[1]*gdo;
			guo += (double)(ft[2]-gux)*ghalfx;
			gvo -= (double)(ft[3]+gvx)*ghalfx;

				//Texture flipping
			if (globalorientation&4)
			{
				r = gux; gux = gvx; gvx = r;
				r = guy; guy = gvy; gvy = r;
				r = guo; guo = gvo; gvo = r;
			}
			if (globalorientation&16) { gux = -gux; guy = -guy; guo = -guo; }
			if (globalorientation&32) { gvx = -gvx; gvy = -gvy; gvo = -gvo; }

				//Texture panning
			fx = (float)sec->floorxpanning*((float)(1<<(picsiz[globalpicnum]&15)))/256.0;
			fy = (float)sec->floorypanning*((float)(1<<(picsiz[globalpicnum]>>4)))/256.0;
			if ((globalorientation&(2+64)) == (2+64)) //Hack for panning for slopes w/ relative alignment
			{
				r = (float)sec->floorheinum / 4096.0; r = 1.0/sqrt(r*r+1);
				if (!(globalorientation&4)) fy *= r; else fx *= r;
			}
			guy += gdy*fx; guo += gdo*fx;
			gvy += gdy*fy; gvo += gdo*fy;

			if (globalorientation&2) //slopes
			{
				px[0] = x0; py[0] = ryp0 + ghoriz;
				px[1] = x1; py[1] = ryp1 + ghoriz;

					//Pick some point guaranteed to be not collinear to the 1st two points
				ox = nx0 + (ny1-ny0);
				oy = ny0 + (nx0-nx1);
				ox2 = (double)(oy-globalposy)*gcosang  - (double)(ox-globalposx)*gsinang;
				oy2 = (double)(ox-globalposx)*gcosang2 + (double)(oy-globalposy)*gsinang2;
				oy2 = 1.0/oy2;
				px[2] = ghalfx*ox2*oy2 + ghalfx; oy2 *= gyxscale;
				py[2] = oy2 + ghoriz;

				for(i=0;i<3;i++)
				{
					dd[i] = px[i]*gdx + py[i]*gdy + gdo;
					uu[i] = px[i]*gux + py[i]*guy + guo;
					vv[i] = px[i]*gvx + py[i]*gvy + gvo;
				}

				py[0] = fy0;
				py[1] = fy1;
				py[2] = (getflorzofslope(sectnum,ox,oy)-globalposz)*oy2 + ghoriz;

				ox = py[1]-py[2]; oy = py[2]-py[0]; oz = py[0]-py[1];
				r = 1.0 / (ox*px[0] + oy*px[1] + oz*px[2]);
				gdx = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
				gux = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
				gvx = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
				ox = px[2]-px[1]; oy = px[0]-px[2]; oz = px[1]-px[0];
				gdy = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
				guy = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
				gvy = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
				gdo = dd[0] - px[0]*gdx - py[0]*gdy;
				guo = uu[0] - px[0]*gux - py[0]*guy;
				gvo = vv[0] - px[0]*gvx - py[0]*gvy;

				if (globalorientation&64) //Hack for relative alignment on slopes
				{
					r = (float)sec->floorheinum / 4096.0;
					r = sqrt(r*r+1);
					if (!(globalorientation&4)) { gvx *= r; gvy *= r; gvo *= r; }
												  else { gux *= r; guy *= r; guo *= r; }
				}
			}
			pow2xsplit = 0; domost(x0,fy0,x1,fy1); //flor
		}
		else if ((nextsectnum < 0) || (!(sector[nextsectnum].floorstat&1)))
		{
				//Parallaxing sky... hacked for Ken's mountain texture; paper-sky only :/
			domostlayers = 0;
#ifdef USE_OPENGL
			if (rendmode == 3)
			{
				glskyshade = polymost_skyshade(sec->visibility);
				if (!nofog) {
				glDisable(GL_FOG);
				fog_is_on = 0;
				//r = ((float)globalpisibility)*((float)((unsigned char)(sec->visibility+16)))*FOGSCALE;
				//r *= ((double)xdimscale*(double)viewingrange*gdo) / (65536.0*65536.0);
				//glFogf(GL_FOG_DENSITY,r);
				}

					//Use clamping for tiled sky textures
				for(i=(1<<pskybits)-1;i>0;i--)
					if (pskyoff[i] != pskyoff[i-1])
						{ skyclamphack = 1; break; }
			}
#endif
			if (1) //parallaxtype == 0)
			{
				dd[0] = (float)xdimen*.0000001; //Adjust sky depth based on screen size!
				t = (double)((1<<(picsiz[globalpicnum]&15))<<pskybits);
				vv[1] = dd[0]*((double)xdimscale*(double)viewingrange)/(65536.0*65536.0);
				vv[0] = dd[0]*((double)((tilesizy[globalpicnum]>>1)+parallaxyoffs)) - vv[1]*ghoriz;
				i = (1<<(picsiz[globalpicnum]>>4)); if (i != tilesizy[globalpicnum]) i += i;
				vv[0] += dd[0]*((double)sec->floorypanning)*((double)i)/256.0;


					//Hack to draw black rectangle below sky when looking down...
				gdx = 0; gdy = gxyaspect / 262144.0; gdo = -ghoriz*gdy;
				gux = 0; guy = 0; guo = 0;
				gvx = 0; gvy = (double)(tilesizy[globalpicnum]-1)*gdy; gvo = (double)(tilesizy[globalpicnum-1])*gdo;
				oy = (((double)tilesizy[globalpicnum])*dd[0]-vv[0])/vv[1];
				if ((oy > fy0) && (oy > fy1)) domost(x0,oy,x1,oy);
				else if ((oy > fy0) != (oy > fy1))
				{     //  fy0                      fy1
						//     \                    /
						//oy----------      oy----------
						//        \              /
						//         fy1        fy0
					ox = (oy-fy0)*(x1-x0)/(fy1-fy0) + x0;
					if (oy > fy0) { domost(x0,oy,ox,oy); domost(ox,oy,x1,fy1); }
							  else { domost(x0,fy0,ox,oy); domost(ox,oy,x1,oy); }
				} else domost(x0,fy0,x1,fy1);


				gdx = 0; gdy = 0; gdo = dd[0];
				gux = gdo*(t*((double)xdimscale)*((double)yxaspect)*((double)viewingrange))/(16384.0*65536.0*65536.0*5.0*1024.0);
				guy = 0; //guo calculated later
				gvx = 0; gvy = vv[1]; gvo = vv[0];

				i = globalpicnum; r = (fy1-fy0)/(x1-x0); //slope of line
				oy = ((double)viewingrange)/(ghalfx*256.0); oz = 1/oy;

				y = ((((int32_t)((x0-ghalfx)*oy))+globalang)>>(11-pskybits));
				fx = x0;
				do
				{
					globalpicnum = pskyoff[y&((1<<pskybits)-1)]+i;
					guo = gdo*(t*((double)(globalang-(y<<(11-pskybits))))/2048.0 + (double)sec->floorxpanning) - gux*ghalfx;
					y++;
					ox = fx; fx = ((double)((y<<(11-pskybits))-globalang))*oz+ghalfx;
					if (fx > x1) { fx = x1; i = -1; }

					pow2xsplit = 0; domost(ox,(ox-x0)*r+fy0,fx,(fx-x0)*r+fy0); //flor
				} while (i >= 0);

			}
#ifdef USE_OPENGL
			if (rendmode == 3)
			{
				skyclamphack = 0;
				glskyshade = 0.f;
				if (!nofog) {
				glEnable(GL_FOG);
				fog_is_on = 1;
				//glFogf(GL_FOG_DENSITY,((float)globalvisibility)*((float)((unsigned char)(sec->visibility+16)))*FOGSCALE);
				}
			}
#endif
			domostlayers = METH_LAYERS;
		}

		globalpicnum = sec->ceilingpicnum; globalshade = sec->ceilingshade; globalpal = (int32_t)((unsigned char)sec->ceilingpal);
		globalorientation = sec->ceilingstat;
		if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,sectnum);
		if (!(globalorientation&1))
		{
			if (!(globalorientation&64))
				{ ft[0] = globalposx; ft[1] = globalposy; ft[2] = cosglobalang; ft[3] = singlobalang; }
			else
			{
					//relative alignment
				fx = (double)(wall[wall[sec->wallptr].point2].x-wall[sec->wallptr].x);
				fy = (double)(wall[wall[sec->wallptr].point2].y-wall[sec->wallptr].y);
				r = 1.0/sqrt(fx*fx+fy*fy); fx *= r; fy *= r;
				ft[2] = cosglobalang*fx + singlobalang*fy;
				ft[3] = singlobalang*fx - cosglobalang*fy;
				ft[0] = ((double)(globalposx-wall[sec->wallptr].x))*fx + ((double)(globalposy-wall[sec->wallptr].y))*fy;
				ft[1] = ((double)(globalposy-wall[sec->wallptr].y))*fx - ((double)(globalposx-wall[sec->wallptr].x))*fy;
				if (!(globalorientation&4)) globalorientation ^= 32; else globalorientation ^= 16;
			}
			gdx = 0;
			gdy = gxyaspect;
			if (!(globalorientation&2)) gdy /= (double)(sec->ceilingz-globalposz);
			gdo = -ghoriz*gdy;
			if (globalorientation&8) { ft[0] /= 8; ft[1] /= -8; ft[2] /= 2097152; ft[3] /= 2097152; }
									  else { ft[0] /= 16; ft[1] /= -16; ft[2] /= 4194304; ft[3] /= 4194304; }
			gux = (double)ft[3]*((double)viewingrange)/-65536.0;
			gvx = (double)ft[2]*((double)viewingrange)/-65536.0;
			guy = (double)ft[0]*gdy; gvy = (double)ft[1]*gdy;
			guo = (double)ft[0]*gdo; gvo = (double)ft[1]*gdo;
			guo += (double)(ft[2]-gux)*ghalfx;
			gvo -= (double)(ft[3]+gvx)*ghalfx;

				//Texture flipping
			if (globalorientation&4)
			{
				r = gux; gux = gvx; gvx = r;
				r = guy; guy = gvy; gvy = r;
				r = guo; guo = gvo; gvo = r;
			}
			if (globalorientation&16) { gux = -gux; guy = -guy; guo = -guo; }
			if (globalorientation&32) { gvx = -gvx; gvy = -gvy; gvo = -gvo; }

				//Texture panning
			fx = (float)sec->ceilingxpanning*((float)(1<<(picsiz[globalpicnum]&15)))/256.0;
			fy = (float)sec->ceilingypanning*((float)(1<<(picsiz[globalpicnum]>>4)))/256.0;
			if ((globalorientation&(2+64)) == (2+64)) //Hack for panning for slopes w/ relative alignment
			{
				r = (float)sec->ceilingheinum / 4096.0; r = 1.0/sqrt(r*r+1);
				if (!(globalorientation&4)) fy *= r; else fx *= r;
			}
			guy += gdy*fx; guo += gdo*fx;
			gvy += gdy*fy; gvo += gdo*fy;

			if (globalorientation&2) //slopes
			{
				px[0] = x0; py[0] = ryp0 + ghoriz;
				px[1] = x1; py[1] = ryp1 + ghoriz;

					//Pick some point guaranteed to be not collinear to the 1st two points
				ox = nx0 + (ny1-ny0);
				oy = ny0 + (nx0-nx1);
				ox2 = (double)(oy-globalposy)*gcosang  - (double)(ox-globalposx)*gsinang ;
				oy2 = (double)(ox-globalposx)*gcosang2 + (double)(oy-globalposy)*gsinang2;
				oy2 = 1.0/oy2;
				px[2] = ghalfx*ox2*oy2 + ghalfx; oy2 *= gyxscale;
				py[2] = oy2 + ghoriz;

				for(i=0;i<3;i++)
				{
					dd[i] = px[i]*gdx + py[i]*gdy + gdo;
					uu[i] = px[i]*gux + py[i]*guy + guo;
					vv[i] = px[i]*gvx + py[i]*gvy + gvo;
				}

				py[0] = cy0;
				py[1] = cy1;
				py[2] = (getceilzofslope(sectnum,ox,oy)-globalposz)*oy2 + ghoriz;

				ox = py[1]-py[2]; oy = py[2]-py[0]; oz = py[0]-py[1];
				r = 1.0 / (ox*px[0] + oy*px[1] + oz*px[2]);
				gdx = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
				gux = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
				gvx = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
				ox = px[2]-px[1]; oy = px[0]-px[2]; oz = px[1]-px[0];
				gdy = (ox*dd[0] + oy*dd[1] + oz*dd[2])*r;
				guy = (ox*uu[0] + oy*uu[1] + oz*uu[2])*r;
				gvy = (ox*vv[0] + oy*vv[1] + oz*vv[2])*r;
				gdo = dd[0] - px[0]*gdx - py[0]*gdy;
				guo = uu[0] - px[0]*gux - py[0]*guy;
				gvo = vv[0] - px[0]*gvx - py[0]*gvy;

				if (globalorientation&64) //Hack for relative alignment on slopes
				{
					r = (float)sec->ceilingheinum / 4096.0;
					r = sqrt(r*r+1);
					if (!(globalorientation&4)) { gvx *= r; gvy *= r; gvo *= r; }
												  else { gux *= r; guy *= r; guo *= r; }
				}
			}
			pow2xsplit = 0; domost(x1,cy1,x0,cy0); //ceil
		}
		else if ((nextsectnum < 0) || (!(sector[nextsectnum].ceilingstat&1)))
		{
			domostlayers = 0;
#ifdef USE_OPENGL
			if (rendmode == 3)
			{
				glskyshade = polymost_skyshade(sec->visibility);
				if (!nofog) {
				glDisable(GL_FOG);
				fog_is_on = 0;
				//r = ((float)globalpisibility)*((float)((unsigned char)(sec->visibility+16)))*FOGSCALE;
				//r *= ((double)xdimscale*(double)viewingrange*gdo) / (65536.0*65536.0);
				//glFogf(GL_FOG_DENSITY,r);
				}

					//Use clamping for tiled sky textures
				for(i=(1<<pskybits)-1;i>0;i--)
					if (pskyoff[i] != pskyoff[i-1])
						{ skyclamphack = 1; break; }
			}
#endif
				//Parallaxing sky... hacked for Ken's mountain texture; paper-sky only :/
			if (1) //parallaxtype == 0)
			{
				dd[0] = (float)xdimen*.0000001; //Adjust sky depth based on screen size!
				t = (double)((1<<(picsiz[globalpicnum]&15))<<pskybits);
				vv[1] = dd[0]*((double)xdimscale*(double)viewingrange)/(65536.0*65536.0);
				vv[0] = dd[0]*((double)((tilesizy[globalpicnum]>>1)+parallaxyoffs)) - vv[1]*ghoriz;
				i = (1<<(picsiz[globalpicnum]>>4)); if (i != tilesizy[globalpicnum]) i += i;
				vv[0] += dd[0]*((double)sec->ceilingypanning)*((double)i)/256.0;

					//Hack to draw black rectangle below sky when looking down...
				gdx = 0; gdy = gxyaspect / -262144.0; gdo = -ghoriz*gdy;
				gux = 0; guy = 0; guo = 0;
				gvx = 0; gvy = 0; gvo = 0;
				oy = -vv[0]/vv[1];
				if ((oy < cy0) && (oy < cy1)) domost(x1,oy,x0,oy);
				else if ((oy < cy0) != (oy < cy1))
				{     //         cy1        cy0
						//        /             \
						//oy----------      oy---------
						//    /                    \
						//  cy0                     cy1
					ox = (oy-cy0)*(x1-x0)/(cy1-cy0) + x0;
					if (oy < cy0) { domost(ox,oy,x0,oy); domost(x1,cy1,ox,oy); }
								else { domost(ox,oy,x0,cy0); domost(x1,oy,ox,oy); }
				} else domost(x1,cy1,x0,cy0);


				gdx = 0; gdy = 0; gdo = dd[0];
				gux = gdo*(t*((double)xdimscale)*((double)yxaspect)*((double)viewingrange))/(16384.0*65536.0*65536.0*5.0*1024.0);
				guy = 0; //guo calculated later
				gvx = 0; gvy = vv[1]; gvo = vv[0];

				i = globalpicnum; r = (cy1-cy0)/(x1-x0); //slope of line
				oy = ((double)viewingrange)/(ghalfx*256.0); oz = 1/oy;

				y = ((((int32_t)((x0-ghalfx)*oy))+globalang)>>(11-pskybits));
				fx = x0;
				do
				{
					globalpicnum = pskyoff[y&((1<<pskybits)-1)]+i;
					guo = gdo*(t*((double)(globalang-(y<<(11-pskybits))))/2048.0 + (double)sec->ceilingxpanning) - gux*ghalfx;
					y++;
					ox = fx; fx = ((double)((y<<(11-pskybits))-globalang))*oz+ghalfx;
					if (fx > x1) { fx = x1; i = -1; }
					pow2xsplit = 0; domost(fx,(fx-x0)*r+cy0,ox,(ox-x0)*r+cy0); //ceil
				} while (i >= 0);
			}
#ifdef USE_OPENGL
			if (rendmode == 3)
			{
				skyclamphack = 0;
				glskyshade = 0.f;
				if (!nofog) {
				glEnable(GL_FOG);
				fog_is_on = 1;
				//glFogf(GL_FOG_DENSITY,((float)globalvisibility)*((float)((unsigned char)(sec->visibility+16)))*FOGSCALE);
				}
			}
#endif
			domostlayers = METH_LAYERS;
		}

			//(x0,cy0) == (u=             0,v=0,d=)
			//(x1,cy0) == (u=wal->xrepeat*8,v=0)
			//(x0,fy0) == (u=             0,v=v)
			//             u = (gux*sx + guy*sy + guo) / (gdx*sx + gdy*sy + gdo)
			//             v = (gvx*sx + gvy*sy + gvo) / (gdx*sx + gdy*sy + gdo)
			//             0 = (gux*x0 + guy*cy0 + guo) / (gdx*x0 + gdy*cy0 + gdo)
			//wal->xrepeat*8 = (gux*x1 + guy*cy0 + guo) / (gdx*x1 + gdy*cy0 + gdo)
			//             0 = (gvx*x0 + gvy*cy0 + gvo) / (gdx*x0 + gdy*cy0 + gdo)
			//             v = (gvx*x0 + gvy*fy0 + gvo) / (gdx*x0 + gdy*fy0 + gdo)
			//sx = x0, u = t0*wal->xrepeat*8, d = yp0;
			//sx = x1, u = t1*wal->xrepeat*8, d = yp1;
			//d = gdx*sx + gdo
			//u = (gux*sx + guo) / (gdx*sx + gdo)
			//yp0 = gdx*x0 + gdo
			//yp1 = gdx*x1 + gdo
			//t0*wal->xrepeat*8 = (gux*x0 + guo) / (gdx*x0 + gdo)
			//t1*wal->xrepeat*8 = (gux*x1 + guo) / (gdx*x1 + gdo)
			//gdx*x0 + gdo = yp0
			//gdx*x1 + gdo = yp1
		gdx = (ryp0-ryp1)*gxyaspect / (x0-x1);
		gdy = 0;
		gdo = ryp0*gxyaspect - gdx*x0;

			//gux*x0 + guo = t0*wal->xrepeat*8*yp0
			//gux*x1 + guo = t1*wal->xrepeat*8*yp1
		gux = (t0*ryp0 - t1*ryp1)*gxyaspect*(float)wal->xrepeat*8.f / (x0-x1);
		guo = t0*ryp0*gxyaspect*(float)wal->xrepeat*8.f - gux*x0;
		guo += (float)wal->xpanning*gdo;
		gux += (float)wal->xpanning*gdx;
		guy = 0;
			//Derivation for u:
			//   (gvx*x0 + gvy*cy0 + gvo) / (gdx*x0 + gdy*cy0 + gdo) = 0
			//   (gvx*x1 + gvy*cy1 + gvo) / (gdx*x1 + gdy*cy1 + gdo) = 0
			//   (gvx*x0 + gvy*fy0 + gvo) / (gdx*x0 + gdy*fy0 + gdo) = v
			//   (gvx*x1 + gvy*fy1 + gvo) / (gdx*x1 + gdy*fy1 + gdo) = v
			//   (gvx*x0 + gvy*cy0 + gvo*1) = 0
			//   (gvx*x1 + gvy*cy1 + gvo*1) = 0
			//   (gvx*x0 + gvy*fy0 + gvo*1) = t
		ogux = gux; oguy = guy; oguo = guo;

		if (nextsectnum >= 0)
		{
			getzsofslope(nextsectnum,nx0,ny0,&cz,&fz);
			ocy0 = ((float)(cz-globalposz))*ryp0 + ghoriz;
			ofy0 = ((float)(fz-globalposz))*ryp0 + ghoriz;
			getzsofslope(nextsectnum,nx1,ny1,&cz,&fz);
			ocy1 = ((float)(cz-globalposz))*ryp1 + ghoriz;
			ofy1 = ((float)(fz-globalposz))*ryp1 + ghoriz;

			if ((wal->cstat&48) == 16) maskwall[maskwallcnt++] = z;

			if (((cy0 < ocy0) || (cy1 < ocy1)) && (!((sec->ceilingstat&sector[nextsectnum].ceilingstat)&1)))
			{
				globalpicnum = wal->picnum; globalshade = wal->shade; globalpal = (int32_t)((unsigned char)wal->pal);
				if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,wallnum+16384);

				if (!(wal->cstat&4)) i = sector[nextsectnum].ceilingz; else i = sec->ceilingz;
				t0 = ((float)(i-globalposz))*ryp0 + ghoriz;
				t1 = ((float)(i-globalposz))*ryp1 + ghoriz;
				t = ((gdx*x0 + gdo) * (float)wal->yrepeat) / ((x1-x0) * ryp0 * 2048.f);
				i = (1<<(picsiz[globalpicnum]>>4)); if (i < tilesizy[globalpicnum]) i <<= 1;
				fy = (float)wal->ypanning * ((float)i) / 256.0;
				gvx = (t0-t1)*t;
				gvy = (x1-x0)*t;
				gvo = -gvx*x0 - gvy*t0 + fy*gdo; gvx += fy*gdx; gvy += fy*gdy;

				if (wal->cstat&8) //xflip
				{
					t = (float)(wal->xrepeat*8 + wal->xpanning*2);
					gux = gdx*t - gux;
					guy = gdy*t - guy;
					guo = gdo*t - guo;
				}
				if (wal->cstat&256) { gvx = -gvx; gvy = -gvy; gvo = -gvo; } //yflip

				pow2xsplit = 1; domost(x1,ocy1,x0,ocy0);

				if (wal->cstat&8) { gux = ogux; guy = oguy; guo = oguo; }
			}
			if (((ofy0 < fy0) || (ofy1 < fy1)) && (!((sec->floorstat&sector[nextsectnum].floorstat)&1)))
			{
				if (!(wal->cstat&2)) nwal = wal;
				else
				{
					nwal = &wall[wal->nextwall];
					guo += (float)(nwal->xpanning-wal->xpanning)*gdo;
					gux += (float)(nwal->xpanning-wal->xpanning)*gdx;
					guy += (float)(nwal->xpanning-wal->xpanning)*gdy;
				}
				globalpicnum = nwal->picnum; globalshade = nwal->shade; globalpal = (int32_t)((unsigned char)nwal->pal);
				if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,wallnum+16384);

				if (!(nwal->cstat&4)) i = sector[nextsectnum].floorz; else i = sec->ceilingz;
				t0 = ((float)(i-globalposz))*ryp0 + ghoriz;
				t1 = ((float)(i-globalposz))*ryp1 + ghoriz;
				t = ((gdx*x0 + gdo) * (float)wal->yrepeat) / ((x1-x0) * ryp0 * 2048.f);
				i = (1<<(picsiz[globalpicnum]>>4)); if (i < tilesizy[globalpicnum]) i <<= 1;
				fy = (float)nwal->ypanning * ((float)i) / 256.0;
				gvx = (t0-t1)*t;
				gvy = (x1-x0)*t;
				gvo = -gvx*x0 - gvy*t0 + fy*gdo; gvx += fy*gdx; gvy += fy*gdy;

				if (wal->cstat&8) //xflip
				{
					t = (float)(wal->xrepeat*8 + nwal->xpanning*2);
					gux = gdx*t - gux;
					guy = gdy*t - guy;
					guo = gdo*t - guo;
				}
				if (nwal->cstat&256) { gvx = -gvx; gvy = -gvy; gvo = -gvo; } //yflip

				pow2xsplit = 1; domost(x0,ofy0,x1,ofy1);

				if (wal->cstat&(2+8)) { guo = oguo; gux = ogux; guy = oguy; }
			}
		}

		if ((nextsectnum < 0) || (wal->cstat&32))   //White/1-way wall
		{
			if (nextsectnum < 0) globalpicnum = wal->picnum; else globalpicnum = wal->overpicnum;
			globalshade = wal->shade; globalpal = (int32_t)((unsigned char)wal->pal);
			if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,wallnum+16384);

			if (nextsectnum >= 0) { if (!(wal->cstat&4)) i = nextsec->ceilingz; else i = sec->ceilingz; }
								  else { if (!(wal->cstat&4)) i = sec->ceilingz;     else i = sec->floorz; }
			t0 = ((float)(i-globalposz))*ryp0 + ghoriz;
			t1 = ((float)(i-globalposz))*ryp1 + ghoriz;
			t = ((gdx*x0 + gdo) * (float)wal->yrepeat) / ((x1-x0) * ryp0 * 2048.f);
			i = (1<<(picsiz[globalpicnum]>>4)); if (i < tilesizy[globalpicnum]) i <<= 1;
			fy = (float)wal->ypanning * ((float)i) / 256.0;
			gvx = (t0-t1)*t;
			gvy = (x1-x0)*t;
			gvo = -gvx*x0 - gvy*t0 + fy*gdo; gvx += fy*gdx; gvy += fy*gdy;

			if (wal->cstat&8) //xflip
			{
				t = (float)(wal->xrepeat*8 + wal->xpanning*2);
				gux = gdx*t - gux;
				guy = gdy*t - guy;
				guo = gdo*t - guo;
			}
			if (wal->cstat&256) { gvx = -gvx; gvy = -gvy; gvo = -gvo; } //yflip
			pow2xsplit = 1; domost(x0,-10000,x1,-10000);
		}

		if (nextsectnum >= 0)
			if ((!(gotsector[nextsectnum>>3]&pow2char[nextsectnum&7])) && (testvisiblemost(x0,x1)))
				polymost_scansector(nextsectnum);
	}
}

static int32_t polymost_bunchfront (int32_t b1, int32_t b2)
{
	double x1b1, x1b2, x2b1, x2b2;
	int32_t b1f, b2f, i;

	b1f = bunchfirst[b1]; x1b1 = dxb1[b1f]; x2b2 = dxb2[bunchlast[b2]]; if (x1b1 >= x2b2) return(-1);
	b2f = bunchfirst[b2]; x1b2 = dxb1[b2f]; x2b1 = dxb2[bunchlast[b1]]; if (x1b2 >= x2b1) return(-1);

	if (x1b1 >= x1b2)
	{
		for(i=b2f;dxb2[i]<=x1b1;i=p2[i]);
		return(wallfront(b1f,i));
	}
	for(i=b1f;dxb2[i]<=x1b2;i=p2[i]);
	return(wallfront(i,b2f));
}

static void polymost_scansector (int32_t sectnum)
{
	double d, xp1, yp1, xp2, yp2;
	walltype *wal, *wal2;
	spritetype *spr;
	int32_t z, zz, startwall, endwall, numscansbefore, scanfirst, bunchfrst, nextsectnum;
	int32_t xs, ys, x1, y1, x2, y2;

	if (sectnum < 0) return;
	if (automapping) show2dsector[sectnum>>3] |= pow2char[sectnum&7];

	sectorborder[0] = sectnum, sectorbordercnt = 1;
	do
	{
		sectnum = sectorborder[--sectorbordercnt];

		for(z=headspritesect[sectnum];z>=0;z=nextspritesect[z])
		{
			spr = &sprite[z];
			if ((((spr->cstat&0x8000) == 0) || (showinvisibility)) &&
				  (spr->xrepeat > 0) && (spr->yrepeat > 0) &&
				  (spritesortcnt < MAXSPRITESONSCREEN))
			{
				xs = spr->x-globalposx; ys = spr->y-globalposy;
				if ((spr->cstat&48) || (xs*gcosang+ys*gsinang > 0))
				{
					copybufbyte(spr,&tsprite[spritesortcnt],sizeof(spritetype));
					tsprite[spritesortcnt++].owner = z;
				}
			}
		}

		gotsector[sectnum>>3] |= pow2char[sectnum&7];

		bunchfrst = numbunches;
		numscansbefore = numscans;

		startwall = sector[sectnum].wallptr; endwall = sector[sectnum].wallnum+startwall;
		scanfirst = numscans;
		xp2 = 0; yp2 = 0;
		for(z=startwall,wal=&wall[z];z<endwall;z++,wal++)
		{
			wal2 = &wall[wal->point2];
			x1 = wal->x-globalposx; y1 = wal->y-globalposy;
			x2 = wal2->x-globalposx; y2 = wal2->y-globalposy;

			nextsectnum = wal->nextsector; //Scan close sectors
			if ((nextsectnum >= 0) && (!(wal->cstat&32)) && (!(gotsector[nextsectnum>>3]&pow2char[nextsectnum&7])))
			{
				d = (double)x1*(double)y2 - (double)x2*(double)y1; xp1 = (double)(x2-x1); yp1 = (double)(y2-y1);
				if (d*d <= (xp1*xp1 + yp1*yp1)*(SCISDIST*SCISDIST*260.0))
					sectorborder[sectorbordercnt++] = nextsectnum;
			}

			if ((z == startwall) || (wall[z-1].point2 != z))
			{
				xp1 = ((double)y1*(double)cosglobalang             - (double)x1*(double)singlobalang            )/64.0;
				yp1 = ((double)x1*(double)cosviewingrangeglobalang + (double)y1*(double)sinviewingrangeglobalang)/64.0;
			}
			else { xp1 = xp2; yp1 = yp2; }
			xp2 = ((double)y2*(double)cosglobalang             - (double)x2*(double)singlobalang            )/64.0;
			yp2 = ((double)x2*(double)cosviewingrangeglobalang + (double)y2*(double)sinviewingrangeglobalang)/64.0;
			if ((yp1 >= SCISDIST) || (yp2 >= SCISDIST))
				if ((double)xp1*(double)yp2 < (double)xp2*(double)yp1) //if wall is facing you...
				{
					if (yp1 >= SCISDIST)
						  dxb1[numscans] = (double)xp1*ghalfx/(double)yp1 + ghalfx;
					else dxb1[numscans] = -1e32;

					if (yp2 >= SCISDIST)
						  dxb2[numscans] = (double)xp2*ghalfx/(double)yp2 + ghalfx;
					else dxb2[numscans] = 1e32;

					if (dxb1[numscans] < dxb2[numscans])
						{ thesector[numscans] = sectnum; thewall[numscans] = z; p2[numscans] = numscans+1; numscans++; }
				}

			if ((wall[z].point2 < z) && (scanfirst < numscans))
				{ p2[numscans-1] = scanfirst; scanfirst = numscans; }
		}

		for(z=numscansbefore;z<numscans;z++)
			if ((wall[thewall[z]].point2 != thewall[p2[z]]) || (dxb2[z] > dxb1[p2[z]]))
				{ bunchfirst[numbunches++] = p2[z]; p2[z] = -1; }

		for(z=bunchfrst;z<numbunches;z++)
		{
			for(zz=bunchfirst[z];p2[zz]>=0;zz=p2[zz]);
			bunchlast[z] = zz;
		}
	} while (sectorbordercnt > 0);
}

void polymost_drawrooms ()
{
	int32_t i, j, k, n, n2, closest;
	double ox, oy, oz, ox2, oy2, oz2, r, px[6], py[6], pz[6], px2[6], py2[6], pz2[6], sx[6], sy[6];

	if (!rendmode) return;

	begindrawing();
	frameoffset = frameplace + windowy1*bytesperline + windowx1;


#ifdef USE_OPENGL
	if (rendmode == 3)
	{
		resizeglcheck();
		_glframedrawn();
		polymost_beginframe_shader(1); //GL 2.1: into the index framebuffer
		if ((!glshbound) && (!glffcleared)) //1.1/1.5: once a frame, as the shader path
			{ glClear(GL_DEPTH_BUFFER_BIT); glffcleared = 1; }

		//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		//glEnable(GL_TEXTURE_2D);
		//glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE); //default anyway
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_ALWAYS); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS

		//glPolygonOffset(1,1); //Supposed to make sprites pasted on walls or floors not disappear
		glDepthRange(0.00001,1.0); //<- this is more widely supported than glPolygonOffset

		 //Enable this for OpenGL red-blue glasses mode :)
		if (glredbluemode)
		{
			float m[4][4];
			static int grbfcnt = 0; grbfcnt++;
			if (redblueclearcnt < numpages) { redblueclearcnt++; glColorMask(1,1,1,1); glClear(GL_COLOR_BUFFER_BIT); }
			if (grbfcnt&1)
			{
				glViewport(windowx1-16,yres-(windowy2+1),windowx2-(windowx1-16)+1,windowy2-windowy1+1);
				glColorMask(1,0,0,1);
				globalposx += (float)singlobalang/1024.0;
				globalposy -= (float)cosglobalang/1024.0;
			}
			else
			{
				glViewport(windowx1,yres-(windowy2+1),windowx2+16-windowx1+1,windowy2-windowy1+1);
				glColorMask(0,1,1,1);
				globalposx -= (float)singlobalang/1024.0;
				globalposy += (float)cosglobalang/1024.0;
			}
		}
	}
#endif

		//Polymost supports true look up/down :) Here, we convert horizon to angle.
		//gchang&gshang are cos&sin of this angle (respectively)
	gyxscale = ((double)xdimenscale)/131072.0;
	gxyaspect = ((double)xyaspect*(double)viewingrange)*(5.0/(65536.0*262144.0));
	gviewxrange = ((double)viewingrange)*((double)xdimen)/(32768.0*1024.0);
	gcosang = ((double)cosglobalang)/262144.0;
	gsinang = ((double)singlobalang)/262144.0;
	gcosang2 = gcosang*((double)viewingrange)/65536.0;
	gsinang2 = gsinang*((double)viewingrange)/65536.0;
	ghalfx = (double)halfxdimen; grhalfxdown10 = 1.0/(((double)ghalfx)*1024);
	ghoriz = (double)globalhoriz;

		//global cos/sin height angle
	r = (double)((ydimen>>1)-ghoriz);
	gshang = r/sqrt(r*r+ghalfx*ghalfx);
	gchang = sqrt(1.0-gshang*gshang);
	ghoriz = (double)(ydimen>>1);

	  //global cos/sin tilt angle
	gctang = cos(gtang);
	gstang = sin(gtang);
	if (fabs(gstang) < .001) //This hack avoids nasty precision bugs in domost()
		{ gstang = 0; if (gctang > 0) gctang = 1.0; else gctang = -1.0; }

	if (inpreparemirror)
		gstang = -gstang;

		//Generate viewport trapezoid (for handling screen up/down)
	px[0] = px[3] = 0-1; px[1] = px[2] = windowx2+1-windowx1+2;
	py[0] = py[1] = 0-1; py[2] = py[3] = windowy2+1-windowy1+2; n = 4;
	for(i=0;i<n;i++)
	{
		ox = px[i]-ghalfx; oy = py[i]-ghoriz; oz = ghalfx;

			//Tilt rotation (backwards)
		ox2 = ox*gctang + oy*gstang;
		oy2 = oy*gctang - ox*gstang;
		oz2 = oz;

			//Up/down rotation (backwards)
		px[i] = ox2;
		py[i] = oy2*gchang + oz2*gshang;
		pz[i] = oz2*gchang - oy2*gshang;
	}

		//Clip to SCISDIST plane
	n2 = 0;
	for(i=0;i<n;i++)
	{
		j = i+1; if (j >= n) j = 0;
		if (pz[i] >= SCISDIST) { px2[n2] = px[i]; py2[n2] = py[i]; pz2[n2] = pz[i]; n2++; }
		if ((pz[i] >= SCISDIST) != (pz[j] >= SCISDIST))
		{
			r = (SCISDIST-pz[i])/(pz[j]-pz[i]);
			px2[n2] = (px[j]-px[i])*r + px[i];
			py2[n2] = (py[j]-py[i])*r + py[i];
			pz2[n2] = SCISDIST; n2++;
		}
	}
	if (n2 < 3) { enddrawing(); return; }
	for(i=0;i<n2;i++)
	{
		r = ghalfx / pz2[i];
		sx[i] = px2[i]*r + ghalfx;
		sy[i] = py2[i]*r + ghoriz;
	}
	initmosts(sx,sy,n2);

	if (searchit == 2)
	{
		short hitsect, hitwall, hitsprite;
		int32_t vx, vy, vz, hitx, hity, hitz;

		ox2 = searchx-ghalfx; oy2 = searchy-ghoriz; oz2 = ghalfx;

			//Tilt rotation
		ox = ox2*gctang + oy2*gstang;
		oy = oy2*gctang - ox2*gstang;
		oz = oz2;

			//Up/down rotation
		ox2 = oz*gchang - oy*gshang;
		oy2 = ox;
		oz2 = oy*gchang + oz*gshang;

			//Standard Left/right rotation
		vx = ox2*((float)cosglobalang) - oy2*((float)singlobalang);
		vy = ox2*((float)singlobalang) + oy2*((float)cosglobalang);
		vz = oz2*16384.0;

		hitallsprites = 1;
		hitscan(globalposx,globalposy,globalposz,globalcursectnum, //Start position
			vx>>12,vy>>12,vz>>8,&hitsect,&hitwall,&hitsprite,&hitx,&hity,&hitz,0xffff0030);
		hitallsprites = 0;

		searchsector = hitsect;
		if (hitwall >= 0)
		{
			searchwall = hitwall; searchstat = 0;
			if (wall[hitwall].nextwall >= 0)
			{
				int32_t cz, fz;
				getzsofslope(wall[hitwall].nextsector,hitx,hity,&cz,&fz);
				if (hitz > fz)
				{
					if (wall[hitwall].cstat&2) //'2' bottoms of walls
						searchwall = wall[hitwall].nextwall;
				}
				else if ((hitz > cz) && (wall[hitwall].cstat&(16+32))) //masking or 1-way
					searchstat = 4;
			}
		}
		else if (hitsprite >= 0) { searchwall = hitsprite; searchstat = 3; }
		else
		{
			int32_t cz, fz;
			getzsofslope(hitsect,hitx,hity,&cz,&fz);
			if ((hitz<<1) < cz+fz) searchstat = 1; else searchstat = 2;
			//if (vz < 0) searchstat = 1; else searchstat = 2; //Won't work for slopes :/
		}
		searchit = 0;
	}

	numscans = numbunches = 0;

	if (globalcursectnum >= MAXSECTORS)
		globalcursectnum -= MAXSECTORS;
	else
	{
		i = globalcursectnum;
		updatesector(globalposx,globalposy,&globalcursectnum);
		if (globalcursectnum < 0) globalcursectnum = i;
	}

	polymost_scansector(globalcursectnum);

	if (inpreparemirror)
	{
		grhalfxdown10x = -grhalfxdown10;
		inpreparemirror = 0;
		polymost_drawalls(0);
		numbunches--;
		bunchfirst[0] = bunchfirst[numbunches];
		bunchlast[0] = bunchlast[numbunches];
	}
	else
		grhalfxdown10x = grhalfxdown10;

	while (numbunches > 0)
	{
		memset(tempbuf,0,numbunches+3); tempbuf[0] = 1;

		closest = 0;              //Almost works, but not quite :(
		for(i=1;i<numbunches;i++)
		{
			j = polymost_bunchfront(i,closest); if (j < 0) continue;
			tempbuf[i] = 1;
			if (!j) { tempbuf[closest] = 1; closest = i; }
		}
		for(i=0;i<numbunches;i++) //Double-check
		{
			if (tempbuf[i]) continue;
			j = polymost_bunchfront(i,closest); if (j < 0) continue;
			tempbuf[i] = 1;
			if (!j) { tempbuf[closest] = 1; closest = i; i = 0; }
		}

		polymost_drawalls(closest);

		numbunches--;
		bunchfirst[closest] = bunchfirst[numbunches];
		bunchlast[closest] = bunchlast[numbunches];
	}
#ifdef USE_OPENGL
	if (rendmode == 3)
	{
		glDepthFunc(GL_LEQUAL); //NEVER,LESS,(,L)EQUAL,GREATER,(NOT,G)EQUAL,ALWAYS

		//glPolygonOffset(0,0);
		glDepthRange(0.0,0.99999); //<- this is more widely supported than glPolygonOffset
	}
#endif

	enddrawing();
}

void polymost_drawmaskwall (int32_t damaskwallcnt)
{
	double dpx[8], dpy[8], dpx2[8], dpy2[8];
	float fx, fy, x0, x1, sx0, sy0, sx1, sy1, xp0, yp0, xp1, yp1, oxp0, oyp0, ryp0, ryp1;
	float f, r, t, t0, t1, nx0, ny0, nx1, ny1, py[4], csy[4], fsy[4];
	int32_t i, j, k, n, n2, x, z, sectnum, z1, z2, lx, rx, cz[4], fz[4], method;
	sectortype *sec, *nsec;
	walltype *wal, *wal2;

	z = maskwall[damaskwallcnt];
	wal = &wall[thewall[z]]; wal2 = &wall[wal->point2];
	sectnum = thesector[z]; sec = &sector[sectnum];
	nsec = &sector[wal->nextsector];
	z1 = max(nsec->ceilingz,sec->ceilingz);
	z2 = min(nsec->floorz,sec->floorz);

	globalpicnum = wal->overpicnum; if ((uint32_t)globalpicnum >= MAXTILES) globalpicnum = 0;
	if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,(short)thewall[z]+16384);
	globalshade = (int32_t)wal->shade;
	globalpal = (int32_t)((unsigned char)wal->pal);
	globalorientation = (int32_t)wal->cstat;

	sx0 = (float)(wal->x-globalposx); sx1 = (float)(wal2->x-globalposx);
	sy0 = (float)(wal->y-globalposy); sy1 = (float)(wal2->y-globalposy);
	yp0 = sx0*gcosang2 + sy0*gsinang2;
	yp1 = sx1*gcosang2 + sy1*gsinang2;
	if ((yp0 < SCISDIST) && (yp1 < SCISDIST)) return;
	xp0 = sy0*gcosang - sx0*gsinang;
	xp1 = sy1*gcosang - sx1*gsinang;

		//Clip to close parallel-screen plane
	oxp0 = xp0; oyp0 = yp0;
	if (yp0 < SCISDIST) { t0 = (SCISDIST-yp0)/(yp1-yp0); xp0 = (xp1-xp0)*t0+xp0; yp0 = SCISDIST; }
						  else t0 = 0.f;
	if (yp1 < SCISDIST) { t1 = (SCISDIST-oyp0)/(yp1-oyp0); xp1 = (xp1-oxp0)*t1+oxp0; yp1 = SCISDIST; }
						else { t1 = 1.f; }

	getzsofslope(sectnum,(wal2->x-wal->x)*t0+wal->x,(wal2->y-wal->y)*t0+wal->y,&cz[0],&fz[0]);
	getzsofslope(wal->nextsector,(wal2->x-wal->x)*t0+wal->x,(wal2->y-wal->y)*t0+wal->y,&cz[1],&fz[1]);
	getzsofslope(sectnum,(wal2->x-wal->x)*t1+wal->x,(wal2->y-wal->y)*t1+wal->y,&cz[2],&fz[2]);
	getzsofslope(wal->nextsector,(wal2->x-wal->x)*t1+wal->x,(wal2->y-wal->y)*t1+wal->y,&cz[3],&fz[3]);

	ryp0 = 1.f/yp0; ryp1 = 1.f/yp1;

		//Generate screen coordinates for front side of wall
	x0 = ghalfx*xp0*ryp0 + ghalfx;
	x1 = ghalfx*xp1*ryp1 + ghalfx;
	if (x1 <= x0) return;

	ryp0 *= gyxscale; ryp1 *= gyxscale;

	gdx = (ryp0-ryp1)*gxyaspect / (x0-x1);
	gdy = 0;
	gdo = ryp0*gxyaspect - gdx*x0;

		//gux*x0 + guo = t0*wal->xrepeat*8*yp0
		//gux*x1 + guo = t1*wal->xrepeat*8*yp1
	gux = (t0*ryp0 - t1*ryp1)*gxyaspect*(float)wal->xrepeat*8.f / (x0-x1);
	guo = t0*ryp0*gxyaspect*(float)wal->xrepeat*8.f - gux*x0;
	guo += (float)wal->xpanning*gdo;
	gux += (float)wal->xpanning*gdx;
	guy = 0;

	if (!(wal->cstat&4)) i = z1; else i = z2;
	i -= globalposz;
	t0 = ((float)i)*ryp0 + ghoriz;
	t1 = ((float)i)*ryp1 + ghoriz;
	t = ((gdx*x0 + gdo) * (float)wal->yrepeat) / ((x1-x0) * ryp0 * 2048.f);
	i = (1<<(picsiz[globalpicnum]>>4)); if (i < tilesizy[globalpicnum]) i <<= 1;
	fy = (float)wal->ypanning * ((float)i) / 256.0;
	gvx = (t0-t1)*t;
	gvy = (x1-x0)*t;
	gvo = -gvx*x0 - gvy*t0 + fy*gdo; gvx += fy*gdx; gvy += fy*gdy;

	if (wal->cstat&8) //xflip
	{
		t = (float)(wal->xrepeat*8 + wal->xpanning*2);
		gux = gdx*t - gux;
		guy = gdy*t - guy;
		guo = gdo*t - guo;
	}
	if (wal->cstat&256) { gvx = -gvx; gvy = -gvy; gvo = -gvo; } //yflip

	method = 1; pow2xsplit = 1;
	if (wal->cstat&128) { if (!(wal->cstat&512)) method = 2; else method = 3; }

#ifdef USE_OPENGL
	if (rendmode == 3) polymost_beginframe_shader(0); //after drawrooms', not cleared
	if (!nofog) {
	if (rendmode == 3)
	{
		glEnable(GL_FOG); fog_is_on = 1; //setpolymost2dview turns it off
		polymost_fogvis(sec->visibility);
	}
	}
#endif

	for(i=0;i<2;i++)
	{
		csy[i] = ((float)(cz[i]-globalposz))*ryp0 + ghoriz;
		fsy[i] = ((float)(fz[i]-globalposz))*ryp0 + ghoriz;
		csy[i+2] = ((float)(cz[i+2]-globalposz))*ryp1 + ghoriz;
		fsy[i+2] = ((float)(fz[i+2]-globalposz))*ryp1 + ghoriz;
	}

		//Clip 2 quadrilaterals
		//               /csy3
		//             /   |
		// csy0------/----csy2
		//   |     /xxxxxxx|
		//   |   /xxxxxxxxx|
		// csy1/xxxxxxxxxxx|
		//   |xxxxxxxxxxx/fsy3
		//   |xxxxxxxxx/   |
		//   |xxxxxxx/     |
		// fsy0----/------fsy2
		//   |   /
		// fsy1/

	dpx[0] = x0; dpy[0] = csy[1];
	dpx[1] = x1; dpy[1] = csy[3];
	dpx[2] = x1; dpy[2] = fsy[3];
	dpx[3] = x0; dpy[3] = fsy[1];
	n = 4;

		//Clip to (x0,csy[0])-(x1,csy[2])
	n2 = 0; t1 = -((dpx[0]-x0)*(csy[2]-csy[0]) - (dpy[0]-csy[0])*(x1-x0));
	for(i=0;i<n;i++)
	{
		j = i+1; if (j >= n) j = 0;

		t0 = t1; t1 = -((dpx[j]-x0)*(csy[2]-csy[0]) - (dpy[j]-csy[0])*(x1-x0));
		if (t0 >= 0) { dpx2[n2] = dpx[i]; dpy2[n2] = dpy[i]; n2++; }
		if ((t0 >= 0) != (t1 >= 0))
		{
			r = t0/(t0-t1);
			dpx2[n2] = (dpx[j]-dpx[i])*r + dpx[i];
			dpy2[n2] = (dpy[j]-dpy[i])*r + dpy[i];
			n2++;
		}
	}
	if (n2 < 3) return;

		//Clip to (x1,fsy[2])-(x0,fsy[0])
	n = 0; t1 = -((dpx2[0]-x1)*(fsy[0]-fsy[2]) - (dpy2[0]-fsy[2])*(x0-x1));
	for(i=0;i<n2;i++)
	{
		j = i+1; if (j >= n2) j = 0;

		t0 = t1; t1 = -((dpx2[j]-x1)*(fsy[0]-fsy[2]) - (dpy2[j]-fsy[2])*(x0-x1));
		if (t0 >= 0) { dpx[n] = dpx2[i]; dpy[n] = dpy2[i]; n++; }
		if ((t0 >= 0) != (t1 >= 0))
		{
			r = t0/(t0-t1);
			dpx[n] = (dpx2[j]-dpx2[i])*r + dpx2[i];
			dpy[n] = (dpy2[j]-dpy2[i])*r + dpy2[i];
			n++;
		}
	}
	if (n < 3) return;

	drawpoly(dpx,dpy,n,method|METH_LAYERS);
}

	//Of the depths at which the rays through the columns xl..xr (xp at depth yp)
	//   of a face sprite's picture cross the wall a..b (projected), the least
	//   (most 0) or the greatest (most 1); -1 if none crosses it in front of the
	//   eye. Both are at the picture's ends or at the wall's ends.
static float polymost_wallcross (float ax, float ay, float bx, float by, float yp, float xl, float xr, int32_t most)
{
	float dx = bx-ax, dy = by-ay, s[4], den, lam, u, d = -1.f;
	int32_t k;

	s[0] = xl; s[1] = xr; s[2] = s[3] = xl;
	if (ay > SCISDIST) s[2] = ax*yp/ay;
	if (by > SCISDIST) s[3] = bx*yp/by;
	for(k=0;k<4;k++)
	{
		if ((s[k] < xl) || (s[k] > xr)) continue;
		den = s[k]*dy - yp*dx; if (fabs(den) < 1e-6) continue;
		lam = (ax*dy - ay*dx)/den;
		u = (ax*yp - ay*s[k])/den;
		if ((u < -1e-4) || (u > 1.0001) || (lam <= 0.f)) continue;
		if ((d < 0.f) || ((lam*yp > d) == (most != 0))) d = lam*yp;
	}
	return(d);
}

	//Classic clips a face sprite only by the walls in front of it (drawsprite's
	//   spritewallfront test), so one lying against a wall it's in front of draws
	//   whole, its flat picture reaching through the wall. Polymost draws it all at
	//   its middle's depth, where the z-buffer cuts it on that wall. This is the
	//   depth (polymost's, as yp) it has to come to over its columns xl..xr (xp at
	//   yp) to stay in front of each such wall of its sector or the ones next to it
	//   (facing the eye, it in front, an end beyond it: classic doesn't clip by it),
	//   but no nearer than any wall drawn this frame that classic does clip it by
	//   (facing the eye, it behind, an end nearer; a red wall only with a step,
	//   up or down, as its sector's floor or ceiling edge hides what's past it) so
	//   that those still hide it; yp if it needn't or can't come nearer.
static int spritewallfront (spritetype *s, int32_t w);
static float polymost_spritenear (spritetype *tspr, float yp, float xl, float xr)
{
	float a[4], d, nearest = yp;
	int32_t i, w, endw, sect, pass;

	for(pass=0;pass<2;pass++)
		for(i=(pass?0:-1);i<(pass?numsectors:sector[tspr->sectnum].wallnum);i++)
		{
			if (pass)
			{
				if (nearest >= yp) return(yp); //nothing to come in front of
				sect = i;
				if (!(gotsector[sect>>3]&pow2char[sect&7])) continue;
			}
			else
			{
				sect = tspr->sectnum;
				if (i >= 0) { sect = wall[sector[sect].wallptr+i].nextsector; if (sect < 0) continue; }
			}
			for(w=sector[sect].wallptr,endw=w+sector[sect].wallnum;w<endw;w++)
			{
				if (spritewallfront(tspr,w) == pass) continue;
				if ((pass) && (wall[w].nextsector >= 0) &&
					 (sector[wall[w].nextsector].floorz == sector[sect].floorz) &&
					 (sector[wall[w].nextsector].ceilingz == sector[sect].ceilingz) &&
					 (!((sector[wall[w].nextsector].floorstat|sector[sect].floorstat|
						 sector[wall[w].nextsector].ceilingstat|sector[sect].ceilingstat)&2)))
					continue; //no step either way, flat: hides nothing
				a[0] = (float)(wall[w].x-globalposx); a[1] = (float)(wall[w].y-globalposy);
				if ((float)(wall[wall[w].point2].x-wall[w].x)*-a[1] < (float)(wall[wall[w].point2].y-wall[w].y)*-a[0])
					continue; //its back to the eye: not drawn
				a[2] = (float)(wall[wall[w].point2].x-globalposx); a[3] = (float)(wall[wall[w].point2].y-globalposy);
				d = a[1]*gcosang - a[0]*gsinang; a[1] = a[0]*gcosang2 + a[1]*gsinang2; a[0] = d;
				d = a[3]*gcosang - a[2]*gsinang; a[3] = a[2]*gcosang2 + a[3]*gsinang2; a[2] = d;
				if (!pass)
				{
					if ((a[1] >= yp) == (a[3] >= yp)) continue; //all nearer (classic clips) or all beyond
					d = polymost_wallcross(a[0],a[1],a[2],a[3],yp,xl,xr,0);
					if ((d > 0.f) && (d < nearest)) nearest = d;
				}
				else
				{
					if ((a[1] >= yp) && (a[3] >= yp)) continue; //all beyond: classic doesn't clip by it
					d = polymost_wallcross(a[0],a[1],a[2],a[3],yp,xl,xr,1)*1.02f; //behind it, past the masks' depth range
					if (d > nearest) nearest = d;
					if (nearest >= yp) return(yp);
				}
			}
		}
		//No nearer than a quarter of its depth, nor than 8 (128 map units): the
		//   GL near plane is at about 2*xdimen/ydimen
	return(max(nearest,max(yp*.25f,8.f)));
}

void polymost_drawsprite (int32_t snum)
{
	double px[6], py[6];
	float f, r, c, s, fx, fy, sx0, sy0, sx1, sy1, xp0, yp0, xp1, yp1, oxp0, oyp0, ryp0, ryp1, ft[4];
	float x0, y0, x1, y1, sc0, sf0, sc1, sf1, px2[6], py2[6], xv, yv, t0, t1, fogshades = 0.f;
	int32_t i, j, spritenum, xoff, yoff, method, npoints, nearer;
	spritetype *tspr;

	tspr = tspriteptr[snum];
	globalpicnum      = tspr->picnum;
	globalshade       = tspr->shade;
	globalpal         = tspr->pal;
	globalorientation = tspr->cstat;
	spritenum         = tspr->owner;
	if (picanm[globalpicnum]&192) globalpicnum += animateoffs(globalpicnum,spritenum+32768);

	xoff = (int32_t)((signed char)((picanm[globalpicnum]>>8)&255))+((int32_t)tspr->xoffset);
	yoff = (int32_t)((signed char)((picanm[globalpicnum]>>16)&255))+((int32_t)tspr->yoffset);

	method = 1+4;
	if (tspr->cstat&2) { if (!(tspr->cstat&512)) method = 2+4; else method = 3+4; }

#ifdef USE_OPENGL
	if (rendmode == 3) polymost_beginframe_shader(0); //after drawrooms', not cleared
	if (!nofog) {
	if (rendmode == 3)
	{
		glEnable(GL_FOG); fog_is_on = 1; //setpolymost2dview turns it off
		polymost_fogvis(sector[tspr->sectnum].visibility);
	}
	}
#endif

	switch((globalorientation>>4)&3)
	{
		case 0: //Face sprite

				//Project 3D to 2D
			sx0 = (float)(tspr->x-globalposx);
			sy0 = (float)(tspr->y-globalposy);
			xp0 = sy0*gcosang  - sx0*gsinang;
			yp0 = sx0*gcosang2 + sy0*gsinang2;
			if (yp0 <= SCISDIST) return;
			ryp0 = 1/yp0;
			sx0 = ghalfx*xp0*ryp0 + ghalfx;
			sy0 = ((float)(tspr->z-globalposz))*gyxscale*ryp0 + ghoriz;

			f = ryp0*(float)xdimen/160.0;
			fx = ((float)tspr->xrepeat)*f;
			fy = ((float)tspr->yrepeat)*f*((float)yxaspect/65536.0);
			sx0 -= fx*(float)xoff; if (tilesizx[globalpicnum]&1) sx0 += fx*.5;
			sy0 -= fy*(float)yoff;
			fx *= ((float)tilesizx[globalpicnum]);
			fy *= ((float)tilesizy[globalpicnum]);

			px[0] = px[3] = sx0-fx*.5; px[1] = px[2] = sx0+fx*.5;
			if (!(globalorientation&128)) { py[0] = py[1] = sy0-fy; py[2] = py[3] = sy0; }
											 else { py[0] = py[1] = sy0-fy*.5; py[2] = py[3] = sy0+fy*.5; }

			gdx = gdy = guy = gvx = 0; gdo = ryp0*gviewxrange;
			if (!(globalorientation&4))
				  { gux = (float)tilesizx[globalpicnum]*gdo/(px[1]-px[0]+.002); guo = -gux*(px[0]-.001); }
			else { gux = (float)tilesizx[globalpicnum]*gdo/(px[0]-px[1]-.002); guo = -gux*(px[1]+.001); }
			if (!(globalorientation&8))
				  { gvy = (float)tilesizy[globalpicnum]*gdo/(py[3]-py[0]+.002); gvo = -gvy*(py[0]-.001); }
			else { gvy = (float)tilesizy[globalpicnum]*gdo/(py[0]-py[3]-.002); gvo = -gvy*(py[3]+.001); }

			//Clip sprites to ceilings/floors when no parallaxing and not sloped
			if (!(sector[tspr->sectnum].ceilingstat&3))
			{
				sy0 = ((float)(sector[tspr->sectnum].ceilingz-globalposz))*gyxscale*ryp0 + ghoriz;
				if (py[0] < sy0) py[0] = py[1] = sy0;
			}
			if (!(sector[tspr->sectnum].floorstat&3))
			{
				sy0 = ((float)(sector[tspr->sectnum].floorz-globalposz))*gyxscale*ryp0 + ghoriz;
				if (py[2] > sy0) py[2] = py[3] = sy0;
			}

				//In front of the walls behind it, as classic: nearer, the same picture
				//   (u, v and the depth scale together) and shade (its fog's shades per
				//   unit of depth scale back), not writing the depth it's moved to
			f = polymost_spritenear(tspr,yp0,(px[0]-ghalfx)*yp0/ghalfx,(px[1]-ghalfx)*yp0/ghalfx);
			nearer = (f < yp0);
			if (nearer)
			{
				f = yp0/f;
				gdo *= f; gux *= f; guo *= f; gvy *= f; gvo *= f;
				zbufwrite = 0;
#ifdef USE_OPENGL
				fogshades = glfogshades; glfogshades *= f;
				if (rendmode == 3) glDepthMask(GL_FALSE);
#endif
			}

			pow2xsplit = 0; drawpoly(px,py,4,method|METH_LAYERS);
			zbufwrite = 1;
#ifdef USE_OPENGL
			if (nearer)
			{
				glfogshades = fogshades;
				if (rendmode == 3) glDepthMask(GL_TRUE);
			}
#endif
			break;
		case 1: //Wall sprite
				//Project 3D to 2D
			if (globalorientation&4) xoff = -xoff;
			if (globalorientation&8) yoff = -yoff;

			xv = (float)tspr->xrepeat * (float)sintable[(tspr->ang     )&2047] / 65536.0;
			yv = (float)tspr->xrepeat * (float)sintable[(tspr->ang+1536)&2047] / 65536.0;
			f = (float)(tilesizx[globalpicnum]>>1) + (float)xoff;
			x0 = (float)(tspr->x-globalposx) - xv*f; x1 = xv*(float)tilesizx[globalpicnum] + x0;
			y0 = (float)(tspr->y-globalposy) - yv*f; y1 = yv*(float)tilesizx[globalpicnum] + y0;

			yp0 = x0*gcosang2 + y0*gsinang2;
			yp1 = x1*gcosang2 + y1*gsinang2;
			if ((yp0 <= SCISDIST) && (yp1 <= SCISDIST)) return;
			xp0 = y0*gcosang - x0*gsinang;
			xp1 = y1*gcosang - x1*gsinang;

				//Clip to close parallel-screen plane
			oxp0 = xp0; oyp0 = yp0;
			if (yp0 < SCISDIST) { t0 = (SCISDIST-yp0)/(yp1-yp0); xp0 = (xp1-xp0)*t0+xp0; yp0 = SCISDIST; }
								else { t0 = 0.f; }
			if (yp1 < SCISDIST) { t1 = (SCISDIST-oyp0)/(yp1-oyp0); xp1 = (xp1-oxp0)*t1+oxp0; yp1 = SCISDIST; }
								else { t1 = 1.f; }

			f = ((float)tspr->yrepeat) * (float)tilesizy[globalpicnum] * 4;

			ryp0 = 1.0/yp0;
			ryp1 = 1.0/yp1;
			sx0 = ghalfx*xp0*ryp0 + ghalfx;
			sx1 = ghalfx*xp1*ryp1 + ghalfx;
			ryp0 *= gyxscale;
			ryp1 *= gyxscale;

			tspr->z -= ((yoff*tspr->yrepeat)<<2);
			if (globalorientation&128)
			{
				tspr->z += ((tilesizy[globalpicnum]*tspr->yrepeat)<<1);
				if (tilesizy[globalpicnum]&1) tspr->z += (tspr->yrepeat<<1); //Odd yspans
			}

			sc0 = ((float)(tspr->z-globalposz-f))*ryp0 + ghoriz;
			sc1 = ((float)(tspr->z-globalposz-f))*ryp1 + ghoriz;
			sf0 = ((float)(tspr->z-globalposz))*ryp0 + ghoriz;
			sf1 = ((float)(tspr->z-globalposz))*ryp1 + ghoriz;

			gdx = (ryp0-ryp1)*gxyaspect / (sx0-sx1);
			gdy = 0;
			gdo = ryp0*gxyaspect - gdx*sx0;

				//Original equations:
				//(gux*sx0 + guo)/(gdx*sx1 + gdo) = tilesizx[globalpicnum]*t0
				//(gux*sx1 + guo)/(gdx*sx1 + gdo) = tilesizx[globalpicnum]*t1
				//
				// gvx*sx0 + gvy*sc0 + gvo = 0
				// gvy*sx1 + gvy*sc1 + gvo = 0
				//(gvx*sx0 + gvy*sf0 + gvo)/(gdx*sx0 + gdo) = tilesizy[globalpicnum]
				//(gvx*sx1 + gvy*sf1 + gvo)/(gdx*sx1 + gdo) = tilesizy[globalpicnum]

				//gux*sx0 + guo = t0*tilesizx[globalpicnum]*yp0
				//gux*sx1 + guo = t1*tilesizx[globalpicnum]*yp1
			if (globalorientation&4) { t0 = 1.f-t0; t1 = 1.f-t1; }
			gux = (t0*ryp0 - t1*ryp1)*gxyaspect*(float)tilesizx[globalpicnum] / (sx0-sx1);
			guy = 0;
			guo = t0*ryp0*gxyaspect*(float)tilesizx[globalpicnum] - gux*sx0;

				//gvx*sx0 + gvy*sc0 + gvo = 0
				//gvx*sx1 + gvy*sc1 + gvo = 0
				//gvx*sx0 + gvy*sf0 + gvo = tilesizy[globalpicnum]*(gdx*sx0 + gdo)
			f = ((float)tilesizy[globalpicnum])*(gdx*sx0 + gdo) / ((sx0-sx1)*(sc0-sf0));
			if (!(globalorientation&8))
			{
				gvx = (sc0-sc1)*f;
				gvy = (sx1-sx0)*f;
				gvo = -gvx*sx0 - gvy*sc0;
			}
			else
			{
				gvx = (sf1-sf0)*f;
				gvy = (sx0-sx1)*f;
				gvo = -gvx*sx0 - gvy*sf0;
			}

			//Clip sprites to ceilings/floors when no parallaxing
			if (!(sector[tspr->sectnum].ceilingstat&1))
			{
				f = ((float)tspr->yrepeat) * (float)tilesizy[globalpicnum] * 4;
				if (sector[tspr->sectnum].ceilingz > tspr->z-f)
				{
					sc0 = ((float)(sector[tspr->sectnum].ceilingz-globalposz))*ryp0 + ghoriz;
					sc1 = ((float)(sector[tspr->sectnum].ceilingz-globalposz))*ryp1 + ghoriz;
				}
			}
			if (!(sector[tspr->sectnum].floorstat&1))
			{
				if (sector[tspr->sectnum].floorz < tspr->z)
				{
					sf0 = ((float)(sector[tspr->sectnum].floorz-globalposz))*ryp0 + ghoriz;
					sf1 = ((float)(sector[tspr->sectnum].floorz-globalposz))*ryp1 + ghoriz;
				}
			}
			
			if (sx0 > sx1)
			{
				if (globalorientation&64) return; //1-sided sprite
				f = sx0; sx0 = sx1; sx1 = f;
				f = sc0; sc0 = sc1; sc1 = f;
				f = sf0; sf0 = sf1; sf1 = f;
			}

			px[0] = sx0; py[0] = sc0;
			px[1] = sx1; py[1] = sc1;
			px[2] = sx1; py[2] = sf1;
			px[3] = sx0; py[3] = sf0;
			pow2xsplit = 0; drawpoly(px,py,4,method|METH_LAYERS);
			break;
		case 2: //Floor sprite
			if ((globalorientation&64) != 0)
				if ((globalposz > tspr->z) == (!(globalorientation&8)))
					return;
			if ((globalorientation&4) > 0) xoff = -xoff;
			if ((globalorientation&8) > 0) yoff = -yoff;

			i = (tspr->ang&2047);
			c = sintable[(i+512)&2047]/65536.0;
			s = sintable[i]/65536.0;
			x0 = ((tilesizx[globalpicnum]>>1)-xoff)*tspr->xrepeat;
			y0 = ((tilesizy[globalpicnum]>>1)-yoff)*tspr->yrepeat;
			x1 = ((tilesizx[globalpicnum]>>1)+xoff)*tspr->xrepeat;
			y1 = ((tilesizy[globalpicnum]>>1)+yoff)*tspr->yrepeat;

				//Project 3D to 2D
			for(j=0;j<4;j++)
			{
				sx0 = (float)(tspr->x-globalposx);
				sy0 = (float)(tspr->y-globalposy);
				if ((j+0)&2) { sy0 -= s*y0; sx0 -= c*y0; } else { sy0 += s*y1; sx0 += c*y1; }
				if ((j+1)&2) { sx0 -= s*x0; sy0 += c*x0; } else { sx0 += s*x1; sy0 -= c*x1; }
				px[j] = sy0*gcosang  - sx0*gsinang;
				py[j] = sx0*gcosang2 + sy0*gsinang2;
			}

			if (tspr->z < globalposz) //if floor sprite is above you, reverse order of points
			{
				f = px[0]; px[0] = px[1]; px[1] = f;
				f = py[0]; py[0] = py[1]; py[1] = f;
				f = px[2]; px[2] = px[3]; px[3] = f;
				f = py[2]; py[2] = py[3]; py[3] = f;
			}

				//Clip to SCISDIST plane
			npoints = 0;
			for(i=0;i<4;i++)
			{
				j = ((i+1)&3);
				if (py[i] >= SCISDIST) { px2[npoints] = px[i]; py2[npoints] = py[i]; npoints++; }
				if ((py[i] >= SCISDIST) != (py[j] >= SCISDIST))
				{
					f = (SCISDIST-py[i])/(py[j]-py[i]);
					px2[npoints] = (px[j]-px[i])*f + px[i];
					py2[npoints] = (py[j]-py[i])*f + py[i]; npoints++;
				}
			}
			if (npoints < 3) return;

				//Project rotated 3D points to screen
			f = ((float)(tspr->z-globalposz))*gyxscale;
			for(j=0;j<npoints;j++)
			{
				ryp0 = 1/py2[j];
				px[j] = ghalfx*px2[j]*ryp0 + ghalfx;
				py[j] = f*ryp0 + ghoriz;
			}

				//gd? Copied from floor rendering code
			gdx = 0;
			gdy = gxyaspect / (double)(tspr->z-globalposz);
			gdo = -ghoriz*gdy;
				//copied&modified from relative alignment
			xv = (float)tspr->x + s*x1 + c*y1; fx = (double)-(x0+x1)*s;
			yv = (float)tspr->y + s*y1 - c*x1; fy = (double)+(x0+x1)*c;
			f = 1.0/sqrt(fx*fx+fy*fy); fx *= f; fy *= f;
			ft[2] = singlobalang*fy + cosglobalang*fx;
			ft[3] = singlobalang*fx - cosglobalang*fy;
			ft[0] = ((double)(globalposy-yv))*fy + ((double)(globalposx-xv))*fx;
			ft[1] = ((double)(globalposx-xv))*fy - ((double)(globalposy-yv))*fx;
			gux = (double)ft[3]*((double)viewingrange)/(-65536.0*262144.0);
			gvx = (double)ft[2]*((double)viewingrange)/(-65536.0*262144.0);
			guy = (double)ft[0]*gdy; gvy = (double)ft[1]*gdy;
			guo = (double)ft[0]*gdo; gvo = (double)ft[1]*gdo;
			guo += (double)(ft[2]/262144.0-gux)*ghalfx;
			gvo -= (double)(ft[3]/262144.0+gvx)*ghalfx;
			f = 4.0/(float)tspr->xrepeat; gux *= f; guy *= f; guo *= f;
			f =-4.0/(float)tspr->yrepeat; gvx *= f; gvy *= f; gvo *= f;
			if (globalorientation&4)
			{
				gux = ((float)tilesizx[globalpicnum])*gdx - gux;
				guy = ((float)tilesizx[globalpicnum])*gdy - guy;
				guo = ((float)tilesizx[globalpicnum])*gdo - guo;
			}

			pow2xsplit = 0; drawpoly(px,py,npoints,method|METH_LAYERS);
			break;

		//case 3: //Voxel sprite
		//   break;
	}
}

	//sx,sy       center of sprite; screen coods*65536
	//z           zoom*65536. > is zoomed in
	//a           angle (0 is default)
	//dastat&1    1:translucence
	//dastat&2    1:auto-scale mode (use 320*200 coordinates)
	//dastat&4    1:y-flip
	//dastat&8    1:don't clip to startumost/startdmost
	//dastat&16   1:force point passed to be top-left corner, 0:Editart center
	//dastat&32   1:reverse translucence
	//dastat&64   1:non-masked, 0:masked
	//dastat&128  1:draw all pages (permanent)
	//dastat&256  1:one-to-one pixel aspect (when dastat&2)
	//cx1,...     clip window (actual screen coords)
void polymost_dorotatesprite (int32_t sx, int32_t sy, int32_t z, short a, short picnum,
	signed char dashade, unsigned char dapalnum, unsigned short dastat, int32_t cx1, int32_t cy1, int32_t cx2, int32_t cy2)
{
	int32_t i, n, nn, x, zz, xoff, yoff, xsiz, ysiz, method;
	int32_t ogpicnum, ogshade, ogpal, oxdimen, oydimen;
	intptr_t ofoffset;
	double ogchang, ogshang, ogctang, ogstang, oghalfx, oghoriz, fx, fy, x1, y1, x2, y2;
	double ogrhalfxdown10, ogrhalfxdown10x;
	double d, cosang, sinang, cosang2, sinang2, px[8], py[8], px2[8], py2[8];

	ogpicnum = globalpicnum; globalpicnum = picnum;
	ogshade  = globalshade;  globalshade  = dashade;
	ogpal    = globalpal;    globalpal    = (int32_t)((unsigned char)dapalnum);
	oghalfx  = ghalfx;       ghalfx       = (double)(xdim>>1);
	ogrhalfxdown10 = grhalfxdown10;    grhalfxdown10 = 1.0/(((double)ghalfx)*1024);
	ogrhalfxdown10x = grhalfxdown10x;  grhalfxdown10x = grhalfxdown10;
	oghoriz  = ghoriz;       ghoriz       = (double)(ydim>>1);
	ofoffset = frameoffset;  frameoffset  = frameplace;
	oxdimen  = xdimen;       xdimen       = xdim;
	oydimen  = ydimen;       ydimen       = ydim;
	ogchang = gchang; gchang = 1.0;
	ogshang = gshang; gshang = 0.0;
	ogctang = gctang; gctang = 1.0;
	ogstang = gstang; gstang = 0.0;

#ifdef USE_OPENGL
	if (rendmode == 3)
	{
		polymost_shaderdraw(1); //GL 2.1: indices, over the 3D view
		resizeglcheck();
		_glframedrawn();
		if (1) //dastat&(2+8))
		{
			float m[4][4];
			glViewport(0,0,xdim,ydim); glox1 = -1; //Force fullscreen (glox1=-1 forces it to restore)
			glMatrixMode(GL_PROJECTION);
			memset(m,0,sizeof(m));
			m[0][0] = m[2][3] = 1.0; m[1][1] = ((float)xdim)/((float)ydim); m[2][2] = 1.0001; m[3][2] = 1-m[2][2];
			glPushMatrix(); glLoadMatrixf(&m[0][0]);
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		}
		glDisable(GL_DEPTH_TEST);
	}
#endif

	method = 0;
	if (!(dastat&64))
	{
		method = 1;
		if (dastat&1) { if (!(dastat&32)) method = 2; else method = 3; }
	}
	method |= 4; //Use OpenGL clamping - dorotatesprite never repeats

	xsiz = tilesizx[globalpicnum]; ysiz = tilesizy[globalpicnum];
	if (dastat&16) { xoff = 0; yoff = 0; }
	else
	{
		xoff = (int32_t)((signed char)((picanm[globalpicnum]>>8)&255))+(xsiz>>1);
		yoff = (int32_t)((signed char)((picanm[globalpicnum]>>16)&255))+(ysiz>>1);
	}
	if (dastat&4) yoff = ysiz-yoff;

	if (dastat&2)  //Auto window size scaling
	{
		if (!(dastat&8))
		{
			if (widescreen) {
				x = ydimenscale;   //= scale(ydimen,yxaspect,mulscale16(200,pixelaspect));
				sx = ((cx1+cx2+2)<<15)+scale(sx-(320<<15),oydimen<<16,200*pixelaspect);
			} else {
				x = xdimenscale;   //= scale(xdimen,yxaspect,320);
				sx = ((cx1+cx2+2)<<15)+scale(sx-(320<<15),oxdimen,320);
			}
			sy = ((cy1+cy2+2)<<15)+mulscale16(sy-(200<<15),x);
		}
		else
		{
				//If not clipping to startmosts, & auto-scaling on, as a
				//hard-coded bonus, scale to full screen instead
			if (widescreen) {
				x = scale(ydim<<16,yxaspect,200*pixelaspect);
				sx = (xdim<<15)+32768+scale(sx-(320<<15),ydim<<16,200*pixelaspect);
			} else {
				x = scale(xdim,yxaspect,320);
				sx = (xdim<<15)+32768+scale(sx-(320<<15),xdim,320);
			}
			sy = (ydim<<15)+32768+mulscale16(sy-(200<<15),x);
		}
		z = mulscale16(z,x);
	}

	d = (double)z/(65536.0*16384.0);
	cosang2 = cosang = (double)sintable[(a+512)&2047]*d;
	sinang2 = sinang = (double)sintable[a&2047]*d;
	if (((dastat&2) && !(dastat&256)) || (!(dastat&8))) //Don't aspect unscaled perms
		{ d = (double)xyaspect/65536.0; cosang2 *= d; sinang2 *= d; }
	else if (pixelaspect == 65536 && ((dastat&2) && (dastat&256))) //Skew 1:1 to 1.2:1 aspect
		{ d = 1.2; cosang2 *= d; sinang2 *= d; }
	px[0] = (double)sx/65536.0 - (double)xoff*cosang2+ (double)yoff*sinang2;
	py[0] = (double)sy/65536.0 - (double)xoff*sinang - (double)yoff*cosang;
	px[1] = px[0] + (double)xsiz*cosang2;
	py[1] = py[0] + (double)xsiz*sinang;
	px[3] = px[0] - (double)ysiz*sinang2;
	py[3] = py[0] + (double)ysiz*cosang;
	px[2] = px[1]+px[3]-px[0];
	py[2] = py[1]+py[3]-py[0];
	n = 4;

	gdx = 0; gdy = 0; gdo = 1.0;
		//px[0]*gux + py[0]*guy + guo = 0
		//px[1]*gux + py[1]*guy + guo = xsiz-.0001
		//px[3]*gux + py[3]*guy + guo = 0
	d = 1.0/(px[0]*(py[1]-py[3]) + px[1]*(py[3]-py[0]) + px[3]*(py[0]-py[1]));
	gux = (py[3]-py[0])*((double)xsiz-.0001)*d;
	guy = (px[0]-px[3])*((double)xsiz-.0001)*d;
	guo = 0 - px[0]*gux - py[0]*guy;

	if (!(dastat&4))
	{     //px[0]*gvx + py[0]*gvy + gvo = 0
			//px[1]*gvx + py[1]*gvy + gvo = 0
			//px[3]*gvx + py[3]*gvy + gvo = ysiz-.0001
		gvx = (py[0]-py[1])*((double)ysiz-.0001)*d;
		gvy = (px[1]-px[0])*((double)ysiz-.0001)*d;
		gvo = 0 - px[0]*gvx - py[0]*gvy;
	}
	else
	{     //px[0]*gvx + py[0]*gvy + gvo = ysiz-.0001
			//px[1]*gvx + py[1]*gvy + gvo = ysiz-.0001
			//px[3]*gvx + py[3]*gvy + gvo = 0
		gvx = (py[1]-py[0])*((double)ysiz-.0001)*d;
		gvy = (px[0]-px[1])*((double)ysiz-.0001)*d;
		gvo = (double)ysiz-.0001 - px[0]*gvx - py[0]*gvy;
	}

	cx2++; cy2++;
		//Clippoly4 (converted from long to double)
	nn = z = 0;
	do
	{
		zz = z+1; if (zz == n) zz = 0;
		x1 = px[z]; x2 = px[zz]-x1; if ((cx1 <= x1) && (x1 <= cx2)) { px2[nn] = x1; py2[nn] = py[z]; nn++; }
		if (x2 <= 0) fx = cx2; else fx = cx1;  d = fx-x1;
		if ((d < x2) != (d < 0)) { px2[nn] = fx; py2[nn] = (py[zz]-py[z])*d/x2 + py[z]; nn++; }
		if (x2 <= 0) fx = cx1; else fx = cx2;  d = fx-x1;
		if ((d < x2) != (d < 0)) { px2[nn] = fx; py2[nn] = (py[zz]-py[z])*d/x2 + py[z]; nn++; }
		z = zz;
	} while (z);
	if (nn >= 3)
	{
		n = z = 0;
		do
		{
			zz = z+1; if (zz == nn) zz = 0;
			y1 = py2[z]; y2 = py2[zz]-y1; if ((cy1 <= y1) && (y1 <= cy2)) { py[n] = y1; px[n] = px2[z]; n++; }
			if (y2 <= 0) fy = cy2; else fy = cy1;  d = fy-y1;
			if ((d < y2) != (d < 0)) { py[n] = fy; px[n] = (px2[zz]-px2[z])*d/y2 + px2[z]; n++; }
			if (y2 <= 0) fy = cy1; else fy = cy2;  d = fy-y1;
			if ((d < y2) != (d < 0)) { py[n] = fy; px[n] = (px2[zz]-px2[z])*d/y2 + px2[z]; n++; }
			z = zz;
		} while (z);
		pow2xsplit = 0; drawpoly(px,py,n,method);
	}

#ifdef USE_OPENGL
	if (rendmode == 3) { glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); }
#endif

	globalpicnum = ogpicnum;
	globalshade  = ogshade;
	globalpal    = ogpal;
	ghalfx       = oghalfx;
	grhalfxdown10 = ogrhalfxdown10;
	grhalfxdown10x = ogrhalfxdown10x;
	ghoriz       = oghoriz;
	frameoffset  = ofoffset;
	xdimen       = oxdimen;
	ydimen       = oydimen;
	gchang = ogchang;
	gshang = ogshang;
	gctang = ogctang;
	gstang = ogstang;
}

#ifdef USE_OPENGL
static float trapextx[2];
static void drawtrap (float x0, float x1, float y0, float x2, float x3, float y1)
{
	float px[4], py[4];
	int32_t i, n;

	if (y0 == y1) return;
	px[0] = x0; py[0] = y0;  py[2] = y1;
		  if (x0 == x1) { px[1] = x3; py[1] = y1; px[2] = x2; n = 3; }
	else if (x2 == x3) { px[1] = x1; py[1] = y0; px[2] = x3; n = 3; }
	else               { px[1] = x1; py[1] = y0; px[2] = x3; px[3] = x2; py[3] = y1; n = 4; }

	glsub_begin(GL_TRIANGLE_FAN);
	for(i=0;i<n;i++)
	{
		px[i] = min(max(px[i],trapextx[0]),trapextx[1]);
		glsub_texcoord(px[i]*gux + py[i]*guy + guo,
						 px[i]*gvx + py[i]*gvy + gvo);
		glsub_vertex(px[i],py[i],0);
	}
	glsub_end();
}

static void tessectrap (float *px, float *py, int32_t *point2, int32_t numpoints)
{
	float x0, x1, m0, m1;
	int32_t i, j, k, z, i0, i1, i2, i3, npoints, gap, numrst;

	static int32_t allocpoints = 0, *slist = 0, *npoint2 = 0;
	typedef struct { float x, y, xi; int32_t i; } raster;
	static raster *rst = 0;
	if (numpoints+16 > allocpoints) //16 for safety
	{
		raster *nrst; int32_t *nslist, *nnpoint2;

		nrst = realloc(rst,(numpoints+16)*sizeof(raster)); if (nrst) rst = nrst;
		nslist = realloc(slist,(numpoints+16)*sizeof(int32_t)); if (nslist) slist = nslist;
		nnpoint2 = realloc(npoint2,(numpoints+16)*sizeof(int32_t)); if (nnpoint2) npoint2 = nnpoint2;
		if ((!nrst) || (!nslist) || (!nnpoint2)) return; //out of memory: draw nothing
		allocpoints = numpoints+16;
	}

		//Remove unnecessary collinear points:
	for(i=0;i<numpoints;i++) npoint2[i] = point2[i];
	npoints = numpoints; z = 0;
	for(i=0;i<numpoints;i++)
	{
		j = npoint2[i]; if ((point2[i] < i) && (i < numpoints-1)) z = 3;
		if (j < 0) continue;
		k = npoint2[j];
		m0 = (px[j]-px[i])*(py[k]-py[j]);
		m1 = (py[j]-py[i])*(px[k]-px[j]);
		if (m0 < m1) { z |= 1; continue; }
		if (m0 > m1) { z |= 2; continue; }
		npoint2[i] = k; npoint2[j] = -1; npoints--; i--; //collinear
	}
	if (!z) return;
	trapextx[0] = trapextx[1] = px[0];
	for(i=j=0;i<numpoints;i++)
	{
		if (npoint2[i] < 0) continue;
		if (px[i] < trapextx[0]) trapextx[0] = px[i];
		if (px[i] > trapextx[1]) trapextx[1] = px[i];
		slist[j++] = i;
	}
	if (z != 3) //Simple polygon... early out
	{
		glsub_begin(GL_TRIANGLE_FAN);
		for(i=0;i<npoints;i++)
		{
			j = slist[i];
			glsub_texcoord(px[j]*gux + py[j]*guy + guo,
							 px[j]*gvx + py[j]*gvy + gvo);
			glsub_vertex(px[j],py[j],0);
		}
		glsub_end();
		return;
	}

		//Sort points by y's
	for(gap=(npoints>>1);gap;gap>>=1)
		for(i=0;i<npoints-gap;i++)
			for(j=i;j>=0;j-=gap)
			{
				if (py[npoint2[slist[j]]] <= py[npoint2[slist[j+gap]]]) break;
				k = slist[j]; slist[j] = slist[j+gap]; slist[j+gap] = k;
			}

	numrst = 0;
	for(z=0;z<npoints;z++)
	{
		i0 = slist[z]; i1 = npoint2[i0]; if (py[i0] == py[i1]) continue;
		i2 = i1; i3 = npoint2[i1];
		if (py[i1] == py[i3]) { i2 = i3; i3 = npoint2[i3]; }

			//i0        i3
			//  \      /
			//   i1--i2
			//  /      \ .
			//i0        i3

		if ((py[i1] < py[i0]) && (py[i2] < py[i3])) //Insert raster
		{
			for(i=numrst;i>0;i--)
			{
				if (rst[i-1].xi*(py[i1]-rst[i-1].y) + rst[i-1].x < px[i1]) break;
				rst[i+1] = rst[i-1];
			}
			numrst += 2;

			if (i&1) //split inside area
			{
				j = i-1;

				x0 = (py[i1] - rst[j  ].y)*rst[j  ].xi + rst[j  ].x;
				x1 = (py[i1] - rst[j+1].y)*rst[j+1].xi + rst[j+1].x;
				drawtrap(rst[j].x,rst[j+1].x,rst[j].y,x0,x1,py[i1]);
				rst[j  ].x = x0; rst[j  ].y = py[i1];
				rst[j+3].x = x1; rst[j+3].y = py[i1];
			}

			m0 = (px[i0]-px[i1]) / (py[i0]-py[i1]);
			m1 = (px[i3]-px[i2]) / (py[i3]-py[i2]);
			j = ((px[i1] > px[i2]) || ((i1 == i2) && (m0 >= m1))) + i;
			k = (i<<1)+1 - j;
			rst[j].i = i0; rst[j].xi = m0; rst[j].x = px[i1]; rst[j].y = py[i1];
			rst[k].i = i3; rst[k].xi = m1; rst[k].x = px[i2]; rst[k].y = py[i2];
		}
		else
		{                  //NOTE:don't count backwards!
			if (i1 == i2) { for(i=0;i<numrst;i++) if (rst[i].i == i1) break; }
						else { for(i=0;i<numrst;i++) if ((rst[i].i == i1) || (rst[i].i == i2)) break; }
			j = i&~1;

			if ((py[i1] > py[i0]) && (py[i2] > py[i3])) //Delete raster
			{
				for(;j<=i+1;j+=2)
				{
					x0 = (py[i1] - rst[j  ].y)*rst[j  ].xi + rst[j  ].x;
					if ((i == j) && (i1 == i2)) x1 = x0; else x1 = (py[i1] - rst[j+1].y)*rst[j+1].xi + rst[j+1].x;
					drawtrap(rst[j].x,rst[j+1].x,rst[j].y,x0,x1,py[i1]);
					rst[j  ].x = x0; rst[j  ].y = py[i1];
					rst[j+1].x = x1; rst[j+1].y = py[i1];
				}
				numrst -= 2; for(;i<numrst;i++) rst[i] = rst[i+2];
			}
			else
			{
				x0 = (py[i1] - rst[j  ].y)*rst[j  ].xi + rst[j  ].x;
				x1 = (py[i1] - rst[j+1].y)*rst[j+1].xi + rst[j+1].x;
				drawtrap(rst[j].x,rst[j+1].x,rst[j].y,x0,x1,py[i1]);
				rst[j  ].x = x0; rst[j  ].y = py[i1];
				rst[j+1].x = x1; rst[j+1].y = py[i1];

				if (py[i0] < py[i3]) { rst[i].x = px[i2]; rst[i].y = py[i2]; rst[i].i = i3; }
									 else { rst[i].x = px[i1]; rst[i].y = py[i1]; rst[i].i = i0; }
				rst[i].xi = (px[rst[i].i] - rst[i].x) / (py[rst[i].i] - py[i1]);
			}
		}
	}
}

void polymost_fillpolygon (int32_t npoints)
{
	static float fx[MAXWALLSB], fy[MAXWALLSB];
	float f, a = 1.0, zero;
	double cx, cy;
	int32_t i, j, k, shb[4];

	globalx1 = mulscale16(globalx1,xyaspect);
	globaly2 = mulscale16(globaly2,xyaspect);
	gux = ((double)asm1)*(1.0/4294967296.0);
	gvx = ((double)(int32_t)asm2)*(1.0/4294967296.0);
	guy = ((double)globalx1)*(1.0/4294967296.0);
	gvy = ((double)globaly2)*(-1.0/4294967296.0);
	guo = (((double)xdim)*gux + ((double)ydim)*guy)*-.5 + ((double)globalposx)*(1.0/4294967296.0);
	gvo = (((double)xdim)*gvx + ((double)ydim)*gvy)*-.5 - ((double)globalposy)*(1.0/4294967296.0);

	for(i=npoints-1;i>=0;i--)
	{
		fx[i] = ((float)rx1[i])/4096.0;
		fy[i] = ((float)ry1[i])/4096.0;
	}

		//u and v run across the power-of-2 texture gloadtile makes without
		//non-power-of-2 textures; with them, the texture is the tile's own size.
		//The fixed point above drops whole repeats, which only a power-of-2
		//texture hides: take them off at the polygon's middle before scaling.
	if (glinfo.texnpot)
	{
		cx = cy = 0;
		for(i=npoints-1;i>=0;i--) { cx += fx[i]; cy += fy[i]; }
		cx /= (double)npoints; cy /= (double)npoints;
		guo -= floor(cx*gux + cy*guy + guo);
		gvo -= floor(cx*gvx + cy*gvy + gvo);
		j = max(1,tilesizx[globalpicnum]); //an animated frame can be empty
		for(i=1;i<j;i+=i);
		f = ((float)i)/((float)j);
		gux *= f; guy *= f; guo *= f;
		j = max(1,tilesizy[globalpicnum]);
		for(i=1;i<j;i+=i);
		f = ((float)i)/((float)j);
		gvx *= f; gvy *= f; gvo *= f;
	}

	setpolymost2dview(); //disables blending, texturing, and depth testing
	if (polymost_shaderdraw(1)) //GL 2.1: indices through the palookups, masked or translucent
	{
		if (!waloff[globalpicnum]) loadtile(globalpicnum);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,polymost_idxtex(globalpicnum,0));
		shb[0] = shb[2] = 0; //its box, in the 2D view (framebuffer pixels, bottom-up)
		for(i=npoints-1;i>=0;i--)
		{
			j = (int32_t)fx[i]; if ((i == npoints-1) || (j < shb[0])) shb[0] = j;
			j = (int32_t)ceil(fx[i]); if ((i == npoints-1) || (j > shb[2])) shb[2] = j;
			j = yres-(int32_t)ceil(fy[i]); if ((i == npoints-1) || (j < shb[1])) shb[1] = j;
			j = yres-(int32_t)fy[i]; if ((i == npoints-1) || (j > shb[3])) shb[3] = j;
		}
		shb[0] = max(shb[0]-1,0); shb[1] = max(shb[1]-1,0);
		shb[2] = min(shb[2]+1,glshfbox); shb[3] = min(shb[3]+1,glshfboy);
		k = (globalpolytype)?((globalorientation>>7)&3):0; if ((k < 2) || (!glshtrans)) k = 1;
		if ((k >= 2) && (!glshcolor)) polymost_transread(shb);
		polymost_shprog((k >= 2) ? 2 : 1);
		polymost_shtile(globalpicnum,0);
		if ((glshcolor) && (k >= 2)) glEnable(GL_BLEND);
		glsub_color((float)globalshade,(float)(palookup[globalpal]?globalpal:0),0.f,(float)k);
	}
	else
	{
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.32f);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,glpic[gltexcache(globalpicnum,globalpal,0)]);

	f = polymost_shaderamp(globalpicnum,globalshade,&zero);
	switch ((globalpolytype)?((globalorientation>>7)&3):0) {
		case 0:
		case 1: a = 1.0; glDisable(GL_BLEND); break;
		case 2: a = 0.66; glEnable(GL_BLEND); break;
		case 3: a = 0.33; glEnable(GL_BLEND); break;
	}
	glsub_color(f,f,f,a);
	}

	tessectrap(fx,fy,xb1,npoints);
	if (glshbound) polymost_drawn(shb);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	_glframedrawn();
}
#endif

int32_t polymost_drawtilescreen (int32_t tilex, int32_t tiley, int32_t wallnum, int32_t dimen)
{
#ifdef USE_OPENGL
	float xdime, ydime, xdimepad, ydimepad, scx, scy;
	int32_t i;

	if ((rendmode != 3) || (qsetmode != 200)) return(-1);

	if (!glinfo.texnpot) {
		i = (1<<(picsiz[wallnum]&15)); if (i < tilesizx[wallnum]) i += i; xdimepad = (float)i;
		i = (1<<(picsiz[wallnum]>>4)); if (i < tilesizy[wallnum]) i += i; ydimepad = (float)i;
	} else {
		xdimepad = (float)max(1,tilesizx[wallnum]);
		ydimepad = (float)max(1,tilesizy[wallnum]);
	}
	xdime = (float)tilesizx[wallnum]; xdimepad = xdime/xdimepad;
	ydime = (float)tilesizy[wallnum]; ydimepad = ydime/ydimepad;

	if ((xdime <= dimen) && (ydime <= dimen))
	{
		scx = xdime;
		scy = ydime;
	}
	else
	{
		scx = (float)dimen;
		scy = (float)dimen;
		if (xdime < ydime) scx *= xdime/ydime; else scy *= ydime/xdime;
	}

	if (polymost_shaderdraw(0)) //GL 2.1: the tile's indices over index 255
	{
		glDisable(GL_TEXTURE_2D);
		glsub_begin(GL_TRIANGLE_FAN);
		polymost_2dcolor(255);
		glsub_vertex((float)tilex    ,(float)tiley    ,0);
		glsub_vertex((float)tilex+scx,(float)tiley    ,0);
		glsub_vertex((float)tilex+scx,(float)tiley+scy,0);
		glsub_vertex((float)tilex    ,(float)tiley+scy,0);
		glsub_end();
		polymost_shaderdraw(1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,polymost_idxtex(wallnum,4));
		polymost_shtile(wallnum,4);
		glsub_color(0.f,0.f,0.f,1.f); //shade 0, pal 0, masked
		glsub_begin(GL_TRIANGLE_FAN);
		glsub_texcoord(       0,       0); glsub_vertex((float)tilex    ,(float)tiley    ,0);
		glsub_texcoord(xdimepad,       0); glsub_vertex((float)tilex+scx,(float)tiley    ,0);
		glsub_texcoord(xdimepad,ydimepad); glsub_vertex((float)tilex+scx,(float)tiley+scy,0);
		glsub_texcoord(       0,ydimepad); glsub_vertex((float)tilex    ,(float)tiley+scy,0);
		glsub_end();
		return(0);
	}

	glBindTexture(GL_TEXTURE_2D,glpic[gltexcache(wallnum,0,4)]);
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP); //not needed: done in polymost_drawtilescreen
	//glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

	glDisable(GL_TEXTURE_2D);
	glsub_begin(GL_TRIANGLE_FAN);
	glsub_color((float)curpalette[255].r/255.0,(float)curpalette[255].g/255.0,(float)curpalette[255].b/255.0,1);
	glsub_vertex((float)tilex    ,(float)tiley    ,0);
	glsub_vertex((float)tilex+scx,(float)tiley    ,0);
	glsub_vertex((float)tilex+scx,(float)tiley+scy,0);
	glsub_vertex((float)tilex    ,(float)tiley+scy,0);
	glsub_end();

	glsub_color(1,1,1,1);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glsub_begin(GL_TRIANGLE_FAN);
	glsub_texcoord(       0,       0); glsub_vertex((float)tilex    ,(float)tiley    ,0);
	glsub_texcoord(xdimepad,       0); glsub_vertex((float)tilex+scx,(float)tiley    ,0);
	glsub_texcoord(xdimepad,ydimepad); glsub_vertex((float)tilex+scx,(float)tiley+scy,0);
	glsub_texcoord(       0,ydimepad); glsub_vertex((float)tilex    ,(float)tiley+scy,0);
	glsub_end();
	
	return(0);
#else
	return -1;
#endif
}

void polymost_precache(int32_t dapicnum, int32_t dapalnum, int32_t datype)
{
#ifdef USE_OPENGL
	// dapicnum and dapalnum are like you'd expect
	// datype is 0 for a wall/floor/ceiling and 1 for a sprite
	//    basically this just means walls are repeating
	//    while sprites are clamped

	if (rendmode < 3) return;

	if (gldrawpath == GLPATH_SHADER) //GL 2.1: the tile's indices, the same for every pal
	{
		if (!waloff[dapicnum]) loadtile(dapicnum);
		if (waloff[dapicnum]) polymost_idxtex(dapicnum,datype&1);
		return;
	}
	
	if (!palookup[dapalnum]) dapalnum = 0;
	gltexcache(dapicnum, dapalnum, (datype & 1) << 2);
#endif
}
