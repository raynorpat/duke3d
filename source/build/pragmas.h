/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */

#ifndef __PRAGMAS_H__
#define __PRAGMAS_H__


#include <stdint.h>
#include "platform.h" //for __int64

static __inline void swapchar(unsigned char *p1, unsigned char *p2)
{ unsigned char tmp = *p1; *p1 = *p2; *p2 = tmp; }
static __inline void swapshort(short *p1, short *p2)
{ short tmp = *p1; *p1 = *p2; *p2 = tmp; }
static __inline void swaplong(int32_t *p1, int32_t *p2)
{ int32_t tmp = *p1; *p1 = *p2; *p2 = tmp; }
static __inline void swapchar2(unsigned char *p1, unsigned char *p2, int xsiz)
{
    swapchar(p1, p2);
    swapchar(p1 + 1, p2 + xsiz);
}


uint32_t getkensmessagecrc(intptr_t param);
int32_t msqrtasm(int i1);

void vlin16first (int32_t i1, int32_t i2);

static __inline int sqr (int input1) { return input1*input1; }

/* internal use:32x32 = 64bit */
static __inline __int64 mul32_64(int i1,int i2)
{
	return (__int64)i1*i2;
}
/*
================
scale

Compute a*b/c with a full 64-bit product.
Division truncates toward zero, including for negative results.
================
*/
static __inline int scale (int input1, int input2, int input3)
{
	return (int)(mul32_64(input1,input2)/input3);
}
/*
================
mulscale

Multiply in 64 bits, then take the shifted 32-bit result.
The arithmetic shift rounds negative products down, unlike division.
================
*/
static __inline int mulscale (int input1, int input2, int input3)
{
	return (int)(mul32_64(input1,input2)>>input3);
}
/*
================
dmulscale

Add two full products before shifting the fixed-point result.
Shifting each term first would lose the carry from their fractional bits.
================
*/
static __inline int dmulscale  (int input1, int input2, int input3,int input4,int input5)
{
	return (int)((mul32_64(input1,input2) + mul32_64(input3,input4))>>input5);
}
/*
================
tmulscale

Add three full products before shifting, for fixed-point dot products.
Keep the low bits until all terms have been accumulated.
================
*/
static __inline int tmulscale(int i1, int i2, int i3, int i4, int i5, int i6,int shift)
{
	return (int)((mul32_64(i1,i2) + mul32_64(i3,i4) + mul32_64(i5,i6))>>shift);
}
/*
================
divscale

Shift the numerator in 64 bits before dividing.
The caller must supply a nonzero divisor and a representable quotient.
================
*/
static __inline int divscale(int i1, int i2, int i3)
{
	return (int)(((__int64)i1<<i3)/i2);
}

#define DEFFUNCS \
DEFFUN(1)\
DEFFUN(2)\
DEFFUN(3)\
DEFFUN(4)\
DEFFUN(5)\
DEFFUN(6)\
DEFFUN(7)\
DEFFUN(8)\
DEFFUN(9)\
DEFFUN(10)\
DEFFUN(11)\
DEFFUN(12)\
DEFFUN(13)\
DEFFUN(14)\
DEFFUN(15)\
DEFFUN(16)\
DEFFUN(17)\
DEFFUN(18)\
DEFFUN(19)\
DEFFUN(20)\
DEFFUN(21)\
DEFFUN(22)\
DEFFUN(23)\
DEFFUN(24)\
DEFFUN(25)\
DEFFUN(26)\
DEFFUN(27)\
DEFFUN(28)\
DEFFUN(29)\
DEFFUN(30)\
DEFFUN(31)\
DEFFUN(32)

#define DEFFUN(N) \
static __inline int mulscale##N(int input1, int input2) \
{ return mulscale(input1,input2,N); }
DEFFUNCS
#undef DEFFUN

#define DEFFUN(N) \
static __inline int dmulscale##N(int input1, int input2,int input3,int input4) \
{ return dmulscale(input1,input2,input3,input4,N); }
DEFFUNCS
#undef DEFFUN

#define DEFFUN(N) \
static __inline int tmulscale##N(int i1, int i2,int i3,int i4,int i5,int i6) \
{ return tmulscale(i1,i2,i3,i4,i5,i6,N); }
DEFFUNCS
#undef DEFFUN

#define DEFFUN(N) \
static __inline int divscale##N(int input1, int input2) \
{ return divscale(input1,input2,N); }
DEFFUNCS
#undef DEFFUN

static __inline int ksgn(int i1)
{
  if (i1 < 0) return -1;
  else if (i1 > 0) return 1;
  else return 0;
}

static __inline int sgn(int i1) { return ksgn(i1); }
static __inline int klabs (int i1)
{
  /* Fix: negate as unsigned, since -INT32_MIN is undefined in C; x86 neg leaves it as is. */
  if (i1 < 0) i1 = (int)(0u - (unsigned)i1);
  return i1;
}
static __inline int mul3 (int i1) { return i1*3; }
static __inline int mul5 (int i1) { return i1*5; }
static __inline int mul9 (int i1) { return i1*9; }

void copybufreverse(void *source, void *dest, int size);
void copybuf(void *source, void *dest, int size);
void clearbuf(void *buffer, int size, int32_t fill_value);
void clearbufbyte(void *buffer, int size, int32_t fill_value);
void copybufbyte(void *source, void *dest, int size);

void qinterpolatedown16 (int32_t *source, int size, int linum, int linum_inc);
void qinterpolatedown16short (int32_t *source, int size, int linum, int linum_inc);


#endif /* !defined _INCLUDE_PRAGMAS_H_ */

/* end of pragmas.h ... */


