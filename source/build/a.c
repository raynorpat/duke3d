// A.ASM replacement using C
// Mainly by Ken Silverman, with things melded with his port by
// Jonathon Fowler (jf@jonof.id.au)
//
// "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
// Ken Silverman's official web site: "http://www.advsys.net/ken"
// See the included license file "BUILDLIC.TXT" for license info.
//
// From JFBuild's src/a-c.c, commit e06cbd005a8e6d5f38b5a7aa27ea597dd233c16e
// (https://github.com/jonof/jfbuild), which is Ken's A.C from BUILDC.ZIP made
// 64-bit clean. Local changes (see VENDORED.md):
//  - parameter types follow our a.h (intptr_t for addresses), and shift
//    counts are masked to 0..31 as x86 does;
//  - each family of routines keeps its own setup state, as A.ASM's
//    self-modifying code did: our engine sets some up once and draws other
//    kinds in between (JFBuild's engine re-runs each setup before use);
//  - mhline and thline draw the last pixel of a span, as A.ASM does;
//  - vlineasm1, mvlineasm1 and tvlineasm1 return the final texture
//    position, which our engine's four-column paths read back;
//  - setupslopevlin and slopevlin are the icculus port's, and prevlineasm1
//    keeps A.ASM's one-pixel shortcut, so the picture matches A.ASM's exactly;
//  - the routines JFBuild's a-c.c doesn't have, because its engine stopped
//    calling them, are at the end: the four-column vline routines built on
//    the one-column ones, tvlineasm2, and the parallax-sky rhline family.

#include <stdint.h>
#include "platform.h"
#include "build.h"
#include "a.h"

extern int32_t asm1, asm4, fpuasm, globalx3, globaly3;
extern intptr_t asm2, asm3;
extern int32_t vplce[4], vince[4];
extern intptr_t palookupoffse[4], bufplce[4];

	//The texel at (bx,by) for a texture 2^logx by 2^logy. A texture 1 texel
	//tall (logy 0) takes nothing from by, as A.ASM's shld by 0 does.
#define TEXEL(buf,bx,by,logx,logy) \
	(buf)[((((uint32_t)(bx))>>((32-(logx))&31))<<(logy))+((logy) ? (((uint32_t)(by))>>(32-(logy))) : 0)]

static int32_t bpl, transmode = 0;
static unsigned char *gtrans;

	//Global variable functions
void setvlinebpl(int32_t dabpl) { bpl = dabpl; }
void fixtransluscence(intptr_t datransoff) { gtrans = (unsigned char *)datransoff; }
void settransnormal(void) { transmode = 0; }
void settransreverse(void) { transmode = 1; }


	//Ceiling/floor horizontal line functions
static int32_t hlogx, hlogy, hbxinc, hbyinc;
static unsigned char *hbuf, *hpal;
void sethlinesizes(int32_t logx, int32_t logy, intptr_t bufplc)
	{ hlogx = logx; hlogy = logy; hbuf = (unsigned char *)bufplc; }
void setpalookupaddress(char *paladdr) { hpal = (unsigned char *)paladdr; }
void setuphlineasm4(int32_t bxinc, int32_t byinc) { hbxinc = bxinc; hbyinc = byinc; }
void hlineasm4(int32_t cnt, int32_t skiploadincs, int32_t paloffs, uint32_t by, uint32_t bx, intptr_t p)
{
	unsigned char *palptr, *pp;

	palptr = &hpal[paloffs];
	pp = (unsigned char *)p;
	if (!skiploadincs) { hbxinc = asm1; hbyinc = (int32_t)asm2; }
	for(;cnt>=0;cnt--)
	{
		*pp = palptr[TEXEL(hbuf,bx,by,hlogx,hlogy)];
		bx -= hbxinc;
		by -= hbyinc;
		pp--;
	}
}


	//Sloped ceiling/floor vertical line functions: the icculus port's, which follow
	//A.ASM's float reciprocal exactly (JFBuild's krecip version rounds differently).
