/*
 * The window's display modes. The names need no SDL: the game's and
 *  editor's settings use them.
 */
#ifndef DISPLAYMODE_H
#define DISPLAYMODE_H

	/* a window; borderless: full screen at the desktop's resolution;
	 * exclusive: full screen at the display mode nearest the game's. */
enum { DISPLAY_WINDOWED, DISPLAY_BORDERLESS, DISPLAY_EXCLUSIVE, DISPLAY_COUNT };

	/* The setting's name of a mode: "windowed", "borderless" or "exclusive". */
static inline const char *display_modename(int mode)
{
	static const char *const names[DISPLAY_COUNT] = { "windowed", "borderless", "exclusive" };
	return names[(mode > 0 && mode < DISPLAY_COUNT) ? mode : DISPLAY_WINDOWED];
}

	/* A mode from its name in any case; -1 for anything else. */
static inline int display_modefromname(const char *s)
{
	int mode, i;

	for(mode=0;mode<DISPLAY_COUNT;mode++)
	{
		const char *n = display_modename(mode);
		for(i=0;n[i] && (s[i]|0x20) == n[i];i++);
		if (!n[i] && !s[i]) return mode;
	}
	return -1;
}

#endif
