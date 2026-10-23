/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */

#ifndef _INCLUDE_A_H_
#define _INCLUDE_A_H_
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

void mmxoverlay(void);
void sethlinesizes(int32_t,int32_t,intptr_t);
void setpalookupaddress(char *);
void setuphlineasm4(int32_t,int32_t);
void hlineasm4(int32_t,int32_t,int32_t,uint32_t,uint32_t,intptr_t);
void setuprhlineasm4(int32_t,int32_t,int32_t,intptr_t,int32_t,int32_t);
void rhlineasm4(int32_t,intptr_t,int32_t,uint32_t,uint32_t,intptr_t);
void setuprmhlineasm4(int32_t,int32_t,int32_t,intptr_t,int32_t,int32_t);
void rmhlineasm4(int32_t,intptr_t,int32_t,int32_t,int32_t,intptr_t);
void setupqrhlineasm4(int32_t,int32_t,int32_t,intptr_t,int32_t,int32_t);
void qrhlineasm4(int32_t,intptr_t,int32_t,int32_t,int32_t,intptr_t);
void setvlinebpl(int32_t);
void fixtransluscence(intptr_t);
int32_t prevlineasm1(int32_t,intptr_t,int32_t,int32_t,intptr_t,intptr_t);
int32_t vlineasm1(int32_t,intptr_t,int32_t,int32_t,intptr_t,intptr_t);
void setuptvlineasm(int32_t);
int32_t tvlineasm1(int32_t,intptr_t,int32_t,int32_t,intptr_t,intptr_t);
void setuptvlineasm2(int32_t,intptr_t,intptr_t);
void tvlineasm2(uint32_t,uint32_t,uintptr_t,uintptr_t,uint32_t,uintptr_t);
int32_t mvlineasm1(int32_t,intptr_t,int32_t,int32_t,intptr_t,intptr_t);
void setupvlineasm(int32_t);
void vlineasm4(int32_t,intptr_t);
void setupmvlineasm(int32_t);
void mvlineasm4(int32_t,intptr_t);
void setupspritevline(intptr_t,int32_t,int32_t,int32_t);
void spritevline(int32_t,int32_t,int32_t,intptr_t,intptr_t);
void msetupspritevline(intptr_t,int32_t,int32_t,int32_t);
void mspritevline(int32_t,int32_t,int32_t,intptr_t,intptr_t);
void tsetupspritevline(intptr_t,int32_t,int32_t,int32_t);
void tspritevline(int32_t,int32_t,int32_t,intptr_t,intptr_t);
void mhline(intptr_t,uint32_t,int32_t,int32_t,uint32_t,intptr_t);
void msethlineshift(int32_t,int32_t);
void thline(intptr_t,uint32_t,int32_t,int32_t,uint32_t,intptr_t);
void tsethlineshift(int32_t,int32_t);
void setupslopevlin(int32_t,intptr_t,int32_t);
void slopevlin(intptr_t,int32_t,intptr_t *,int32_t,int32_t,int32_t);
void settransnormal(void);
void settransreverse(void);
void setupdrawslab(int32_t,intptr_t);
void drawslab(int32_t,int32_t,int32_t,int32_t,intptr_t,intptr_t);
void stretchhline(intptr_t,int32_t,int32_t,int32_t,intptr_t,intptr_t);


#ifdef __cplusplus
}
#endif

#endif /* include-once-blocker. */

/* end of a.h ... */