/* #pragma aux setupslopevlin parm [eax][ebx][ecx] modify [edx] */
static intptr_t slopemach_ebx;
static int32_t slopemach_ecx;
static int32_t slopemach_edx;
static unsigned char slopemach_ah1;
static unsigned char slopemach_ah2;
static float asm2_f;
typedef union { unsigned int i; float f; } bitwisef2i;
void setupslopevlin(int32_t i1, intptr_t i2, int32_t i3)
{
    bitwisef2i c;
    slopemach_ebx = i2;
    slopemach_ecx = i3;
    slopemach_edx = (1<<(i1&0x1f)) - 1;
    slopemach_edx <<= ((i1&0x1f00)>>8);
    slopemach_ah1 = 32-((i1&0x1f00)>>8);
    slopemach_ah2 = (slopemach_ah1 - (i1&0x1f)) & 0x1f;
    c.f = asm2_f = (float)asm1;
    asm2 = c.i;
} /* setupslopevlin */

extern int32_t reciptable[2048];
extern int32_t globalx3, globaly3;
extern int32_t fpuasm;
#define low32(a) ((a&0xffffffff))

/* #pragma aux slopevlin parm [eax][ebx][ecx][edx][esi][edi] */
void slopevlin(intptr_t i1, int32_t _i2, intptr_t *i3, int32_t i4, int32_t i5, int32_t i6)
{
    uint32_t i2 = (uint32_t)_i2;
    bitwisef2i c;
    uint32_t ecx,eax,ebx,edx,esi,edi;
    intptr_t pal;
    float a = (float) asm3 + asm2_f;
    i1 -= slopemach_ecx;
    esi = i5 + (uint32_t)low32((__int64)globalx3 * (__int64)(i2<<3));
    edi = i6 + (uint32_t)low32((__int64)globaly3 * (__int64)(i2<<3));
    ebx = i4;
    do {
	    // -------------
	    // All this is calculating a fixed point approx. of 1/a
	    c.f = a;
	    fpuasm = eax = c.i;
	    edx = (((int32_t)eax) < 0) ? 0xffffffff : 0;
	    eax = eax << 1;
	    ecx = (eax>>24);	//  exponent
	    eax = ((eax&0xffe000)>>11);
	    ecx = ((ecx&0xffffff00)|((ecx-2)&0xff));
	    eax = reciptable[eax/4];
	    eax >>= (ecx&0x1f);
	    eax ^= edx;
	    // -------------
	    edx = i2;
	    i2 = eax;
	    eax -= edx;
	    ecx = (uint32_t)low32((__int64)globalx3 * (__int64)eax);
	    eax = (uint32_t)low32((__int64)globaly3 * (__int64)eax);
	    a += asm2_f;

	    asm4 = ebx;
	    ecx = ((ecx&0xffffff00)|(ebx&0xff));
	    if (ebx >= 8) ecx = ((ecx&0xffffff00)|8);

	    ebx = esi;
	    edx = edi;
	    while ((ecx&0xff))
	    {
		    ebx >>= slopemach_ah2;
		    esi += ecx;
		    edx >>= slopemach_ah1;
		    ebx &= slopemach_edx;
		    edi += eax;
		    i1 += slopemach_ecx;
		    edx = ((edx&0xffffff00)|((((unsigned char *)slopemach_ebx)[ebx+edx])));
		    pal = *i3; // register trickery
		    i3--;
		    eax = ((eax&0xffffff00)|(((unsigned char *)pal)[edx]));
		    ebx = esi;
		    *((unsigned char *)i1) = (eax&0xff);
		    edx = edi;
		    ecx = ((ecx&0xffffff00)|((ecx-1)&0xff));
	    }
	    ebx = asm4;
	    ebx -= 8;	// BITSOFPRECISIONPOW
    } while ((int32_t)ebx > 0);
} /* slopevlin */


	//Wall,face sprite/wall sprite vertical line functions
static int32_t vlogy, mvlogy, tvlogy;
void setupvlineasm(int32_t neglogy) { vlogy = neglogy&31; }
int32_t vlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, int32_t vplc, intptr_t bufplc, intptr_t p)
{
	unsigned char *pp = (unsigned char *)p, *pbuf = (unsigned char *)bufplc, *ppal = (unsigned char *)paloffs;
	uint32_t uvplc = (uint32_t)vplc;

	for(;cnt>=0;cnt--)
	{
		*pp = ppal[pbuf[uvplc>>vlogy]];
		pp += bpl;
		uvplc += vinc;
	}
	return (int32_t)uvplc;
}

