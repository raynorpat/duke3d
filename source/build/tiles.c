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
#include "engine_internal.h"

/* set this to something non-zero to get loadtile() debugging info on stderr. */
#define BUILD_CACHEDEBUG 0
#define MAXTILEFILES 256

int32_t artversion;
char *pic = NULL;
char picsiz[MAXTILES], tilefilenum[MAXTILES];
int32_t tilefileoffs[MAXTILES];

int32_t artsize = 0, cachesize = 0;

#ifdef SUPERBUILD
intptr_t voxoff[MAXVOXELS][MAXVOXMIPS];
int32_t voxlock[MAXVOXELS][MAXVOXMIPS];
#endif

static char artfilename[20];
static int32_t numtilefiles, artfil = -1, artfilnum, artfilplc;



/*
================
setgotpic

Mark a tile in the usage bitmap and refresh its evictable cache lock.
Locks at 200 or above are pinned and must not be downgraded.
================
*/
        void setgotpic(uint32_t param)
        {
	    unsigned char *gotpic_a = (unsigned char*)gotpic;
	    unsigned char *walock_a = (unsigned char*)walock;
	    unsigned char *pow2char_a = (unsigned char*)pow2char;
	    
	    if (walock_a[param] < 200) walock_a[param] = 199;

	    gotpic_a[param>>3] |= pow2char_a[param&7];
        }


/*
================
animateoffs

Select a tile animation frame from the locked render clock and picanm flags.
Return a signed offset for oscillating, forward or backward animation.
================
*/
int animateoffs(short tilenum, short fakevar)
{
	int32_t i, k, offs;

	offs = 0;
	/* Low picanm bits give the frame count; the high-byte shift controls playback speed. */
	i = (totalclocklock>>((picanm[tilenum]>>24)&15));
	if ((picanm[tilenum]&63) > 0)
	{
		switch(picanm[tilenum]&192)
		{
			case 64:
				k = (i%((picanm[tilenum]&63)<<1));
				if (k < (picanm[tilenum]&63))
					offs = k;
				else
					offs = (((picanm[tilenum]&63)<<1)-k);
				break;
			case 128:
				offs = (i%((picanm[tilenum]&63)+1));
					break;
			case 192:
				offs = -(i%((picanm[tilenum]&63)+1));
		}
	}
	return(offs);
}

/*
================
loadtile

Load a tile's column-major pixels from its ART file into the cache.
Keep the current ART handle and file position so nearby reads avoid reopening it.
================
*/
void loadtile(short tilenume)
{
	char *ptr;
	int32_t i, dasiz;

	if ((unsigned)tilenume >= (unsigned)MAXTILES) return;
	dasiz = tilesizx[tilenume]*tilesizy[tilenume];
	if (dasiz <= 0) return;

	i = tilefilenum[tilenume];
	if (i != artfilnum)
	{
		if (artfil != -1) kclose(artfil);
		artfilnum = i;
		artfilplc = 0L;

		artfilename[7] = (i%10)+48;
		artfilename[6] = ((i/10)%10)+48;
		artfilename[5] = ((i/100)%10)+48;
		artfil = kopen4load(artfilename,0);
		faketimerhandler();
	}

	#if BUILD_CACHEDEBUG
        fprintf(stderr, "BUILDCACHE: Tile:%d\n", tilenume);
    #endif

	if (waloff[tilenume] == 0)
	{
		walock[tilenume] = 199;
		allocache(&waloff[tilenume],dasiz,(unsigned char *) &walock[tilenume]);
	}

	if (artfilplc != tilefileoffs[tilenume])
	{
		klseek(artfil,tilefileoffs[tilenume]-artfilplc,SEEK_CUR);
		faketimerhandler();
	}
	ptr = (char *)waloff[tilenume];
	kread(artfil,ptr,dasiz);
	faketimerhandler();
	artfilplc = tilefileoffs[tilenume]+dasiz;
}


