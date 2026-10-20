/*
 * render_gl's checks of the game's renderer 3 palette code (menues.c
 *  glsetgamepalette), which need the game's headers: render_gl.c keeps
 *  only the engine's.
 */
#include "duke3d.h"

void gltexinvalidateall(void);  /* polymost.c */

/* curpalette after setbrightness(0, pal). */
static void palette_of(char *pal, palette_t *out)
{
    setbrightness(0, (unsigned char *) pal);
    memcpy(out, curpalette, 256 * sizeof (palette_t));
}

/*
 * glsetgamepalette, with renderer 3 running: applies a new player palette
 *  keeping the fade, lights glow (fullbright 240) in the game's palettes and
 *  not in the title one, and in a game the palette is the viewed player's
 *  (screenpeek), not myconnectindex's. Returns 1 if it all holds.
 */
int glgamepalette_check(void)
{
    palette_t normal[256], water[256];
    int ok = 1;

    genspriteremaps();  /* the game's water, title, ... palettes */
    palette_of(waterpal, water);
    palette_of((char *) palette, normal);
    myconnectindex = screenpeek = 0;
    ud.brightness = 0;
    ps[0].gm = 0;
    ps[0].palette = ps[1].palette = (char *) palette;
    glsetgamepalette();

    setpalettefade(63, 0, 0, 32);
    ps[0].palette = waterpal;
    glsetgamepalette();
    if ((memcmp(curpalette, water, sizeof (water)) != 0) || (palfadedelta != 128))
        { printf("glsetgamepalette: water palette or fade not kept\n"); ok = 0; }
    if (polymosttexfullbright != 240)
        { printf("glsetgamepalette: no fullbright in the water palette\n"); ok = 0; }

    ps[0].palette = titlepal;
    glsetgamepalette();
    if (polymosttexfullbright != 256)
        { printf("glsetgamepalette: fullbright in the title palette\n"); ok = 0; }

    /* In a game, viewing player 1 while player 0 (this one) is underwater. */
    ps[0].gm = MODE_GAME;
    ps[0].palette = waterpal;
    screenpeek = 1;
    glsetgamepalette();
    if (memcmp(curpalette, normal, sizeof (normal)) != 0)
        { printf("glsetgamepalette: not the viewed player's palette\n"); ok = 0; }

    ps[0].gm = 0;
    ps[0].palette = (char *) palette;
    myconnectindex = screenpeek = 0;
    setpalettefade(0, 0, 0, 0);
    polymosttexfullbright = 256;
    gltexinvalidateall();
    setbrightness(0, palette);
    return ok;
}