void setupmvlineasm(int32_t neglogy) { mvlogy = neglogy&31; }
int32_t mvlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, int32_t vplc, intptr_t bufplc, intptr_t p)
{
	unsigned char ch, *pp = (unsigned char *)p, *pbuf = (unsigned char *)bufplc, *ppal = (unsigned char *)paloffs;
	uint32_t uvplc = (uint32_t)vplc;

	for(;cnt>=0;cnt--)
	{
		ch = pbuf[uvplc>>mvlogy]; if (ch != 255) *pp = ppal[ch];
		pp += bpl;
		uvplc += vinc;
	}
	return (int32_t)uvplc;
}

void setuptvlineasm(int32_t neglogy) { tvlogy = neglogy&31; }
int32_t tvlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, int32_t vplc, intptr_t bufplc, intptr_t p)
{
	unsigned char ch, *pp = (unsigned char *)p, *pbuf = (unsigned char *)bufplc, *ppal = (unsigned char *)paloffs;
	uint32_t uvplc = (uint32_t)vplc;

	if (transmode)
	{
		for(;cnt>=0;cnt--)
		{
			ch = pbuf[uvplc>>tvlogy];
			if (ch != 255) *pp = gtrans[(*pp)+(ppal[ch]<<8)];
			pp += bpl;
			uvplc += vinc;
		}
	}
	else
	{
		for(;cnt>=0;cnt--)
		{
			ch = pbuf[uvplc>>tvlogy];
			if (ch != 255) *pp = gtrans[((*pp)<<8)+ppal[ch]];
			pp += bpl;
			uvplc += vinc;
		}
	}
	return (int32_t)uvplc;
}

	//Floor sprite horizontal line functions
	//Fix: A.ASM draws (cntup16>>16)+1 pixels ("sub ecx,65536 / jnc"); JFBuild's ">0" dropped the last one of every span.
static int32_t mlogx, mlogy, tlogx, tlogy;
void msethlineshift(int32_t logx, int32_t logy) { mlogx = logx; mlogy = logy; }
void mhline(intptr_t bufplc, uint32_t bx, int32_t cntup16, int32_t junk, uint32_t by, intptr_t p)
{
	unsigned char ch, *pp = (unsigned char *)p, *pbuf = (unsigned char *)bufplc, *ppal = (unsigned char *)asm3;

	for(cntup16>>=16;cntup16>=0;cntup16--)
	{
		ch = TEXEL(pbuf,bx,by,mlogx,mlogy);
		if (ch != 255) *pp = ppal[ch];
		bx += asm1;
		by += (int32_t)asm2;
		pp++;
	}
	(void)junk;
}

void tsethlineshift(int32_t logx, int32_t logy) { tlogx = logx; tlogy = logy; }
void thline(intptr_t bufplc, uint32_t bx, int32_t cntup16, int32_t junk, uint32_t by, intptr_t p)
{
	unsigned char ch, *pp = (unsigned char *)p, *pbuf = (unsigned char *)bufplc, *ppal = (unsigned char *)asm3;

	if (transmode)
	{
		for(cntup16>>=16;cntup16>=0;cntup16--)
		{
			ch = TEXEL(pbuf,bx,by,tlogx,tlogy);
			if (ch != 255) *pp = gtrans[(*pp)+(ppal[ch]<<8)];
			bx += asm1;
			by += (int32_t)asm2;
			pp++;
		}
	}
	else
	{
		for(cntup16>>=16;cntup16>=0;cntup16--)
		{
			ch = TEXEL(pbuf,bx,by,tlogx,tlogy);
			if (ch != 255) *pp = gtrans[((*pp)<<8)+ppal[ch]];
			bx += asm1;
			by += (int32_t)asm2;
			pp++;
		}
	}
	(void)junk;
}


	//Rotatesprite vertical line functions
