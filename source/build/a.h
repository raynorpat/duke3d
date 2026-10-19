/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */

#ifndef _INCLUDE_A_H_
#define _INCLUDE_A_H_
#include <stdint.h>

#if (defined __WATCOMC__)
#error Do not include this header with Watcom C.
#endif

#ifdef __cplusplus
extern "C" {
#endif

int32_t mmxoverlay(void);
int32_t sethlinesizes(int32_t,int32_t,int32_t);
int32_t setpalookupaddress(char *);
int32_t setuphlineasm4(int32_t,int32_t);
int32_t hlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t setuprhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t rhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t setuprmhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t rmhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t setupqrhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t qrhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t setvlinebpl(int32_t);
int32_t fixtransluscence(int32_t);
int32_t prevlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t vlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t setuptvlineasm(int32_t);
int32_t tvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t setuptvlineasm2(int32_t,int32_t,int32_t);
int32_t tvlineasm2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t mvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t setupvlineasm(int32_t);
int32_t vlineasm4(int32_t,int32_t);
int32_t setupmvlineasm(int32_t);
int32_t mvlineasm4(int32_t,int32_t);
void setupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
void spritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
void msetupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
void mspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
void tsetupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
void tspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t mhline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t mhlineskipmodify(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t msethlineshift(int32_t,int32_t);
int32_t thline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t thlineskipmodify(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t tsethlineshift(int32_t,int32_t);
int32_t setupslopevlin(int32_t,int32_t,int32_t);
int32_t slopevlin(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t settransnormal(void);
int32_t settransreverse(void);
int32_t setupdrawslab(int32_t,int32_t);
int32_t drawslab(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t stretchhline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
int32_t is_vmware_running(void);

    /* !!! This part might be better stated as "USE_ASM".  --ryan. */
#ifdef USE_I386_ASM
  int32_t asm_mmxoverlay(void);
  int32_t asm_sethlinesizes(int32_t,int32_t,int32_t);
  int32_t asm_setpalookupaddress(char *);
  int32_t asm_setuphlineasm4(int32_t,int32_t);
  int32_t asm_hlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_setuprhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_rhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_setuprmhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_rmhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_setupqrhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_qrhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_setvlinebpl(int32_t);
  int32_t asm_fixtransluscence(int32_t);
  int32_t asm_prevlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_vlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_setuptvlineasm(int32_t);
  int32_t asm_tvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_setuptvlineasm2(int32_t,int32_t,int32_t);
  int32_t asm_tvlineasm2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_mvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_setupvlineasm(int32_t);
  int32_t asm_vlineasm4(int32_t,int32_t);
  int32_t asm_setupmvlineasm(int32_t);
  int32_t asm_mvlineasm4(int32_t,int32_t);
  void asm_setupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  void asm_spritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  void asm_msetupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  void asm_mspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  void asm_tsetupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  void asm_tspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_mhline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_mhlineskipmodify(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_msethlineshift(int32_t,int32_t);
  int32_t asm_thline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_thlineskipmodify(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_tsethlineshift(int32_t,int32_t);
  int32_t asm_setupslopevlin(int32_t,int32_t,int32_t);
  int32_t asm_slopevlin(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_settransnormal(void);
  int32_t asm_settransreverse(void);
  int32_t asm_setupdrawslab(int32_t,int32_t);
  int32_t asm_drawslab(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_stretchhline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t);
  int32_t asm_isvmwarerunning(void);

  /*
   * !!! I need a reference to this, for mprotect(), but the actual function
   * !!!  is never called in BUILD...just from other ASM routines. --ryan.
   */
  int32_t asm_prohlineasm4(void);

  #if ((defined __GNUC__) && (!defined C_IDENTIFIERS_UNDERSCORED))

    int32_t asm_mmxoverlay(void) __attribute__ ((alias ("_asm_mmxoverlay")));
    int32_t asm_sethlinesizes(int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_sethlinesizes")));
    int32_t asm_setpalookupaddress(char *) __attribute__ ((alias ("_asm_setpalookupaddress")));
    int32_t asm_setuphlineasm4(int32_t,int32_t) __attribute__ ((alias ("_asm_setuphlineasm4")));
    int32_t asm_hlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_hlineasm4")));
    int32_t asm_setuprhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_setuprhlineasm4")));
    int32_t asm_rhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_rhlineasm4")));
    int32_t asm_setuprmhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_setuprmhlineasm4")));
    int32_t asm_rmhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_rmhlineasm4")));
    int32_t asm_setupqrhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_setupqrhlineasm4")));
    int32_t asm_qrhlineasm4(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_qrhlineasm4")));
    int32_t asm_setvlinebpl(int32_t) __attribute__ ((alias ("_asm_setvlinebpl")));
    int32_t asm_fixtransluscence(int32_t) __attribute__ ((alias ("_asm_fixtransluscence")));
    int32_t asm_prevlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_prevlineasm1")));
    int32_t asm_vlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_vlineasm1")));
    int32_t asm_setuptvlineasm(int32_t) __attribute__ ((alias ("_asm_setuptvlineasm")));
    int32_t asm_tvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_tvlineasm1")));
    int32_t asm_setuptvlineasm2(int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_setuptvlineasm2")));
    int32_t asm_tvlineasm2(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_tvlineasm2")));
    int32_t asm_mvlineasm1(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_mvlineasm1")));
    int32_t asm_setupvlineasm(int32_t) __attribute__ ((alias ("_asm_setupvlineasm")));
    int32_t asm_vlineasm4(int32_t,int32_t) __attribute__ ((alias ("_asm_vlineasm4")));
    int32_t asm_setupmvlineasm(int32_t) __attribute__ ((alias ("_asm_setupmvlineasm")));
    int32_t asm_mvlineasm4(int32_t,int32_t) __attribute__ ((alias ("_asm_mvlineasm4")));
    void asm_setupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_setupspritevline")));
    void asm_spritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_spritevline")));
    void asm_msetupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_msetupspritevline")));
    void asm_mspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_mspritevline")));
    void asm_tsetupspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_tsetupspritevline")));
    void asm_tspritevline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_tspritevline")));
    int32_t asm_mhline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_mhline")));
    int32_t asm_mhlineskipmodify(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_mhlineskipmodify")));
    int32_t asm_msethlineshift(int32_t,int32_t) __attribute__ ((alias ("_asm_msethlineshift")));
    int32_t asm_thline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_thline")));
    int32_t asm_thlineskipmodify(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_thlineskipmodify")));
    int32_t asm_tsethlineshift(int32_t,int32_t) __attribute__ ((alias ("_asm_tsethlineshift")));
    int32_t asm_setupslopevlin(int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_setupslopevlin")));
    int32_t asm_slopevlin(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_slopevlin")));
    int32_t asm_settransnormal(void) __attribute__ ((alias ("_asm_settransnormal")));
    int32_t asm_settransreverse(void) __attribute__ ((alias ("_asm_settransreverse")));
    int32_t asm_setupdrawslab(int32_t,int32_t) __attribute__ ((alias ("_asm_setupdrawslab")));
    int32_t asm_drawslab(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_drawslab")));
    int32_t asm_stretchhline(int32_t,int32_t,int32_t,int32_t,int32_t,int32_t) __attribute__ ((alias ("_asm_stretchhline")));
    int32_t asm_isvmwarerunning(void) __attribute__ ((alias ("_asm_isvmwarerunning")));

    /*
	 * !!! I need a reference to this, for mprotect(), but the actual function
     * !!!  is never called in BUILD...just from other ASM routines. --ryan.
	 */
    int32_t asm_prohlineasm4(void) __attribute__ ((alias ("_asm_prohlineasm4")));

  #endif /* ELF/GCC */
#endif /* defined USE_I386_ASM */

#ifdef __cplusplus
}
#endif

#endif /* include-once-blocker. */

/* end of a.h ... */


