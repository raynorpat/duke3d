#include "gameversion.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static int equal(const char *a, const char *b)
{
    while (*a && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { a++; b++; }
    return !*a && !*b;
}

int gameversion_parse_args(int *argc, char **argv, const GameProfile **forced,
                          int *help, char *error, size_t error_size)
{
    int i, out = 1;
    *forced = NULL; *help = 0;
    for (i = 1; i < *argc; i++) {
        if (equal(argv[i], "-gameversion")) {
            if (*forced || i+1 == *argc || !(*forced = game_profile_find(argv[++i]))) {
                snprintf(error, error_size,
                    "Use -gameversion once with: 0.99, 1.0, 1.1, 1.3d-shareware, 1.3d, 1.4, 1.5");
                return 0;
            }
        } else if (equal(argv[i], "-help") || equal(argv[i], "--help") ||
                   !strcmp(argv[i], "?") || !strcmp(argv[i], "/?")) {
            *help = 1;
        } else argv[out++] = argv[i];
    }
    argv[out] = NULL; *argc = out;
    return 1;
}