/*
================
allocatepermanenttile

Allocate a pinned cache tile for generated pictures such as camera views.
Set dimensions and packed size exponents without assigning an ART animation.
================
*/
intptr_t allocatepermanenttile(short tilenume, int32_t xsiz, int32_t ysiz)
{
	int32_t j, dasiz;

	if ((xsiz <= 0) || (ysiz <= 0) || ((unsigned)tilenume >= (unsigned)MAXTILES))
		return(0);

	dasiz = xsiz*ysiz;

	/* Generated art must survive ordinary cache aging and eviction. */
	walock[tilenume] = 255;
	allocache(&waloff[tilenume],dasiz,(unsigned char *) &walock[tilenume]);

	tilesizx[tilenume] = xsiz;
	tilesizy[tilenume] = ysiz;
	picanm[tilenume] = 0;

	j = 15; while ((j > 1) && (pow2long[j] > xsiz)) j--;
	picsiz[tilenume] = ((char)j);
	j = 15; while ((j > 1) && (pow2long[j] > ysiz)) j--;
	picsiz[tilenume] += ((char)(j<<4));

	return(waloff[tilenume]);
}


/*
================
loadpics

Index the ART files and allocate the shared tile-cache arena.
Only metadata is loaded here; pixel data is read later by loadtile.
================
*/
int loadpics(char *filename)
{
	int32_t offscount, localtilestart, localtileend, dasiz;
	short fil, i, j, k;

	strcpy(artfilename,filename);

	for(i=0;i<MAXTILES;i++)
	{
		tilesizx[i] = 0;
		tilesizy[i] = 0;
		picanm[i] = 0L;
	}

	artsize = 0L;

	numtilefiles = 0;
	do
	{
		k = numtilefiles;

		artfilename[7] = (k%10)+48;
		artfilename[6] = ((k/10)%10)+48;
		artfilename[5] = ((k/100)%10)+48;
		if ((fil = kopen4load(artfilename,0)) != -1)
		{
			kread32(fil,&artversion);
			if (artversion != 1) return(-1);

			kread32(fil,&numtiles);
			kread32(fil,&localtilestart);
			kread32(fil,&localtileend);

			/*kread(fil,&tilesizx[localtilestart],(localtileend-localtilestart+1)<<1);*/
			for (i = localtilestart; i <= localtileend; i++)
				kread16(fil,&tilesizx[i]);

			/*kread(fil,&tilesizy[localtilestart],(localtileend-localtilestart+1)<<1);*/
			for (i = localtilestart; i <= localtileend; i++)
			    kread16(fil,&tilesizy[i]);

			/*kread(fil,&picanm[localtilestart],(localtileend-localtilestart+1)<<2);*/
			for (i = localtilestart; i <= localtileend; i++)
			    kread32(fil,&picanm[i]);

			/* ART stores dimensions and animation words before the concatenated tile pixels. */
			offscount = 4+4+4+4+((localtileend-localtilestart+1)<<3);
			for(i=localtilestart;i<=localtileend;i++)
			{
				tilefilenum[i] = k;
				tilefileoffs[i] = offscount;
				dasiz = (int32_t)(tilesizx[i]*tilesizy[i]);
				offscount += dasiz;
				artsize += ((dasiz+15)&0xfffffff0);
			}
			kclose(fil);

			numtilefiles++;
		}
	}
	while (k != numtilefiles);

	clearbuf(&gotpic[0],(int32_t)((MAXTILES+31)>>5),0L);

	/* try dpmi_DETERMINEMAXREALALLOC! */

	cachesize = max(artsize,1048576);
	while ((pic = (char *)malloc(cachesize)) == NULL)
	{
		cachesize -= 65536L;
		if (cachesize < 65536) return(-1);
	}
	/* Align the arena start upward and trim the usable length to the same 16-byte grid. */
	initcache((((intptr_t)(pic))+15)&~(intptr_t)15,(cachesize-(int32_t)((-((intptr_t)(pic)))&15))&0xfffffff0);

	for(i=0;i<MAXTILES;i++)
	{
		j = 15;
		while ((j > 1) && (pow2long[j] > tilesizx[i])) j--;
		/* Pack x and y size exponents into the low and high nibbles for the span drawers. */
		picsiz[i] = ((char)j);
		j = 15;
		while ((j > 1) && (pow2long[j] > tilesizy[i])) j--;
		picsiz[i] += ((char)(j<<4));
	}

	artfil = -1;
	artfilnum = -1;
	artfilplc = 0L;

	return(0);
}

