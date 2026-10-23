/*
 * Inline assembly.
 *
 * Initial PLATFORM_UNIX work done by Andrew Henderson. The DOS stuff is
 *  Ken's original code, and was in pragmas.h
 *
 * Please do NOT harrass Ken Silverman about any code modifications
 *  (including this file) to BUILD.
 */

/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */

#include <stdint.h>
#include <string.h>	// memcpy
#include <stdio.h>
#include "platform.h"
#include "pragmas.h"
#include "build.h"
#include "cache1d.h"
#include "engine.h"
#include "display.h"
#include "engine_internal.h"

#define low32(a) ((a&0xffffffff))

uint32_t getkensmessagecrc(intptr_t param) {
    return(0x56c764d4);
}

int32_t msqrtasm(int i1)
{
    int retval = 0x40000000;
    int temp = 0x20000000;

    while (temp != 0)
    {
	    if (i1 >= retval)
	    {
		    i1 -= retval;
		    retval += temp*4;
	    }
	    retval -= temp;
	    retval >>= 1;
	    temp >>= 2;
    }

    if (i1 >= retval) retval--;
    retval >>= 1;
    return(retval);
}


void clearbuf(void *buffer, int size, int32_t fill_value) {
  int i;
  for (i = 0; i < size; i++) ((int32_t*)buffer)[i] = fill_value;
}

void clearbufbyte(void *vpbuffer, int size, int32_t fill_value) {
  unsigned char *buffer = (unsigned char *) vpbuffer;
  int lsize;
  switch(size){
  case 0: return;
  case 1: *buffer = fill_value; return;
  case 2: *((unsigned short*)buffer) = fill_value; return;
  case 3: { buffer[2]=buffer[1]=buffer[0] = fill_value;} return;
  default:
	if ((intptr_t)buffer&1) {
		*buffer = fill_value; size--;
        buffer++;
	}
	if ((intptr_t)buffer&2) {
		*((unsigned short*)buffer) = fill_value; size-=2;
        buffer += 2;
	}
	lsize = size>>2;
	while(lsize) {
		*((unsigned int*)buffer) = fill_value;
		lsize--;
        buffer += 4;
	}
	if (size&2) {
		*((unsigned short*)buffer) = fill_value;
        buffer += 2;
	}
	if (size&1) {
		*((unsigned char*)buffer) = fill_value;
        buffer++;
	}
  }
}

void copybuf(void *source, void *dest, int size) {
  int i;
  for (i = 0; i < size; i++) ((int32_t*)dest)[i] = ((int32_t*)source)[i];
}

void copybufbyte(void *source, void *dest, int size) {
  memcpy(dest,source,size);
}

void copybufreverse(void *source, void *dest, int size) {
  unsigned char *s=source,*d=dest;
  do {
    *d++ = *s--;
  } while(--size);
}

void qinterpolatedown16 (int32_t *source, int size, int linum, int linum_inc) {
	if (size == 0)
	{
		*source = (linum>>16);
		return;
	}

	while (size)
	{
		*source = (linum>>16);
		source++;
		linum += linum_inc;
		size--;
	}
}

void qinterpolatedown16short (int32_t *source, int size, int linum, int linum_inc)
{
	if (size == 0) return;

	if ((intptr_t)source & 0x2)
	{
        unsigned short *src = (unsigned short *) source;
		*src = ((linum>>16)&0xffff);
        src++;
		linum += linum_inc;
        source = (int32_t *) src;

		size--;
		if (size == 0) return;
	}
	size -= 2;
	if (size < 0)
	{
		*((unsigned short *)source) = ((linum>>16)&0xffff);
		return;
	}

	while (size >= 0)
	{
		// DDOI - this fix is from the Amiga port guys
		// http://www.neoscientists.org/~dante/
#ifdef PLATFORM_BIGENDIAN
		int temp = linum & 0xffff0000;
		linum += linum_inc;
		temp |= (linum>>16);
		linum += linum_inc;
#else
		int temp = linum>>16;
		linum += linum_inc;
		temp += (linum&0xffff0000);
		linum += linum_inc;
#endif
		*source = temp;
		source++;
		size -= 2;
	}
	if (size & 1)
		*((unsigned short *)source) = ((linum>>16)&0xffff);
}


static char tablesloaded = 0;

short radarang[1280];
static unsigned short sqrtable[4096], shlookup[4096+256];
char pow2char[8] = {1,2,4,8,16,32,64,-128};
int32_t pow2long[32] =
{
	1L,2L,4L,8L,
	16L,32L,64L,128L,
	256L,512L,1024L,2048L,
	4096L,8192L,16384L,32768L,
	65536L,131072L,262144L,524288L,
	1048576L,2097152L,4194304L,8388608L,
	16777216L,33554432L,67108864L,134217728L,
	268435456L,536870912L,1073741824L,2147483647L,
};
int32_t reciptable[2048], fpuasm;



        int32_t nsqrtasm(uint32_t param)
	{
	    unsigned short *shlookup_a = (unsigned short*)shlookup;
	    unsigned short *sqrtable_a = (unsigned short*)sqrtable;
	    unsigned short cx;

	    if (param & 0xff000000)
		    cx = shlookup_a[(param>>24)+4096];
	    else
		    cx = shlookup_a[param>>12];

	    param = param >> (cx&0xff);
	    param = ((param&0xffff0000)|sqrtable_a[param]);
	    param = param >> ((cx&0xff00)>>8);

	    return param;
	}



        static int32_t krecipasm(int32_t param)
        {
		unsigned int xormask;
		int mantissa;
		int exponent;
		int32_t recip;

		union {
		float f;
		unsigned int i;
		} fi;

		fi.f = (float) param;

		fpuasm = fi.i;

		xormask = (param < 0) ? 0xffffffff : 0;

		mantissa = (fi.i & 0x007ff000) >> 12;
		exponent = (fi.i - 0x3f800000) >> 23;

		recip = (reciptable[mantissa] >> exponent) ^ xormask;

		return recip;
        } /* krecipasm */
