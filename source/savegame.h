/*
 * Save files use the DOS layout of the build's version (1.5, 1.3D in 1.3D
 * builds, 1.1, 1.0 or 0.99 in those builds): every record packed to 1 byte, every
 * value little-endian, written with the same dfwrite calls (element size and
 * count) the DOS game made, so the LZW blocks come out the same.
 */
#ifndef _INCL_SAVEGAME_H_
#define _INCL_SAVEGAME_H_

#include "gamever.h"

#define SG_WALL_SIZE        32
#define SG_SECTOR_SIZE      40
#define SG_SPRITE_SIZE      44
#if DUKE3D_GAMEVER < GAMEVER_13D
#if DUKE3D_GAMEVER < GAMEVER_10
#define SG_PLAYER_SIZE      0x33f   /* the 0.99 beta player_struct */
#else
#define SG_PLAYER_SIZE      0x34e   /* the 1.1 and 1.0 player_struct */
#endif
#define SG_DOS_MAXHITTYPES  2048    /* as in 1.3D */
#define SG_DOS_MAXPLAYERS   8       /* 1.1 and 1.0 save ps[] and po[] for 8 players */
/* They write ps[] and po[] as 8 elements each, both in one block. */
#define SG_PLAYER_DFSIZE    SG_PLAYER_SIZE
#define SG_PLAYER_DFCOUNT   SG_DOS_MAXPLAYERS
#define SG_PLAYER_BLOCK0    (SG_DOS_MAXPLAYERS*SG_PLAYER_SIZE)
#define SG_DOS_MAXSCRIPT    10240   /* script[] entries in a 1.1 or 1.0 save */
#elif DUKE3D_GAMEVER < GAMEVER_14
#define SG_PLAYER_SIZE      0x552   /* the 1.3D player_struct */
#define SG_DOS_MAXHITTYPES  2048    /* 1.3D saves hittype[] for the first 2048 sprites only */
/* 1.3D writes ps[] as MAXPLAYERS elements, which dfwrite splits after 12. */
#define SG_PLAYER_DFSIZE    SG_PLAYER_SIZE
#define SG_PLAYER_DFCOUNT   MAXPLAYERS
#define SG_PLAYER_BLOCK0    (12*SG_PLAYER_SIZE)
#else
#define SG_PLAYER_SIZE      911
/* 1.5 writes ps[] as one element, which fits in one block. */
#define SG_PLAYER_DFSIZE    (SG_PLAYER_SIZE*MAXPLAYERS)
#define SG_PLAYER_DFCOUNT   1
#define SG_PLAYER_BLOCK0    (SG_PLAYER_SIZE*MAXPLAYERS)
#endif
#define SG_PLAYERORIG_SIZE  16
#if DUKE3D_GAMEVER < GAMEVER_13D
#define SG_PLAYERORIG_DFSIZE  SG_PLAYERORIG_SIZE
#define SG_PLAYERORIG_DFCOUNT SG_DOS_MAXPLAYERS
#else
#define SG_DOS_MAXPLAYERS     MAXPLAYERS
#define SG_DOS_MAXSCRIPT      MAXSCRIPTSIZE
#define SG_PLAYERORIG_DFSIZE  (SG_PLAYERORIG_SIZE*MAXPLAYERS)
#define SG_PLAYERORIG_DFCOUNT 1
#endif
#define SG_WEAPONHIT_SIZE   71
#define SG_WEAPONHIT_TEMPDATA 47    /* temp_data[0]'s offset in it */
#define SG_ANIMWALL_SIZE    6
#if DUKE3D_GAMEVER < GAMEVER_14
#define SG_DOS_MAXTILES     4096    /* actorscrptr[] entries in a 1.3D save */
#else
#define SG_DOS_MAXTILES     6144    /* actortype[] and actorscrptr[] entries in a save */
#endif

/* Converts one record: write != 0 is struct -> bytes, 0 is bytes -> struct.
   Returns the bytes used. */
