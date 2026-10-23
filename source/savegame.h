/*
 * Save files use the selected release's DOS layout: records packed to one byte,
 * little-endian values, and the original dfwrite element sizes and counts.
 */
#ifndef _INCL_SAVEGAME_H_
#define _INCL_SAVEGAME_H_

#include "gameprofile.h"

/* Counts and grouping on disk belong to the selected release. */
typedef struct SaveGameLayout {
    int player_size, dos_maxplayers, dos_maxhittypes, dos_maxtiles, dos_maxscript, player_dfsize, player_dfcount, player_block0, playerorig_dfsize, playerorig_dfcount;
} SaveGameLayout;
const SaveGameLayout *sg_layout(void);

#define SG_WALL_SIZE 32
#define SG_SECTOR_SIZE 40
#define SG_SPRITE_SIZE 44
#define SG_PLAYERORIG_SIZE 16
#define SG_WEAPONHIT_SIZE 71
#define SG_WEAPONHIT_TEMPDATA 47
#define SG_ANIMWALL_SIZE 6
#define SG_PLAYER_SIZE (sg_layout()->player_size)
#define SG_DOS_MAXPLAYERS (sg_layout()->dos_maxplayers)
#define SG_DOS_MAXHITTYPES (sg_layout()->dos_maxhittypes)
#define SG_DOS_MAXTILES (sg_layout()->dos_maxtiles)
#define SG_DOS_MAXSCRIPT (sg_layout()->dos_maxscript)
#define SG_PLAYER_DFSIZE (sg_layout()->player_dfsize)
#define SG_PLAYER_DFCOUNT (sg_layout()->player_dfcount)
#define SG_PLAYER_BLOCK0 (sg_layout()->player_block0)
#define SG_PLAYERORIG_DFSIZE (sg_layout()->playerorig_dfsize)
#define SG_PLAYERORIG_DFCOUNT (sg_layout()->playerorig_dfcount)

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

/* 0.99 and 1.0 saves store script addresses (actorscrptr[] minus script's address,
   hittype temp_data raw) as the DOS game had them. Its script was at this
   address in the DOSBox reference saves; we write saves as if it were. */
#define SG_DOS10_SCRIPTBASE (game_profile->id==GAME_099SW ? 0x2be018 : 0x2d5f08)

/* Checks that the whole file, from fil's position, has this build's unversioned save layout
   (docs/v10-v11/reference.md, "Save file") and that its script addresses all
   point into the script. Leaves fil where it was. Returns the address the
   save's script was at, or -1 if it isn't a sound save of this build's version. */
int32_t sg_check10(int32_t fil);

/* The hittype temp_data slots that hold script addresses: 1, 4 and 5. */
extern const int32_t sg_scriptslots10[3];

/* Check every compressed call, actor script table, flagged script/hittype
   offsets and live animation offsets, starting just after the version block.
   Leaves fil where it was. */
int32_t sg_checksave(int32_t fil);

/* Uncompressed size of the first ps[] block, found without loading anything.
   fil must be just past the version block, and is left there. Returns -1
   if the file ends first. */
int32_t sg_playerblocksize(int32_t fil);

#endif