/*
    static long krecipasm(long x)
    {
        float xf;
        int z;
        int not;
    
        if (x & 0x80000000)
            not = 0x0FFFFFFFF;
        else
            not = 0;
  
        xf = (float)x;
    
        
        x = *((int*)(&xf));
        z = x;
      
        x = x & 0x007FF000;
        x = x >> 10;
    
        z = z - 0x03F800000;
        z = z >> 23;
    
        x = shift_algebraic_right(reciptable[(x>>2)], z) ^ not;
        return x;
    } 
    */



        int32_t getclipmask(int i1, int i2, int i3, int i4)
        {
	    uint32_t eax;
	    eax = i1 >> 31;
	    eax = eax << 1; i2 *= 2;
	    if (i2&0x80000000) { eax++; }
	    eax = eax << 1; i3 *= 2;
	    if (i3&0x80000000) { eax++; }
	    eax = eax << 1; i4 *= 2;
	    if (i4&0x80000000) { eax++; }
	    return ((eax<<4)^(eax|0xf0));
        }

#if (BUILD_ENGINEREV < 19960427L)
/* Before revision 19960427 the game logic took square roots with ksqrtasm
 * from a 2048-entry table built by Newton's method. Rendering keeps the
 * table above. */
static unsigned short ksqrtable[2048];

static void initksqrt1996(void)
{
	uint32_t i, root, num;
	int32_t temp;

	for(i=0;i<2048;i++)
	{
		root = 128;
		num = i<<20;
		do
		{
			temp = root;
			root = (root+num/root)>>1;
		} while((temp-root+1) > 2);
		temp = root*root-num;
		while (klabs(temp-2*root+1) < klabs(temp))
		{
			temp += -(2*root)+1;
			root--;
		}
		while (klabs(temp+2*root+1) < klabs(temp))
		{
			temp += 2*root+1;
			root++;
		}
		ksqrtable[i] = (unsigned short)root;
	}
}

/* The 1996 ksqrtasm: below 2^11 a table lookup, above it the table at a
 * reduced precision, scaled back up. */
static int32_t ksqrtasm(int32_t num)
{
	uint32_t a = (uint32_t)num;
	int32_t b, r;

	if (num < 0) a = 0u-a;
	if (a < 2048) r = ksqrtable[a];
	else
	{
		for(b=31;!(a>>b);b--);
		b = (b-9)&~1;
		r = ksqrtable[a>>b]<<(b>>1);
	}
	return(r>>10);
}
#define LOGICSQRT(x) ksqrtasm(x)
#else
#define LOGICSQRT(x) nsqrtasm(x)
#endif

static void initksqrt(void)
{
	int32_t i, j, k;

	j = 1; k = 0;
	for(i=0;i<4096;i++)
	{
		if (i >= j) { j <<= 2; k++; }
		sqrtable[i] = (unsigned short)(msqrtasm((i<<18)+131072)<<1);
		shlookup[i] = (k<<1)+((10-k)<<8);
		if (i < 256) shlookup[i+4096] = ((k+6)<<1)+((10-(k+6))<<8);
	}
}


void engine_loadtables(void)
{
	int32_t i, fil;

	if (tablesloaded == 0)
	{
		initksqrt();
#if (BUILD_ENGINEREV < 19960427L)
		initksqrt1996();
#endif

		for(i=0;i<2048;i++) reciptable[i] = divscale30(2048L,i+2048);

		if ((fil = kopen4load("tables.dat",0)) != -1)
		{
            for (i = 0; i < 2048; i++)
				kread16(fil,&sintable[i]);

            for (i = 0; i < 640; i++)
				kread16(fil,&radarang[i]);

			for(i=0;i<640;i++) radarang[1279-i] = -radarang[i];
			kread(fil,textfont,1024);
			kread(fil,smalltextfont,1024);
			kread(fil,britable,1024);
			kclose(fil);
		}
		tablesloaded = 1;
	}
}


int getangle(int32_t xvect, int32_t yvect)
{
	if ((xvect|yvect) == 0) return(0);
	if (xvect == 0) return(512+((yvect<0)<<10));
	if (yvect == 0) return(((xvect<0)<<10));
	if (xvect == yvect) return(256+((xvect<0)<<10));
	if (xvect == -yvect) return(768+((xvect>0)<<10));
	if (klabs(xvect) > klabs(yvect))
		return(((radarang[640+scale(160,yvect,xvect)]>>6)+((xvect<0)<<10))&2047);
	return(((radarang[640-scale(160,xvect,yvect)]>>6)+512+((yvect<0)<<10))&2047);
}

int ksqrt(int32_t num)
{
	return(LOGICSQRT(num));
}

void rotatepoint(int32_t xpivot, int32_t ypivot, int32_t x, int32_t y, short daang, int32_t *x2, int32_t *y2)
{
	int32_t dacos, dasin;

	dacos = sintable[(daang+2560)&2047];
	dasin = sintable[(daang+2048)&2047];
	x -= xpivot;
	y -= ypivot;
	*x2 = dmulscale14(x,dacos,-y,dasin) + xpivot;
	*y2 = dmulscale14(y,dacos,x,dasin) + ypivot;
}

int krand(void)
{
	randomseed = (randomseed*27584621)+1;
	return(((uint32_t)randomseed)>>16);
}


int32_t engine_render_sqrt(uint32_t value)
{
	return nsqrtasm(value);
}

int32_t engine_recip(int32_t value)
{
	return krecipasm(value);
}