static unsigned char *spal, *mspal, *tspal;
static int32_t sbxinc, sbyinc, sysiz, msbxinc, msbyinc, msysiz, tsbxinc, tsbyinc, tsysiz;
void setupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz)
{
	spal = (unsigned char *)paloffs;
	sbxinc = bxinc;
	sbyinc = byinc;
	sysiz = ysiz;
}
void spritevline(int32_t bx, int32_t by, int32_t cnt, intptr_t bufplc, intptr_t p)
{
	unsigned char *pp = (unsigned char *)p, *pbuf = (unsigned char *)bufplc;

	for(;cnt>1;cnt--)
	{
		*pp = spal[pbuf[(bx>>16)*sysiz+(by>>16)]];
		bx += sbxinc;
		by += sbyinc;
		pp += bpl;
	}
}

void msetupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz)
{
	mspal = (unsigned char *)paloffs;
	msbxinc = bxinc;
	msbyinc = byinc;
	msysiz = ysiz;
}
void mspritevline(int32_t bx, int32_t by, int32_t cnt, intptr_t bufplc, intptr_t p)
{
	unsigned char ch, *pp = (unsigned char *)p, *pbuf = (unsigned char *)bufplc;

	for(;cnt>1;cnt--)
	{
		ch = pbuf[(bx>>16)*msysiz+(by>>16)];
		if (ch != 255) *pp = mspal[ch];
		bx += msbxinc;
		by += msbyinc;
		pp += bpl;
	}
}

void tsetupspritevline(intptr_t paloffs, int32_t bxinc, int32_t byinc, int32_t ysiz)
{
	tspal = (unsigned char *)paloffs;
	tsbxinc = bxinc;
	tsbyinc = byinc;
	tsysiz = ysiz;
}
void tspritevline(int32_t bx, int32_t by, int32_t cnt, intptr_t bufplc, intptr_t p)
{
	unsigned char ch, *pp = (unsigned char *)p, *pbuf = (unsigned char *)bufplc;

	if (transmode)
	{
		for(;cnt>1;cnt--)
		{
			ch = pbuf[(bx>>16)*tsysiz+(by>>16)];
			if (ch != 255) *pp = gtrans[(*pp)+(tspal[ch]<<8)];
			bx += tsbxinc;
			by += tsbyinc;
			pp += bpl;
		}
	}
	else
	{
		for(;cnt>1;cnt--)
		{
			ch = pbuf[(bx>>16)*tsysiz+(by>>16)];
			if (ch != 255) *pp = gtrans[((*pp)<<8)+tspal[ch]];
			bx += tsbxinc;
			by += tsbyinc;
			pp += bpl;
		}
	}
}

static int32_t slabbpl;
static unsigned char *slabpal;
void setupdrawslab (int32_t dabpl, intptr_t pal)
	{ slabbpl = dabpl; slabpal = (unsigned char *)pal; }

void drawslab (int32_t dx, int32_t v, int32_t dy, int32_t vi, intptr_t vptr, intptr_t p)
{
	int32_t x;
	unsigned char *pp = (unsigned char *)p;

	while (dy > 0)
	{
		for(x=0;x<dx;x++) *(pp+x) = slabpal[(int32_t)(*(unsigned char *)((v>>16)+vptr))];
		pp += slabbpl; v += vi; dy--;
	}
}

void stretchhline (intptr_t p0, int32_t u, int32_t cnt, int32_t uinc, intptr_t rptr, intptr_t p)
{
	unsigned char *np;

	np = (unsigned char *)(p-(cnt<<2));
	do
	{
		p--;
		*(unsigned char *)p = *(unsigned char *)((u>>16)+rptr); u -= uinc;
	} while ((unsigned char *)p > np);
	(void)p0;
}


void mmxoverlay(void) { }


	//Not in JFBuild's a-c.c: our engine still draws these four columns at a time.
	//prevlineasm1 is vlineasm1, except that A.ASM draws a one-pixel span with a
	//shortcut whose palette index keeps the high bits of the shifted texture
	//position: for textures over 256 texels tall that's a darker shade row. Kept,
	//as the icculus port did, so these pixels look as they do in DOS.
