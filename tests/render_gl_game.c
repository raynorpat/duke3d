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

/* What video_setrenderer's mode sets use (the size: video_setmode's). */
void videoswitch_setup(void)
{
    ScreenMode = 1;
    genspriteremaps();  /* the game's water palette */
    myconnectindex = screenpeek = 0;
    ps[0].palette = (char *) palette;  /* as the game has it from the start */
}

/*
 * video_setrenderer, from renderer 3: the viewed player's (water) palette
 *  and a fade (63, 0, 0, 32) are what renderer 0 then shows, and back in 3
 *  the palette is the water one again (not the base one the mode set put
 *  in, nor skipped as already applied), with the fade kept for the quad.
 *  Returns 1 if it all holds.
 */
int videoswitch_palette_check(void)
{
    palette_t water[256], got[256], want[256];
    int ok = 1;

    myconnectindex = screenpeek = 0;
    ud.brightness = 0;
    ps[0].gm = MODE_GAME;
    ps[0].palette = waterpal;
    if (video_setrenderer(3) != 3) { printf("videoswitch: not in renderer 3\n"); return 0; }
    palette_of(waterpal, water);
    glresetgamepalette();
    glsetgamepalette();
    setpalettefade(63, 0, 0, 32);

    video_setrenderer(0);
    memcpy(got, curpalette, sizeof (got));
    setbrightness(0, (unsigned char *) waterpal);
    setpalettefade(63, 0, 0, 32);  /* in renderer 0: blended into the palette */
    memcpy(want, curpalette, sizeof (want));
    if (memcmp(got, want, sizeof (want)) != 0)
        { printf("videoswitch: renderer 0 doesn't show the faded water palette\n"); ok = 0; }

    video_setrenderer(3);
    if (memcmp(curpalette, water, sizeof (water)) != 0)
        { printf("videoswitch: renderer 3 again doesn't have the water palette\n"); ok = 0; }
    if ((palfadedelta != 128) || (palfadergb.r != 252) || (palfadergb.g != 0) || (palfadergb.b != 0))
        { printf("videoswitch: renderer 3 again lost the fade\n"); ok = 0; }
    printf("renderer 3 -> 0 -> 3 with the water palette and a fade: %s\n", ok ? "kept" : "lost");

    ps[0].gm = 0;
    ps[0].palette = (char *) palette;
    setpalettefade(0, 0, 0, 0);
    return ok;
}
