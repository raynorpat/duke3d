/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "build.h"
#include "cache1d.h"
#include "engine.h"
#include "display.h"
#include "pragmas.h"
#include "a.h"
#include "engine_internal.h"

#if defined(POLYMOST) && defined(USE_OPENGL)
#include "SDL_opengl.h"
#define GLSUB_WRAP
#include "glsubmit.h"
#include "glext.h"
#endif

/* used to be static. --ryan. */
int32_t curbrightness = 0;

char britable[16][64];
char textfont[1024], smalltextfont[1024];

char *transluc = NULL, paletteloaded = 0;

static short bakumost[MAXXDIM+1], bakdmost[MAXXDIM+1];

static unsigned char palettebuf[1024];

#define FASTPALGRIDSIZ 8
static int32_t rdist[129], gdist[129], bdist[129];
static char colhere[((FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2))>>3];
static char colhead[(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)*(FASTPALGRIDSIZ+2)];
static int32_t colnext[256];
static char coldist[8] = {0,1,2,3,4,3,2,1};
static int32_t colscan[27];

static void initfastcolorlookup(int32_t rscale, int32_t gscale, int32_t bscale)
{
	int32_t i, j, x, y, z;
	char *pal1;

	j = 0;
	for(i=64;i>=0;i--)
	{
		/*j = (i-64)*(i-64);*/
		rdist[i] = rdist[128-i] = j*rscale;
		gdist[i] = gdist[128-i] = j*gscale;
		bdist[i] = bdist[128-i] = j*bscale;
		j += 129-(i<<1);
	}

	clearbufbyte((void *)FP_OFF(colhere),sizeof(colhere),0L);
	clearbufbyte((void *)FP_OFF(colhead),sizeof(colhead),0L);

	pal1 = (char *)&palette[768-3];
	for(i=255;i>=0;i--,pal1-=3)
	{
		j = (pal1[0]>>3)*FASTPALGRIDSIZ*FASTPALGRIDSIZ+(pal1[1]>>3)*FASTPALGRIDSIZ+(pal1[2]>>3)+FASTPALGRIDSIZ*FASTPALGRIDSIZ+FASTPALGRIDSIZ+1;
		if (colhere[j>>3]&pow2char[j&7]) colnext[i] = colhead[j]; else colnext[i] = -1;
		colhead[j] = i;
		colhere[j>>3] |= pow2char[j&7];
	}

	i = 0;
	for(x=-FASTPALGRIDSIZ*FASTPALGRIDSIZ;x<=FASTPALGRIDSIZ*FASTPALGRIDSIZ;x+=FASTPALGRIDSIZ*FASTPALGRIDSIZ)
		for(y=-FASTPALGRIDSIZ;y<=FASTPALGRIDSIZ;y+=FASTPALGRIDSIZ)
			for(z=-1;z<=1;z++)
				colscan[i++] = x+y+z;
	i = colscan[13]; colscan[13] = colscan[26]; colscan[26] = i;
}


void engine_loadpalette(void)
{
	int32_t i, j, k, dist, fil;
	char *ptr;

	if (paletteloaded != 0) return;
	if ((fil = kopen4load("palette.dat",0)) == -1) return;

	kread(fil,palette,768);
	kread16(fil,&numpalookups);

	if ((palookup[0] = (char *)kkmalloc(numpalookups<<8)) == NULL)
		allocache((intptr_t *)&palookup[0],numpalookups<<8,&permanentlock);
	if ((transluc = (char *)kkmalloc(65536L)) == NULL)
		allocache((intptr_t *)&transluc,65536,&permanentlock);

	globalpalwritten = palookup[0]; globalpal = 0;
	setpalookupaddress(globalpalwritten);

	fixtransluscence(FP_OFF(transluc));

	kread(fil,palookup[globalpal],numpalookups<<8);


	/*kread(fil,transluc,65536);*/
	for (k = 0; k < (65536 / 4); k++)
	    kread32(fil, ((int32_t *) transluc) + k);

	kclose(fil);

	initfastcolorlookup(30L,59L,11L);

	paletteloaded = 1;

	if (vidoption == 6)
	{
		for(k=0;k<MAXPALOOKUPS;k++)
			if (palookup[k] != NULL)
				for(i=0;i<256;i++)
				{
					dist = palette[i*3]*3+palette[i*3+1]*5+palette[i*3+2]*2;
					ptr = (char *)(FP_OFF(palookup[k])+i);
					for(j=0;j<32;j++)
						ptr[j<<8] = (char)min(max(mulscale10(dist,32-j),0),15);
				}

		if (transluc != NULL)
		{
			for(i=0;i<16;i++)
				for(j=0;j<16;j++)
					transluc[(i<<8)+j] = ((i+j+1)>>1);
		}
	}
}