int32_t prevlineasm1(int32_t vinc, intptr_t paloffs, int32_t cnt, int32_t vplc, intptr_t bufplc, intptr_t p)
{
	uint32_t i;

	if (cnt != 0) return vlineasm1(vinc,paloffs,cnt,vplc,bufplc,p);
	i = ((uint32_t)vplc)>>vlogy;
	i = (i&0xffffff00) | ((unsigned char *)bufplc)[i];
	*(unsigned char *)p = ((unsigned char *)paloffs)[i];
	return vplc+vinc;
}

	//cnt rows of four adjacent columns, from vplce[], vince[], palookupoffse[] and bufplce[].
void vlineasm4(int32_t cnt, intptr_t p)
{
	int32_t i;

	for(i=0;i<4;i++)
		vplce[i] = vlineasm1(vince[i],palookupoffse[i],cnt-1,vplce[i],bufplce[i],p+i);
}

void mvlineasm4(int32_t cnt, intptr_t p)
{
	int32_t i;

	for(i=0;i<4;i++)
		vplce[i] = mvlineasm1(vince[i],palookupoffse[i],cnt-1,vplce[i],bufplce[i],p+i);
}

static unsigned char tran2shr;
static uintptr_t tran2pal_ebx;
static uintptr_t tran2pal_ecx;
void setuptvlineasm2(int32_t i1, intptr_t i2, intptr_t i3)
{
	tran2shr = (i1&0x1f);
	tran2pal_ebx = i2;
	tran2pal_ecx = i3;
} /* */

/* #pragma aux tvlineasm2 parm [eax][ebx][ecx][edx][esi][edi] */
void tvlineasm2(uint32_t i1, uint32_t i2, uintptr_t i3, uintptr_t i4, uint32_t i5, uintptr_t i6)
{
	uint32_t ebp = i1;
	uint32_t tran2inca = i2;
	uint32_t tran2incb = asm1;
	uintptr_t tran2bufa = i3;
	uintptr_t tran2bufb = i4;
	unsigned char *tran2edi = (unsigned char *)asm2;
	unsigned char *tran2edi1 = (unsigned char *)asm2 + 1;
	/*
	 * The asm walked a negative 32-bit offset up to zero from the column
	 *  end in asm2. Keep that offset 32-bit, and index from asm2 directly.
	 */
	uint32_t i6off = (uint32_t)(i6 - (uintptr_t)asm2);

	do {
		i1 = i5 >> tran2shr;
		i2 = ebp >> tran2shr;
		i5 += tran2inca;
		ebp += tran2incb;
		i3 = ((unsigned char *)tran2bufa)[i1];
		i4 = ((unsigned char *)tran2bufb)[i2];
		if (i3 == 255) { // skipdraw1
			if (i4 != 255) { // skipdraw3
				unsigned short val;
				val = ((unsigned char *)tran2pal_ecx)[i4];
				val |= (tran2edi1[(int32_t)i6off]<<8);
				if (transmode) val = ((val>>8)|(val<<8));
				tran2edi1[(int32_t)i6off] =
					gtrans[val];
			}
		} else if (i4 == 255) { // skipdraw2
			unsigned short val;
			val = ((unsigned char *)tran2pal_ebx)[i3];
			val |= (tran2edi[(int32_t)i6off]<<8);
			if (transmode) val = ((val>>8)|(val<<8));
			tran2edi[(int32_t)i6off] =
				gtrans[val];
		} else {
			unsigned short l = tran2edi[(int32_t)i6off]<<8;
			unsigned short r = tran2edi1[(int32_t)i6off]<<8;
			l |= ((unsigned char *)tran2pal_ebx)[i3];
			r |= ((unsigned char *)tran2pal_ecx)[i4];
			if (transmode) {
				l = ((l>>8)|(l<<8));
				r = ((r>>8)|(r<<8));
			}
			tran2edi[(int32_t)i6off] =
				gtrans[l];
			tran2edi1[(int32_t)i6off] =
				gtrans[r];
		}
		i6off += bpl;
	} while (i6off > i6off - bpl);
	asm1 = i5;
	asm2 = ebp;
} /* tvlineasm2 */


	//The parallax sky's line routines, kept from the icculus port.