#ifdef SUPERBUILD
void qloadkvx(int32_t voxindex, char *filename)
{
	int32_t i, fil, dasiz, lengcnt, lengtot;
	char *ptr;

	if ((fil = kopen4load(filename,0)) == -1) return;

	lengcnt = 0;
	lengtot = kfilelength(fil);

	for(i=0;i<MAXVOXMIPS;i++)
	{
		kread32(fil,&dasiz);

			/* Must store filenames to use cacheing system :( */
		voxlock[voxindex][i] = BUILDSWAP_INTEL32(200);
		allocache(&voxoff[voxindex][i],dasiz,(unsigned char *)&voxlock[voxindex][i]);
		ptr = (char *)voxoff[voxindex][i];
		kread(fil,ptr,dasiz);

		lengcnt += dasiz+4;
		if (lengcnt >= lengtot-768) break;
	}
	kclose(fil);
}
#endif


/*
================
copytilepiece

Copy nontransparent pixels between column-major tiles.
Wrap the source coordinates while clipping writes to the destination dimensions.
================
*/
void copytilepiece(int32_t tilenume1, int32_t sx1, int32_t sy1, int32_t xsiz, int32_t ysiz,
                   int32_t tilenume2, int32_t sx2, int32_t sy2)
{
	unsigned char *ptr1, *ptr2, dat;
	int32_t xsiz1, ysiz1, xsiz2, ysiz2, i, j, x1, y1, x2, y2;

	xsiz1 = tilesizx[tilenume1]; ysiz1 = tilesizy[tilenume1];
	xsiz2 = tilesizx[tilenume2]; ysiz2 = tilesizy[tilenume2];
	if ((xsiz1 > 0) && (ysiz1 > 0) && (xsiz2 > 0) && (ysiz2 > 0))
	{
		if (waloff[tilenume1] == 0) loadtile((short) tilenume1);
		if (waloff[tilenume2] == 0) loadtile((short) tilenume2);

		x1 = sx1;
		for(i=0;i<xsiz;i++)
		{
			y1 = sy1;
			for(j=0;j<ysiz;j++)
			{
				x2 = sx2+i;
				y2 = sy2+j;
				if ((x2 >= 0) && (y2 >= 0) && (x2 < xsiz2) && (y2 < ysiz2))
				{
					ptr1 = (unsigned char *) (waloff[tilenume1] + x1*ysiz1 + y1);
					ptr2 = (unsigned char *) (waloff[tilenume2] + x2*ysiz2 + y2);
					dat = *ptr1;
					if (dat != 255)
						*ptr2 = *ptr1;
				}

				y1++; if (y1 >= ysiz1) y1 = 0;
			}
			x1++; if (x1 >= xsiz1) x1 = 0;
		}
	}
}

/*
================
squarerotatetile

Transpose a square tile in place with paired swaps across its diagonal.
The operation is skipped for nonsquare tiles.
================
*/
void squarerotatetile(short tilenume)
{
	int32_t i, j, k, xsiz, ysiz;
	unsigned char *ptr1, *ptr2;

	xsiz = tilesizx[tilenume]; ysiz = tilesizy[tilenume];

		/* supports square tiles only for rotation part */
	if (xsiz == ysiz)
	{
		k = (xsiz<<1);
		for(i=xsiz-1;i>=0;i--)
		{
			ptr1 = (unsigned char *) (waloff[tilenume]+i*(xsiz+1));
            ptr2 = ptr1;
			if ((i&1) != 0) { ptr1--; ptr2 -= xsiz; swapchar(ptr1,ptr2); }
			for(j=(i>>1)-1;j>=0;j--)
				{ ptr1 -= 2; ptr2 -= k; swapchar2(ptr1,ptr2,xsiz); }
		}
	}
}