int setgamemode(char davidoption, int32_t daxdim, int32_t daydim)
{
	strcpy(kensmessage,"!!!! BUILD engine&tools programmed by Ken Silverman of E.G. RI.  (c) Copyright 1995 Ken Silverman.  Summary:  BUILD = Ken. !!!!");
	if (getkensmessagecrc(FP_OFF(kensmessage)) != 0x56c764d4)
		{ setvmode(0x3); printf("Nice try.\n"); exit(0); }
    return(_setgamemode(davidoption, daxdim, daydim));
} /* setgamemode */


void drawline256 (int32_t x1, int32_t y1, int32_t x2, int32_t y2, unsigned char col)
{
	int32_t dx, dy, i, j, inc, plc, daend;
	intptr_t p;

	col = palookup[0][col];

#if defined(POLYMOST) && defined(USE_OPENGL)
	if (rendmode == 3)
	{
		setpolymost2dview();
		glsub_begin(GL_LINES);
		engine_gl_2dcolor(col);
		glsub_vertex((float)x1/4096.0,(float)y1/4096.0,0);
		glsub_vertex((float)x2/4096.0,(float)y2/4096.0,0);
		glsub_end();
		return;
	}
#endif

	dx = x2-x1; dy = y2-y1;
	if (dx >= 0)
	{
		if ((x1 >= wx2) || (x2 < wx1)) return;
		if (x1 < wx1) y1 += scale(wx1-x1,dy,dx), x1 = wx1;
		if (x2 > wx2) y2 += scale(wx2-x2,dy,dx), x2 = wx2;
	}
	else
	{
		if ((x2 >= wx2) || (x1 < wx1)) return;
		if (x2 < wx1) y2 += scale(wx1-x2,dy,dx), x2 = wx1;
		if (x1 > wx2) y1 += scale(wx2-x1,dy,dx), x1 = wx2;
	}
	if (dy >= 0)
	{
		if ((y1 >= wy2) || (y2 < wy1)) return;
		if (y1 < wy1) x1 += scale(wy1-y1,dx,dy), y1 = wy1;
		if (y2 > wy2) x2 += scale(wy2-y2,dx,dy), y2 = wy2;
	}
	else
	{
		if ((y2 >= wy2) || (y1 < wy1)) return;
		if (y2 < wy1) x2 += scale(wy1-y2,dx,dy), y2 = wy1;
		if (y1 > wy2) x1 += scale(wy2-y1,dx,dy), y1 = wy2;
	}

	if (klabs(dx) >= klabs(dy))
	{
		if (dx == 0) return;
		if (dx < 0)
		{
			i = x1; x1 = x2; x2 = i;
			i = y1; y1 = y2; y2 = i;
		}

		inc = divscale12(dy,dx);
		plc = y1+mulscale12((2047-x1)&4095,inc);
		i = ((x1+2048)>>12); daend = ((x2+2048)>>12);
		for(;i<daend;i++)
		{
			j = (plc>>12);
			if ((j >= startumost[i]) && (j < startdmost[i]))
				drawpixel(ylookup[j]+i+frameplace,col);
			plc += inc;
		}
	}
	else
	{
		if (dy < 0)
		{
			i = x1; x1 = x2; x2 = i;
			i = y1; y1 = y2; y2 = i;
		}

		inc = divscale12(dx,dy);
		plc = x1+mulscale12((2047-y1)&4095,inc);
		i = ((y1+2048)>>12); daend = ((y2+2048)>>12);
		p = ylookup[i]+frameplace;
		for(;i<daend;i++)
		{
			j = (plc>>12);
			if ((i >= startumost[j]) && (i < startdmost[j]))
				drawpixel(j+p,col);
			plc += inc; p += ylookup[1];
		}
	}
}

