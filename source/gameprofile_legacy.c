/* Temporary bridge for incremental conversion; removed with build variants. */
#include "gameprofile.h"
void game_profile_select_build_default(void)
{
    if (game_profile) return;
#if DUKE3D_GAMEVER < GAMEVER_10
    game_profile_select(game_profile_get(GAME_099SW));
#elif DUKE3D_GAMEVER < GAMEVER_11
    game_profile_select(game_profile_get(GAME_10SW));
#elif DUKE3D_GAMEVER < GAMEVER_13D
    game_profile_select(game_profile_get(GAME_11SW));
#elif DUKE3D_GAMEVER < GAMEVER_14
#ifdef VOLUMEONE
    game_profile_select(game_profile_get(GAME_13DSW));
#else
    game_profile_select(game_profile_get(GAME_13D));
#endif
#elif DUKE3D_GAMEVER < GAMEVER_15
    game_profile_select(game_profile_get(GAME_14));
#else
    game_profile_select(game_profile_get(GAME_15));
#endif
}