/*
================
setsprite

Update a sprite's XYZ and relink its sector list when the XY position crosses a boundary.
A failed sector lookup returns -1 after the position has already been changed.
================
*/
int setsprite(short spritenum, int32_t newx, int32_t newy, int32_t newz)
{
	short tempsectnum;

	sprite[spritenum].x = newx;
	sprite[spritenum].y = newy;
	sprite[spritenum].z = newz;

	tempsectnum = sprite[spritenum].sectnum;
	updatesector(newx,newy,&tempsectnum);
	if (tempsectnum < 0)
		return(-1);
	if (tempsectnum != sprite[spritenum].sectnum)
		changespritesect(spritenum,tempsectnum);

	return(0);
}


/*
================
initspritelists

Put every sprite slot on the sector and status free lists.
The extra MAXSECTORS/MAXSTATUS heads are free-list sentinels, not live buckets.
================
*/
void initspritelists(void)
{
	int32_t i;

	for (i=0;i<MAXSECTORS;i++)     /* Init doubly-linked sprite sector lists */
		headspritesect[i] = -1;
	headspritesect[MAXSECTORS] = 0;
	for(i=0;i<MAXSPRITES;i++)
	{
		prevspritesect[i] = i-1;
		nextspritesect[i] = i+1;
		sprite[i].sectnum = MAXSECTORS;
	}
	prevspritesect[0] = -1;
	nextspritesect[MAXSPRITES-1] = -1;


	for(i=0;i<MAXSTATUS;i++)      /* Init doubly-linked sprite status lists */
		headspritestat[i] = -1;
	headspritestat[MAXSTATUS] = 0;
	for(i=0;i<MAXSPRITES;i++)
	{
		prevspritestat[i] = i-1;
		nextspritestat[i] = i+1;
		sprite[i].statnum = MAXSTATUS;
	}
	prevspritestat[0] = -1;
	nextspritestat[MAXSPRITES-1] = -1;
}


/*
================
insertsprite

Pair status and sector insertions for a new sprite slot.
The two free lists must agree on the next unused index.
================
*/
int insertsprite(short sectnum, short statnum)
{
	insertspritestat(statnum);
	return(insertspritesect(sectnum));
}


/*
================
insertspritesect

Take the first sector-free slot and link it at the requested sector's head.
Update both directions of the chain and the sprite's sectnum.
================
*/
int insertspritesect(short sectnum)
{
	short blanktouse;

	if ((sectnum >= MAXSECTORS) || (headspritesect[MAXSECTORS] == -1))
		return(-1);  /* list full */

	blanktouse = headspritesect[MAXSECTORS];

	headspritesect[MAXSECTORS] = nextspritesect[blanktouse];
	if (headspritesect[MAXSECTORS] >= 0)
		prevspritesect[headspritesect[MAXSECTORS]] = -1;

	prevspritesect[blanktouse] = -1;
	nextspritesect[blanktouse] = headspritesect[sectnum];
	if (headspritesect[sectnum] >= 0)
		prevspritesect[headspritesect[sectnum]] = blanktouse;
	headspritesect[sectnum] = blanktouse;

	sprite[blanktouse].sectnum = sectnum;

	return(blanktouse);
}


/*
================
insertspritestat

Take the first status-free slot and link it at the requested status head.
The parallel sector insertion is performed separately.
================
*/
int insertspritestat(short statnum)
{
	short blanktouse;

	if ((statnum >= MAXSTATUS) || (headspritestat[MAXSTATUS] == -1))
		return(-1);  /* list full */

	blanktouse = headspritestat[MAXSTATUS];

	headspritestat[MAXSTATUS] = nextspritestat[blanktouse];
	if (headspritestat[MAXSTATUS] >= 0)
		prevspritestat[headspritestat[MAXSTATUS]] = -1;

	prevspritestat[blanktouse] = -1;
	nextspritestat[blanktouse] = headspritestat[statnum];
	if (headspritestat[statnum] >= 0)
		prevspritestat[headspritestat[statnum]] = blanktouse;
	headspritestat[statnum] = blanktouse;

	sprite[blanktouse].statnum = statnum;

	return(blanktouse);
}