/*
 * This is ryan's change. SDL requires me to call SDL_UpdateRect() to force
 *  vid updates without a SDL_Flip() call, but there's no such thing in the
 *  DOS version of this program, so text won't show up sometimes without
 *  my update call in Linux.  However, to get a nice shadow effect on some
 *  text, Ken draws a string at an offset and darker, and then on top of it
 *  draws the actual string. Two SDL_UpdateRect() calls in over top of each
 *  other cause flicker, so I have this function here so the shadow can
 *  be drawn with _noupdate, and the actual string is draw with an update.
 */
static void __printext256(int32_t xpos, int32_t ypos, short col, short backcol, char name[82], char fontsize, int should_update)
{
	int32_t stx, i, x, y, charxsiz;
	char *fontptr, *letptr, *ptr;

	stx = xpos;

	if (fontsize) { fontptr = smalltextfont; charxsiz = 4; }
				else { fontptr = textfont; charxsiz = 8; }

#if defined(POLYMOST) && defined(USE_OPENGL)
	if (rendmode == 3)
	{
		int32_t xx, yy, lc = -1;

		setpolymost2dview();
		glsub_begin(GL_POINTS);
		for(i=0;name[i];i++)
		{
			letptr = &fontptr[name[i]<<3];
			xx = stx-fontsize;
			yy = ypos+7;
			for(y=7;y>=0;y--)
			{
				for(x=charxsiz-1;x>=0;x--)
				{
					if (letptr[y]&pow2char[7-fontsize-x])
					{
						if (lc != col)
							engine_gl_2dcolor(col);
						lc = col;
						glsub_vertex((float)(xx+x)+0.5f,(float)yy+0.5f,0);
					}
					else if (backcol >= 0)
					{
						if (lc != backcol)
							engine_gl_2dcolor(backcol);
						lc = backcol;
						glsub_vertex((float)(xx+x)+0.5f,(float)yy+0.5f,0);
					}
				}
				yy--;
			}
			stx += charxsiz;
		}
		glsub_end();
		return;
	}
#endif

	for(i=0;name[i];i++)
	{
		letptr = &fontptr[name[i]<<3];
		ptr = (char *)(ylookup[ypos+7]+(stx-fontsize)+frameplace);
		for(y=7;y>=0;y--)
		{
			for(x=charxsiz-1;x>=0;x--)
			{
				if (letptr[y]&pow2char[7-fontsize-x])
					ptr[x] = (char)col;
				else if (backcol >= 0)
					ptr[x] = (char)backcol;
			}
			ptr -= ylookup[1];
		}
		stx += charxsiz;
	}

    if (should_update)
        _updateScreenRect(xpos, ypos, charxsiz * i, 8);
}

void printext256(int32_t xpos, int32_t ypos, short col, short backcol, char name[82], char fontsize)
{
    __printext256(xpos, ypos, col, backcol, name, fontsize, 1);
}

void printext256_noupdate(int32_t xpos, int32_t ypos, short col, short backcol, char name[82], char fontsize)
{
    __printext256(xpos, ypos, col, backcol, name, fontsize, 0);
}

/*
 * JFBuild: the pixel aspect of the mode. Build is built around the
 *  non-square pixels of Mode 13h, so to get things back square on VGA
 *  screens, things need to be "compressed" vertically a little. Modes
 *  wider than 4:3 are widescreen; taller ones (eg. 1280x1024) stretch
 *  the 2D elements vertically a little.
 */
void calcpixelaspect(void)
{
	int32_t ratio;

	widescreen = 0;
	tallscreen = 0;
	if ((xdim == 320 && ydim == 200) || (xdim == 640 && ydim == 400)) {
		pixelaspect = 65536;
	} else {
		ratio = divscale16(ydim*320, xdim*240);
		pixelaspect = divscale16(240*320L,320*200L);

		if (ratio < 65536) {
			widescreen = 1;
		} else if (ratio > 65536) {
			tallscreen = 1;
			pixelaspect = divscale16(ydim*320L,xdim*200L);
		}
	}
}


