/*
 * The Windows "Saved Games" folder, kept apart from the game sources so
 * <windows.h> never meets their typedefs.
 */
#ifndef _INCL_WIN32_SAVEDIR_H_
#define _INCL_WIN32_SAVEDIR_H_

#include <stddef.h>

/* Writes the user's Saved Games folder, without a trailing separator, in
   the ANSI code page the game's file calls use. Returns 0, or -1 if it
   can't be found or represented. */
int win32_savedgames_dir(char *buf, size_t len);

#endif
