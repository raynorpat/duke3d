/*
 * Cache1d declarations.
 *
 *  Written by Ryan C. Gordon. (icculus@clutteredmind.org)
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

#ifndef _INCLUDE_CACHE1D_H_
#define _INCLUDE_CACHE1D_H_
#include <stdint.h>

void initcache(intptr_t dacachestart, int32_t dacachesize);
void allocache (intptr_t *newhandle, int32_t newbytes, unsigned char *newlockptr);
void suckcache (int32_t *suckptr);
void agecache(void);
void reportandexit(char *errormessage);
int32_t initgroupfile(const char *filename);
void uninitgroupfile(void);
int32_t kopen4load(const char *filename, char searchfirst);
int32_t kread(int32_t handle, void *buffer, int32_t leng);
int kread8(int32_t handle, char *buffer);
int kread16(int32_t handle, short *buffer);
int kread32(int32_t handle, int32_t *buffer);
int32_t klseek(int32_t handle, int32_t offset, int32_t whence);
int32_t kfilelength(int32_t handle);
void kclose(int32_t handle);
void kdfread(void *buffer, size_t dasizeof, size_t count, int32_t fil);
void kdfread8(char *buffer, size_t count, int32_t fil);
void kdfread16(short *buffer, size_t count, int32_t fil);
void kdfread32(int32_t *buffer, size_t count, int32_t fil);
void dfread(void *buffer, size_t dasizeof, size_t count, FILE *fil);
void dfread8(char *buffer, size_t count, FILE *fil);
void dfread16(short *buffer, size_t count, FILE *fil);
void dfread32(int32_t *buffer, size_t count, FILE *fil);
void dfwrite(void *buffer, size_t dasizeof, size_t count, FILE *fil);
void dfwrite8(char *buffer, size_t count, FILE *fil);
void dfwrite16(short *buffer, size_t count, FILE *fil);
void dfwrite32(int32_t *buffer, size_t count, FILE *fil);
int32_t compress(char *lzwinbuf, int32_t uncompleng, char *lzwoutbuf);
int32_t uncompress(char *lzwinbuf, int32_t compleng, char *lzwoutbuf);

#endif  /* !defined _INCLUDE_CACHE1D_H_ */

/* end of cache1d.h ... */



