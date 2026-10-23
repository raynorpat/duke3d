#ifndef DUKE_GAMEVERSION_H
#define DUKE_GAMEVERSION_H
#include "gameprofile.h"
int gameversion_parse_args(int *argc, char **argv, const GameProfile **forced,
                          int *help, char *error, size_t error_size);
#endif
