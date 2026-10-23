#ifndef DUKE_PROFILE_TEST_SUPPORT_H
#define DUKE_PROFILE_TEST_SUPPORT_H
#include "gameprofile.h"
#include <stdio.h>
#include <string.h>
static int test_select_profile(int *argc, char **argv, const char *fallback_token)
{
    const GameProfile *profile = NULL;
    int i, out=1;
    for (i=1; i<*argc; i++) {
        if (!strcmp(argv[i], "-test-gameversion")) {
            if (profile || i+1==*argc || !(profile=game_profile_find(argv[++i]))) return 0;
        } else argv[out++]=argv[i];
    }
    argv[out]=NULL; *argc=out;
    if (!profile && fallback_token) profile=game_profile_find(fallback_token);
    if (profile) return game_profile_select(profile);
    game_profile_select_build_default();
    return game_profile!=NULL;
}
#endif
