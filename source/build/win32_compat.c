/*
 * Win32 compatibility code: the pieces win32_compat.h declares but that
 *  only unix_compat.c defined upstream. Not part of icculus's release;
 *  see VENDORED.md.
 */

/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */

#ifndef PLATFORM_WIN32
#error Please define PLATFORM_WIN32 to use this code.
#endif

#include <stdio.h>
#include "win32_compat.h"

/*
  256 byte table to assist in performing an
  algebraic shift right. These values represent
  all possible contributions of the sign bit
  when shifted by different values.
 */
const int hbits[64] =
{
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x00000000,0x00000000,0x00000000,0x00000000,
    0x80000000,0xC0000000,0xE0000000,0xF0000000,
    0xF8000000,0xFC000000,0xFE000000,0xFF000000,
    0xFF800000,0xFFC00000,0xFFE00000,0xFFF00000,
    0xFFF80000,0xFFFC0000,0xFFFE0000,0xFFFF0000,
    0xFFFF8000,0xFFFFC000,0xFFFFE000,0xFFFFF000,
    0xFFFFF800,0xFFFFFC00,0xFFFFFE00,0xFFFFFF00,
    0xFFFFFF80,0xFFFFFFC0,0xFFFFFFE0,0xFFFFFFF0,
    0xFFFFFFF8,0xFFFFFFFC,0xFFFFFFFE,0xFFFFFFFF
};



/* end of win32_compat.c ... */
