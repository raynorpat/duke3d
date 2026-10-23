#include "gameprofile.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static const GameProfile profiles[] = {
    { GAME_099SW, "0.99", "0.99 shareware", GAMEVER_099, 19960104, 9, 1, 0 },
    { GAME_10SW, "1.0", "1.0 shareware", GAMEVER_10, 19960129, 10, 1, 0 },
    { GAME_11SW, "1.1", "1.1 shareware", GAMEVER_11, 19960220, 12, 1, 0 },
    { GAME_13DSW, "1.3d-shareware", "1.3D shareware", GAMEVER_13D, 19960419, 14, 1, 0 },
    { GAME_13D, "1.3d", "1.3D full", GAMEVER_13D, 19960419, 103, 0, 0 },
    { GAME_14, "1.4", "1.4 Plutonium Pak", GAMEVER_14, 19960820, 116, 0, 1 },
    { GAME_15, "1.5", "1.5 Atomic Edition", GAMEVER_15, 20000614, 117, 0, 1 }
};
static const GameGrp grps[] = {
    { 9690241, 0x02f18900, "0.99 shareware", &profiles[GAME_099SW], 1 },
    { 10429258, 0xa28aa589, "1.0 shareware", &profiles[GAME_10SW], 1 },
    { 10442980, 0x912e1e8d, "1.1 shareware", &profiles[GAME_11SW], 1 },
    { 11035779, 0x983ad923, "1.3D shareware", &profiles[GAME_13DSW], 1 },
    { 10444391, 0xc5f71561, "1.5 Mac shareware (unsupported)", NULL, 1 },
    { 26524524, 0xbbc9ce44, "1.3D full", &profiles[GAME_13D], 0 },
    { 44348015, 0xf514a6ac, "1.4 Plutonium Pak", &profiles[GAME_14], 0 },
    { 44356548, 0xfd3dcff1, "1.5 Atomic Edition", &profiles[GAME_15], 0 }
};
const GameProfile *game_profile;

const GameProfile *game_profile_get(GameProfileId id)
{
    return (unsigned)id < GAME_PROFILE_COUNT ? &profiles[id] : NULL;
}

const GameProfile *game_profile_find(const char *token)
{
    int i;
    for (i = 0; i < GAME_PROFILE_COUNT; i++) {
        const unsigned char *a = (const unsigned char *)token;
        const unsigned char *b = (const unsigned char *)profiles[i].token;
        while (*a && tolower(*a) == tolower(*b)) { a++; b++; }
        if (!*a && !*b) return &profiles[i];
    }
    return NULL;
}

const GameGrp *game_grp_find(uint64_t size, uint32_t crc)
{
    size_t i;
    for (i = 0; i < sizeof(grps)/sizeof(grps[0]); i++)
        if (grps[i].size == size && grps[i].crc == crc) return &grps[i];
    return NULL;
}

GameResolveResult game_profile_resolve(const GameGrp *detected,
    const GameProfile *forced, const GameProfile **selected)
{
    *selected = NULL;
    if (forced) {
        if (detected && detected->shareware && !forced->shareware)
            return GAME_RESOLVE_SHAREWARE_CONFLICT;
        *selected = forced;
        return GAME_RESOLVE_OK;
    }
    if (!detected) return GAME_RESOLVE_NEEDS_OVERRIDE;
    if (!detected->profile) return GAME_RESOLVE_UNSUPPORTED;
    *selected = detected->profile;
    return GAME_RESOLVE_OK;
}

int game_profile_select(const GameProfile *profile)
{
    if (!profile || (game_profile && game_profile != profile)) return 0;
    game_profile = profile;
    return 1;
}

uint32_t grpcrc32(uint32_t crc, const unsigned char *buf, size_t n)
{
    static uint32_t table[256];
    uint32_t c;
    int i, j;
    if (table[1] == 0)
        for (i = 0; i < 256; i++) {
            for (c = i, j = 0; j < 8; j++) c = (c & 1) ? 0xEDB88320 ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
    crc = ~crc;
    while (n--) crc = table[(crc ^ *buf++) & 255] ^ (crc >> 8);
    return ~crc;
}

static uint32_t le32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}

int game_grp_inspect(const char *path, uint64_t *size, uint32_t *crc,
                     char *error, size_t error_size)
{
    unsigned char buf[65536];
    FILE *fp = fopen(path, "rb");
    uint32_t count, i;
    uint64_t required;
    size_t n;
    const char *reason = "invalid or truncated GRP";
    *size = 0; *crc = 0;
    if (!fp) {
        snprintf(error, error_size, "%s: %s", path, strerror(errno));
        return 0;
    }
    if (fread(buf, 1, 16, fp) != 16 || memcmp(buf, "KenSilverman", 12)) goto invalid;
    *crc = grpcrc32(0, buf, 16); *size = 16;
    count = le32(buf+12);
    /* Bound all directory entries and member extents before the legacy mounter. */
    required = 16 + (uint64_t)count*16;
    for (i = 0; i < count; i++) {
        if (fread(buf, 1, 16, fp) != 16) goto invalid;
        *crc = grpcrc32(*crc, buf, 16); *size += 16;
        if (UINT64_MAX-required < le32(buf+12)) goto invalid;
        required += le32(buf+12);
    }
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        *crc = grpcrc32(*crc, buf, n); *size += n;
    }
    if (ferror(fp)) { reason = "GRP read error"; goto invalid; }
    if (*size < required) goto invalid;
    fclose(fp);
    return 1;
invalid:
    if (ferror(fp)) reason = "GRP read error";
    snprintf(error, error_size, "%s: %s", path, reason);
    fclose(fp);
    return 0;
}