void setview(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	int32_t i, xfov = 65536;

	if (widescreen) xfov = divscale16(xdim*3, ydim*4);

	windowx1 = x1; wx1 = (x1<<12);
	windowy1 = y1; wy1 = (y1<<12);
	windowx2 = x2; wx2 = ((x2+1)<<12);
	windowy2 = y2; wy2 = ((y2+1)<<12);

	xdimen = (x2-x1)+1; halfxdimen = (xdimen>>1);
	xdimenrecip = divscale32(1L,xdimen);
	ydimen = (y2-y1)+1;

	setaspect(mulscale16(65536,xfov), pixelaspect);

	for(i=0;i<windowx1;i++) { startumost[i] = 1, startdmost[i] = 0; }
	for(i=windowx1;i<=windowx2;i++)
		{ startumost[i] = windowy1, startdmost[i] = windowy2+1; }
	for(i=windowx2+1;i<xdim;i++) { startumost[i] = 1, startdmost[i] = 0; }

	viewoffset = windowy1*bytesperline + windowx1;

	if ((stereomode) || (vidoption == 6))
	{
		ostereopixelwidth = stereopixelwidth;
		xdimen = (windowx2-windowx1+1)+(stereopixelwidth<<1); halfxdimen = (xdimen>>1);
		xdimenrecip = divscale32(1L,xdimen);
		setaspect((int32_t)divscale16(xdimen,windowx2-windowx1+1),yxaspect);
	}
}


void setaspect(int32_t daxrange, int32_t daaspect)
{
	viewingrange = daxrange;
	viewingrangerecip = divscale32(1L,daxrange);

	yxaspect = daaspect;
	xyaspect = divscale32(1,yxaspect);
	xdimenscale = scale(xdimen,yxaspect,320);
	xdimscale = scale(320,xyaspect,xdimen);
	ydimenscale = scale(ydimen,yxaspect,mulscale16(200,pixelaspect));
}


static int getclosestcol(int32_t r, int32_t g, int32_t b)
{
	int32_t i, j, k, dist, mindist, retcol;
	char *pal1;

	j = (r>>3)*FASTPALGRIDSIZ*FASTPALGRIDSIZ+(g>>3)*FASTPALGRIDSIZ+(b>>3)+FASTPALGRIDSIZ*FASTPALGRIDSIZ+FASTPALGRIDSIZ+1;
	mindist = min(rdist[coldist[r&7]+64+8],gdist[coldist[g&7]+64+8]);
	mindist = min(mindist,bdist[coldist[b&7]+64+8]);
	mindist++;

	r = 64-r; g = 64-g; b = 64-b;

	retcol = -1;
	for(k=26;k>=0;k--)
	{
		i = colscan[k]+j; if ((colhere[i>>3]&pow2char[i&7]) == 0) continue;
		i = colhead[i];
		do
		{
			pal1 = (char *)&palette[i*3];
			dist = gdist[pal1[1]+g];
			if (dist < mindist)
			{
				dist += rdist[pal1[0]+r];
				if (dist < mindist)
				{
					dist += bdist[pal1[2]+b];
					if (dist < mindist) { mindist = dist; retcol = i; }
				}
			}
			i = colnext[i];
		} while (i >= 0);
	}
	if (retcol >= 0) return(retcol);

	mindist = 0x7fffffff;
	pal1 = (char *)&palette[768-3];
	for(i=255;i>=0;i--,pal1-=3)
	{
		dist = gdist[pal1[1]+g]; if (dist >= mindist) continue;
		dist += rdist[pal1[0]+r]; if (dist >= mindist) continue;
		dist += bdist[pal1[2]+b]; if (dist >= mindist) continue;
		mindist = dist; retcol = i;
	}
	return(retcol);
}