/*
================
deletesprite

Return a sprite slot to both free lists.
Removing it from only one list would leave its sector and status ownership inconsistent.
================
*/
int deletesprite(short spritenum)
{
	deletespritestat(spritenum);
	return(deletespritesect(spritenum));
}


/*
================
deletespritesect

Unlink a sprite from its sector chain and push it onto the sector free list.
Its status chain is handled separately.
================
*/
int deletespritesect(short deleteme)
{
	if (sprite[deleteme].sectnum == MAXSECTORS)
		return(-1);

	if (headspritesect[sprite[deleteme].sectnum] == deleteme)
		headspritesect[sprite[deleteme].sectnum] = nextspritesect[deleteme];

	if (prevspritesect[deleteme] >= 0) nextspritesect[prevspritesect[deleteme]] = nextspritesect[deleteme];
	if (nextspritesect[deleteme] >= 0) prevspritesect[nextspritesect[deleteme]] = prevspritesect[deleteme];

	if (headspritesect[MAXSECTORS] >= 0) prevspritesect[headspritesect[MAXSECTORS]] = deleteme;
	prevspritesect[deleteme] = -1;
	nextspritesect[deleteme] = headspritesect[MAXSECTORS];
	headspritesect[MAXSECTORS] = deleteme;

	sprite[deleteme].sectnum = MAXSECTORS;
	return(0);
}


/*
================
deletespritestat

Unlink a sprite from its status chain and push it onto the status free list.
Its sector chain is handled separately.
================
*/
int deletespritestat(short deleteme)
{
	if (sprite[deleteme].statnum == MAXSTATUS)
		return(-1);

	if (headspritestat[sprite[deleteme].statnum] == deleteme)
		headspritestat[sprite[deleteme].statnum] = nextspritestat[deleteme];

	if (prevspritestat[deleteme] >= 0) nextspritestat[prevspritestat[deleteme]] = nextspritestat[deleteme];
	if (nextspritestat[deleteme] >= 0) prevspritestat[nextspritestat[deleteme]] = prevspritestat[deleteme];

	if (headspritestat[MAXSTATUS] >= 0) prevspritestat[headspritestat[MAXSTATUS]] = deleteme;
	prevspritestat[deleteme] = -1;
	nextspritestat[deleteme] = headspritestat[MAXSTATUS];
	headspritestat[MAXSTATUS] = deleteme;

	sprite[deleteme].statnum = MAXSTATUS;
	return(0);
}


/*
================
changespritesect

Relink a live sprite into a different sector bucket.
Delete-then-insert reuses the just-freed index and leaves the status chain alone.
================
*/
int changespritesect(short spritenum, short newsectnum)
{
	if ((newsectnum < 0) || (newsectnum > MAXSECTORS)) return(-1);
	if (sprite[spritenum].sectnum == newsectnum) return(0);
	if (sprite[spritenum].sectnum == MAXSECTORS) return(-1);
	if (deletespritesect(spritenum) < 0) return(-1);
	insertspritesect(newsectnum);
	return(0);
}


/*
================
changespritestat

Relink a live sprite into a different status bucket.
Delete-then-insert reuses the just-freed index and leaves the sector chain alone.
================
*/
int changespritestat(short spritenum, short newstatnum)
{
	if ((newstatnum < 0) || (newstatnum > MAXSTATUS)) return(-1);
	if (sprite[spritenum].statnum == newstatnum) return(0);
	if (sprite[spritenum].statnum == MAXSTATUS) return(-1);
	if (deletespritestat(spritenum) < 0) return(-1);
	insertspritestat(newstatnum);
	return(0);
}


void engine_tiles_uninit(void)
{
	if (pic != NULL) { free(pic); pic = NULL; }
	if (artfil != -1) kclose(artfil);
}