static int32_t rmach_eax;
static int32_t rmach_ebx;
static int32_t rmach_ecx;
static intptr_t rmach_edx;
static int32_t rmach_esi;
void setuprhlineasm4(int32_t i1, int32_t i2, int32_t i3, intptr_t i4, int32_t i5, int32_t i6)
{
    rmach_eax = i1;
    rmach_ebx = i2;
    rmach_ecx = i3;
    rmach_edx = i4;
    rmach_esi = i5;
} /* setuprhlineasm4 */

void rhlineasm4(int32_t i1, intptr_t i2, int32_t i3, uint32_t i4, uint32_t i5, intptr_t i6)
{
    uint32_t ebp;
    intptr_t rmach6b = i6 - i1 - 1;

    if (i1 <= 0) return;

    i6 = i1;
    do {
	    i3 = ((i3&0xffffff00)|(*((unsigned char *)i2)));
	    i4 -= rmach_eax;
	    ebp = (((i4+rmach_eax) < i4) ? -1 : 0);
	    i5 -= rmach_ebx;
	    if ((i5 + rmach_ebx) < i5) i2 -= (rmach_ecx+1);
	    else i2 -= rmach_ecx;
	    ebp &= rmach_esi;
	    i1 = ((i1&0xffffff00)|(((unsigned char *)rmach_edx)[i3]));
	    ((unsigned char *)rmach6b)[i6] = (i1&0xff);
	    i2 -= ebp;
	    i6--;
    } while (i6);
} /* rhlineasm4 */

static int32_t rmmach_eax;
static int32_t rmmach_ebx;
static int32_t rmmach_ecx;
static intptr_t rmmach_edx;
static int32_t rmmach_esi;
void setuprmhlineasm4(int32_t i1, int32_t i2, int32_t i3, intptr_t i4, int32_t i5, int32_t i6)
{
    rmmach_eax = i1;
    rmmach_ebx = i2;
    rmmach_ecx = i3;
    rmmach_edx = i4;
    rmmach_esi = i5;
} /* setuprmhlineasm4 */

/* #pragma aux rmhlineasm4 parm [eax][ebx][ecx][edx][esi][edi] */
void rmhlineasm4(int32_t i1, intptr_t i2, int32_t i3, int32_t i4, int32_t i5, intptr_t i6)
{
    uint32_t ebp;
    intptr_t rmach6b = i6 - i1 - 1;

    if (i1 <= 0) return;

    i6 = i1;
    do {
	    i3 = ((i3&0xffffff00)|(*((unsigned char *)i2)));
	    i4 -= rmmach_eax;
	    ebp = (((i4+rmmach_eax) < i4) ? -1 : 0);
	    i5 -= rmmach_ebx;
	    if ((i5 + rmmach_ebx) < i5) i2 -= (rmmach_ecx+1);
	    else i2 -= rmmach_ecx;
	    ebp &= rmmach_esi;
	    if ((i3&0xff) != 255) {
		    i1 = ((i1&0xffffff00)|(((unsigned char *)rmmach_edx)[i3]));
		    ((unsigned char *)rmach6b)[i6] = (i1&0xff);
	    }
	    i2 -= ebp;
	    i6--;
    } while (i6);
} /* rmhlineasm4 */


/* #pragma aux setupqrhlineasm4 parm [eax][ebx][ecx][edx][esi][edi] */
void setupqrhlineasm4(int32_t i1, int32_t i2, int32_t i3, intptr_t i4, int32_t i5, int32_t i6)
{
    setuprhlineasm4(i1,i2,i3,i4,i5,i6);
} /* setupqrhlineasm4 */


/* #pragma aux qrhlineasm4 parm [eax][ebx][ecx][edx][esi][edi] */
void qrhlineasm4(int32_t i1, intptr_t i2, int32_t i3, int32_t i4, int32_t i5, intptr_t i6)
{
    rhlineasm4(i1,i2,i3,i4,i5,i6);
} /* qrhlineasm4 */