void makepalookup(int32_t palnum, char *remapbuf, signed char r,
                  signed char g, signed char b, char dastat)
{
	int32_t i, j, dist, palscale;
	char *ptr, *ptr2;

	if (paletteloaded == 0) return;

	if (palookup[palnum] == NULL)
	{
			/* Allocate palookup buffer */
		if ((palookup[palnum] = (char *)kkmalloc(numpalookups<<8)) == NULL)
			allocache((intptr_t *)&palookup[palnum],numpalookups<<8,&permanentlock);
	}

#if defined(POLYMOST) && defined(USE_OPENGL)
	engine_gl_lookupchanged();  /* renderer 3's GL 2.1 tier uploads them again */
#endif

	if (dastat == 0) return;
	if ((r|g|b|63) != 63) return;

	if ((r|g|b) == 0)
	{
		for(i=0;i<256;i++)
		{
			ptr = (char *)(FP_OFF(palookup[0])+remapbuf[i]);
			ptr2 = (char *)(FP_OFF(palookup[palnum])+i);
			for(j=0;j<numpalookups;j++)
				{ *ptr2 = *ptr; ptr += 256; ptr2 += 256; }
		}
	}
	else
	{
		ptr2 = (char *)FP_OFF(palookup[palnum]);
		for(i=0;i<numpalookups;i++)
		{
			palscale = divscale16(i,numpalookups);
			for(j=0;j<256;j++)
			{
				ptr = (char *)&palette[remapbuf[j]*3];
				*ptr2++ = getclosestcol((int32_t)ptr[0]+mulscale16(r-ptr[0],palscale),
												(int32_t)ptr[1]+mulscale16(g-ptr[1],palscale),
												(int32_t)ptr[2]+mulscale16(b-ptr[2],palscale));
			}
		}
	}

	if ((vidoption == 6) && (qsetmode == 200))
	{
		for(i=0;i<256;i++)
		{
			dist = palette[i*3]*3+palette[i*3+1]*5+palette[i*3+2]*2;
			ptr = (char *)(FP_OFF(palookup[palnum])+i);
			for(j=0;j<32;j++)
				ptr[j<<8] = (char)min(max(mulscale10(dist,32-j),0),15);
		}
	}
}


	/* the last palette passed to setbrightness, which setpalettefade blends. */
static unsigned char basepal[768];

static void applybrightness(unsigned char *dapal)
{
	int32_t i, j, k;

	k = 0;
	if (vidoption == 6)
	{
		for(j=0;j<16;j++)
			for(i=0;i<16;i++)
			{
				palettebuf[k++] = britable[curbrightness][j<<2];
				palettebuf[k++] = 0;
				palettebuf[k++] = britable[curbrightness][i<<2];
				palettebuf[k++] = 0;
			}
	}
	else
	{
		for(i=0;i<256;i++)
		{
			palettebuf[k++] = britable[curbrightness][dapal[i*3+2]];
			palettebuf[k++] = britable[curbrightness][dapal[i*3+1]];
			palettebuf[k++] = britable[curbrightness][dapal[i*3+0]];
			palettebuf[k++] = 0;
		}
	}

	VBE_setPalette(0, 256, (char *) palettebuf);
}


void setbrightness(char dabrightness, unsigned char *dapal)
{
	curbrightness = min(max((int32_t)dabrightness,0),15);
	if (dapal != basepal) memcpy(basepal, dapal, sizeof(basepal));
#ifdef POLYMOST
	palfadergb.r = palfadergb.g = palfadergb.b = 0;
	palfadedelta = 0;
#endif
	applybrightness(basepal);
}


/*
 * setpalettefade
 *  Fades toward (r,g,b), 0-63 each, by offset/64. Renderer 3 draws it as a
 *  quad over the frame; the others blend the palette as the game's palto does.
 */
void setpalettefade(unsigned char r, unsigned char g, unsigned char b, unsigned char offset)
{
	unsigned char fadepal[768];
	int32_t i;

#ifdef POLYMOST
	palfadergb.r = r<<2; palfadergb.g = g<<2; palfadergb.b = b<<2;
	palfadedelta = (unsigned char)min(offset<<2,255);
#ifdef USE_OPENGL
	if (rendmode == 3) { _glpalettefade(); return; }
#endif
#endif

	for(i=0;i<768;i+=3)
	{
		fadepal[i+0] = basepal[i+0]+((((int32_t)r-(int32_t)basepal[i+0])*(int32_t)offset)>>6);
		fadepal[i+1] = basepal[i+1]+((((int32_t)g-(int32_t)basepal[i+1])*(int32_t)offset)>>6);
		fadepal[i+2] = basepal[i+2]+((((int32_t)b-(int32_t)basepal[i+2])*(int32_t)offset)>>6);
	}
	applybrightness(fadepal);
}


#if defined(POLYMOST) && defined(USE_OPENGL)
/* The palette setbrightness was last given (getfadedpalette fades it), for
   renderer 3's resize to keep across the mode's setup, which resets it. */
unsigned char *getbasepalette(void)
{
	return(basepal);
}
#endif


