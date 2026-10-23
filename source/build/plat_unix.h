/*
 * Unix compatibility header. Takes care of some legacy code issues.
 *
 *  Written by Ryan C. Gordon (icculus@clutteredmind.org)
 *
 * Please do NOT harrass Ken Silverman about any code modifications
 *  (including this file) to BUILD.
 */

/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */

#ifndef _INCLUDE_PLAT_UNIX_H_
#define _INCLUDE_PLAT_UNIX_H_

#if (!defined PLATFORM_UNIX)
#error PLATFORM_UNIX is not defined.
#endif

#define __int64 long long

#define PLATFORM_SUPPORTS_SDL

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <assert.h>

extern const int hbits[];

/*
  Do some bitwise magic to approximate an algebraic (sign preserving)
  right shift.
 */
#define shift_algebraic_right(value,distance) \
(((value) >> (distance))| \
 (hbits[(distance) + (((value) & 0x80000000) >> 26)]))


#ifndef O_BINARY
#define O_BINARY 0
#endif

/* damned -ansi flag... :) */
int stricmp(const char *x, const char *y);

#if (defined __STRICT_ANSI__)
#define inline __inline__
#endif

#define printext16 printext256
#define printext16_noupdate printext256_noupdate

/* Other DOSisms. See plat_unix.c for implementation. */
int32_t filelength(int fhandle);

/* !!! need an implementation of findfirst()/findnext()! */
/*     Look for references to _dos_findfirst() in build.c! */

#if (!defined S_IREAD)
#define S_IREAD S_IRUSR
#endif

#ifdef DC //bero
#undef stdout
#undef stderr
#define        stdout ((FILE*)2)
#define        stderr ((FILE*)2)
#endif

#ifndef max
#define max(x, y)  (((x) > (y)) ? (x) : (y))
#endif

#ifndef min
#define min(x, y)  (((x) < (y)) ? (x) : (y))
#endif

#endif

/* end of plat_unix.h ... */