typedef int32_t (*sg_rec)(unsigned char *b, void *rec, int write);

int32_t sg_wall(unsigned char *b, void *rec, int write);
int32_t sg_sector(unsigned char *b, void *rec, int write);
int32_t sg_sprite(unsigned char *b, void *rec, int write);
int32_t sg_player(unsigned char *b, void *rec, int write);
int32_t sg_playerorig(unsigned char *b, void *rec, int write);
int32_t sg_weaponhit(unsigned char *b, void *rec, int write);
int32_t sg_animwall(unsigned char *b, void *rec, int write);

void sg_write8(FILE *fil, int32_t v);
void sg_write16(FILE *fil, int32_t v);
void sg_write32(FILE *fil, int32_t v);
int32_t sg_read8(int32_t fil);
int32_t sg_read16(int32_t fil);
int32_t sg_read32(int32_t fil);

/* dasizeof * count bytes of shorts or int32s, as one dfwrite/kdfread call. */
void sg_write16s(FILE *fil, short *a, int32_t dasizeof, int32_t count);
void sg_read16s(int32_t fil, short *a, int32_t dasizeof, int32_t count);
/* Like sg_read16s, but stores at most n shorts, and clamps count so the read
   can't overflow the codec's buffer. */
void sg_read16sn(int32_t fil, short *a, int32_t n, int32_t dasizeof, int32_t count);
void sg_write32s(FILE *fil, int32_t *a, int32_t dasizeof, int32_t count);
void sg_read32s(int32_t fil, int32_t *a, int32_t dasizeof, int32_t count);

/* n records, stride bytes apart in memory, as one dfwrite/kdfread call of
   count elements of dasizeof bytes. */
void sg_writerecs(FILE *fil, void *recs, size_t stride, sg_rec fn, int32_t n, int32_t dasizeof, int32_t count);
void sg_readrecs(int32_t fil, void *recs, size_t stride, sg_rec fn, int32_t n, int32_t dasizeof, int32_t count);

/* Resolve wall offsets independently of native linker placement. dos is true
   when any script pointer bitmap entry is set; otherwise ambiguous offsets
   are refused. Returns the byte offset within wall[], or -1. */
int32_t sg_walloffset(int32_t offset, int64_t nativebase, int32_t dos);
/* Resolve a live animation pointer with the same provenance. */
int32_t *sg_animptr(int32_t offset, int32_t dos);

/* LZW blocks dfwrite emits for one call. */
int32_t sg_dfblocks(int32_t dasizeof, int32_t count);

#if DUKE3D_GAMEVER < GAMEVER_11
/* 0.99 and 1.0 saves store script addresses (actorscrptr[] minus script's address,
   hittype temp_data raw) as the DOS game had them. Its script was at this
   address in the DOSBox reference saves; we write saves as if it were. */
#if DUKE3D_GAMEVER < GAMEVER_10
#define SG_DOS10_SCRIPTBASE 0x2be018
#else
#define SG_DOS10_SCRIPTBASE 0x2d5f08
#endif

/* Checks that the whole file, from fil's position, has this build's unversioned save layout
   (docs/v10-v11/reference.md, "Save file") and that its script addresses all
   point into the script. Leaves fil where it was. Returns the address the
   save's script was at, or -1 if it isn't a sound save of this build's version. */
int32_t sg_check10(int32_t fil);

/* The hittype temp_data slots that hold script addresses: 1, 4 and 5. */
extern const int32_t sg_scriptslots10[3];
#endif

#if DUKE3D_GAMEVER >= GAMEVER_11
/* Check every compressed call, actor script table, flagged script/hittype
   offsets and live animation offsets, starting just after the version block.
   Leaves fil where it was. */
int32_t sg_checksave(int32_t fil);

/* Uncompressed size of the first ps[] block, found without loading anything.
   fil must be just past the version block, and is left there. Returns -1
   if the file ends first. */
int32_t sg_playerblocksize(int32_t fil);
#endif

#endif
