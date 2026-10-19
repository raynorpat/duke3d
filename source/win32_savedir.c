/*
 * See win32_savedir.h.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <stdio.h>
#include <stdlib.h>

#include "win32_savedir.h"

int win32_savedgames_dir(char *buf, size_t len)
{
    PWSTR wide = NULL;
    BOOL lossy = FALSE;
    const char *profile;
    int ok = 0;

    if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_SavedGames, 0, NULL, &wide)))
        ok = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wide, -1,
                                 buf, (int)len, NULL, &lossy) != 0 && !lossy;
    CoTaskMemFree(wide);
    if (ok)
        return 0;

    profile = getenv("USERPROFILE");
    if (profile != NULL && profile[0] != '\0' &&
        snprintf(buf, len, "%s\\Saved Games", profile) < (int)len)
        return 0;
    return -1;
}