#if defined(POLYMOST) && defined(USE_OPENGL)
/*
 * getfadedpalette
 *  The palette renderer 3's GL 2.1 tier shows: curpalette, or with a fade,
 *  the base palette faded as the 8-bit one is above, made 8-bit as
 *  VBE_setPalette does. palfadedelta keeps offsets up to 64.
 */
void getfadedpalette(palette_t *pal)
{
	int32_t i, offset;
	unsigned char c[3];

	if (!palfadedelta) { memcpy(pal,curpalette,sizeof(palette_t)*256); return; }
	offset = (palfadedelta == 255) ? 64 : (palfadedelta>>2);
	for(i=0;i<256;i++)
	{
		c[0] = basepal[i*3+0]+((((int32_t)(palfadergb.r>>2)-(int32_t)basepal[i*3+0])*offset)>>6);
		c[1] = basepal[i*3+1]+((((int32_t)(palfadergb.g>>2)-(int32_t)basepal[i*3+1])*offset)>>6);
		c[2] = basepal[i*3+2]+((((int32_t)(palfadergb.b>>2)-(int32_t)basepal[i*3+2])*offset)>>6);
		c[0] = britable[curbrightness][c[0]&63];
		c[1] = britable[curbrightness][c[1]&63];
		c[2] = britable[curbrightness][c[2]&63];
		pal[i].r = (c[0]<<2)|(c[0]>>4);
		pal[i].g = (c[1]<<2)|(c[1]>>4);
		pal[i].b = (c[2]<<2)|(c[2]>>4);
		pal[i].f = 0;
	}
}
#endif


