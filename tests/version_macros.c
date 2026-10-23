/*
 * Checks the version macros this configuration was built with against the
 *  values CMake expects for its DUKE3D_VERSION and DUKE3D_SHAREWARE. All the
 *  checks happen at compile time; running it only reports success.
 */
#include <stdio.h>
#include "duke3d.h"

#if DUKE3D_GAMEVER != EXPECT_GAMEVER
#error "DUKE3D_GAMEVER"
#endif
#if BUILD_ENGINEREV != EXPECT_ENGINEREV
#error "BUILD_ENGINEREV"
#endif
#if BYTEVERSION != EXPECT_BYTEVERSION
#error "BYTEVERSION"
#endif

#if EXPECT_PLUTOPAK
#ifndef PLUTOPAK
#error "PLUTOPAK should be defined"
#endif
#else
#ifdef PLUTOPAK
#error "PLUTOPAK should not be defined"
#endif
#endif

#if EXPECT_VOLUMEONE
#ifndef VOLUMEONE
#error "VOLUMEONE should be defined"
#endif
#else
#ifdef VOLUMEONE
#error "VOLUMEONE should not be defined"
#endif
#endif

int main(void)
{
    printf("gamever %ld enginerev %ld byteversion %d\n",
        (long)DUKE3D_GAMEVER, (long)BUILD_ENGINEREV, (int)BYTEVERSION);
    return 0;
}
