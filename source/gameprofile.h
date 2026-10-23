#ifndef DUKE_GAMEPROFILE_H
#define DUKE_GAMEPROFILE_H
#include <stddef.h>
#include <stdint.h>
#include "gamever.h"

typedef enum {
    GAME_099SW, GAME_10SW, GAME_11SW, GAME_13DSW,
    GAME_13D, GAME_14, GAME_15, GAME_PROFILE_COUNT
} GameProfileId;
typedef struct {
    GameProfileId id;
    const char *token, *name;
    int32_t gamever, enginerev;
    unsigned char byteversion, shareware, plutopak;
} GameProfile;
typedef struct {
    uint64_t size;
    uint32_t crc;
    const char *name;
    const GameProfile *profile;
    int shareware;
} GameGrp;
extern const GameProfile *game_profile;
const GameProfile *game_profile_get(GameProfileId id);
const GameProfile *game_profile_find(const char *token);
const GameGrp *game_grp_find(uint64_t size, uint32_t crc);
uint32_t grpcrc32(uint32_t crc, const unsigned char *buf, size_t count);
int game_grp_inspect(const char *path, uint64_t *size, uint32_t *crc,
                     char *error, size_t error_size);
typedef enum { GAME_RESOLVE_OK, GAME_RESOLVE_NEEDS_OVERRIDE,
               GAME_RESOLVE_UNSUPPORTED, GAME_RESOLVE_SHAREWARE_CONFLICT }
    GameResolveResult;
GameResolveResult game_profile_resolve(const GameGrp *detected,
    const GameProfile *forced, const GameProfile **selected);
void game_profile_select_build_default(void); /* removed after migration */
int game_profile_select(const GameProfile *profile);
#endif