void clearview(int32_t dacol)
{
	int32_t y, dx;
	intptr_t p;

	if (qsetmode != 200) return;

#if defined(POLYMOST) && defined(USE_OPENGL)
	if (rendmode == 3)
	{
		if ((engine_gl_indexedtarget()))  /* GL 2.1: the index (but for texture filtering's colour) */
			glClearColor((float)dacol/255.f,0,0,0);
		else
			glClearColor(curpalette[dacol].r/255.0,curpalette[dacol].g/255.0,curpalette[dacol].b/255.0,0);
		/* only the view window, as below (JFBuild clears it all) */
		glScissor(windowx1,yres-(windowy2+1),windowx2-windowx1+1,windowy2-windowy1+1);
		glEnable(GL_SCISSOR_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
		_glframedrawn();
		return;
	}
#endif

	dx = windowx2-windowx1+1;
	dacol += (dacol<<8); dacol += (dacol<<16);
	if (vidoption == 6)
	{
		p = FP_OFF(screen)+ylookup[windowy1]+windowx1;
		for(y=windowy1;y<=windowy2;y++)
		{
			clearbufbyte((void *)p,dx,dacol);
                        clearbufbyte((void *)(p+65536),dx,dacol);
			p += ylookup[1];
		}
		faketimerhandler();
		return;
	}
	p = frameplace+ylookup[windowy1]+windowx1;
	for(y=windowy1;y<=windowy2;y++)
		{ clearbufbyte((void *)p,dx,dacol); p += ylookup[1]; }
	faketimerhandler();
}


void clearallviews(int32_t dacol)
{
	int32_t i;

	if (qsetmode != 200) return;

#if defined(POLYMOST) && defined(USE_OPENGL)
	if (rendmode == 3)
	{
		if ((engine_gl_indexedtarget()))  /* GL 2.1: the index (but for texture filtering's colour) */
			glClearColor((float)dacol/255.f,0,0,0);
		else
			glClearColor(curpalette[dacol].r/255.0,curpalette[dacol].g/255.0,curpalette[dacol].b/255.0,0);
		glViewport(0,0,xdim,ydim); engine_gl_resetview();
		glClear(GL_COLOR_BUFFER_BIT);
		_glframedrawn();
		return;
	}
#endif

	dacol += (dacol<<8); dacol += (dacol<<16);

	switch(vidoption)
	{
		case 1:
			for(i=0;i<numpages;i++)
			{
				setactivepage(i);
				clearbufbyte((void *)frameplace,imageSize,0L);
			}
			setactivepage(activepage);
		case 2:
			clearbuf((void *)frameplace,(xdim*ydim)>>2,0L);
			break;
		case 6:
			clearbuf(screen,128000L>>2,dacol);
			break;
	}
	faketimerhandler();
}


void plotpixel(int32_t x, int32_t y, char col)
{
#if defined(POLYMOST) && defined(USE_OPENGL)
	if (rendmode == 3)
	{
		setpolymost2dview();
		glsub_begin(GL_POINTS);
		engine_gl_2dcolor((unsigned char)col);
		glsub_vertex((float)x+0.5f,(float)y+0.5f,0);
		glsub_end();
		return;
	}
#endif
	drawpixel(ylookup[y]+x+frameplace,(int32_t)col);
}


unsigned char getpixel(int32_t x, int32_t y)
{
#if defined(POLYMOST) && defined(USE_OPENGL)
	if (rendmode == 3) return 0;
#endif
	return(readpixel(ylookup[y]+x+frameplace));
}

	/* MUST USE RESTOREFORDRAWROOMS AFTER DRAWING */
static int32_t setviewcnt = 0;
static int32_t bakvidoption[4];
static intptr_t bakframeplace[4];
static int32_t bakxsiz[4], bakysiz[4];

static int32_t bakwindowx1[4], bakwindowy1[4];
static int32_t bakwindowx2[4], bakwindowy2[4];
#ifdef POLYMOST
static int32_t bakrendmode, baktile;
#endif


void setviewtotile(short tilenume, int32_t xsiz, int32_t ysiz)
{
	int32_t i, j;

		/* DRAWROOMS TO TILE BACKUP&SET CODE */
	tilesizx[tilenume] = xsiz; tilesizy[tilenume] = ysiz;
		/* and its picsiz, as allocatepermanenttile, for Polymost */
	j = 15; while ((j > 1) && (pow2long[j] > xsiz)) j--;
	picsiz[tilenume] = ((char)j);
	j = 15; while ((j > 1) && (pow2long[j] > ysiz)) j--;
	picsiz[tilenume] += ((char)(j<<4));
	bakxsiz[setviewcnt] = xsiz; bakysiz[setviewcnt] = ysiz;
	bakvidoption[setviewcnt] = vidoption; vidoption = 2;
	bakframeplace[setviewcnt] = frameplace; frameplace = waloff[tilenume];
	bakwindowx1[setviewcnt] = windowx1; bakwindowy1[setviewcnt] = windowy1;
	bakwindowx2[setviewcnt] = windowx2; bakwindowy2[setviewcnt] = windowy2;
#ifdef POLYMOST
	if (setviewcnt == 0) {
		bakrendmode = rendmode;
		baktile = tilenume;
	}
	rendmode = 0;
#endif
	copybufbyte(&startumost[windowx1],&bakumost[windowx1],(windowx2-windowx1+1)*sizeof(bakumost[0]));
	copybufbyte(&startdmost[windowx1],&bakdmost[windowx1],(windowx2-windowx1+1)*sizeof(bakdmost[0]));
	setview(0,0,ysiz-1,xsiz-1);
	setaspect(65536,65536);
	j = 0; for(i=0;i<=xsiz;i++) { ylookup[i] = j, j += ysiz; }
	setvlinebpl(ysiz);
	setviewcnt++;
}


void setviewback(void)
{
	int32_t i, j, k;

	if (setviewcnt <= 0) return;
	setviewcnt--;

#ifdef POLYMOST
	if (setviewcnt == 0) {
		rendmode = bakrendmode;
		invalidatetile(baktile,-1,-1);
	}
#endif

	setview(bakwindowx1[setviewcnt],bakwindowy1[setviewcnt],
			  bakwindowx2[setviewcnt],bakwindowy2[setviewcnt]);
	copybufbyte(&bakumost[windowx1],&startumost[windowx1],(windowx2-windowx1+1)*sizeof(startumost[0]));
	copybufbyte(&bakdmost[windowx1],&startdmost[windowx1],(windowx2-windowx1+1)*sizeof(startdmost[0]));
	vidoption = bakvidoption[setviewcnt];
	frameplace = bakframeplace[setviewcnt];
	if (setviewcnt == 0)
		k = bakxsiz[0];
	else
		k = max(bakxsiz[setviewcnt-1],bakxsiz[setviewcnt]);
	j = 0; for(i=0;i<=k;i++) ylookup[i] = j, j += bytesperline;
	setvlinebpl(bytesperline);
}




