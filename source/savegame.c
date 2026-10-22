/*
 * Converts saved records to and from the DOS save layout. See savegame.h.
 */
#include "duke3d.h"
#include "savegame.h"

#define SG_LZWSIZE 16384    /* LZWSIZE in build/cache1d.c */

static unsigned char sgbuf[MAXSPRITES*SG_WEAPONHIT_SIZE];   /* the largest block */

static void put16(unsigned char *b, int32_t v) { b[0] = (unsigned char)v; b[1] = (unsigned char)(v>>8); }
static void put32(unsigned char *b, int32_t v) { put16(b,v); put16(b+2,v>>16); }
static int32_t get16(const unsigned char *b) { return (short)(b[0]|(b[1]<<8)); }
static int32_t get32(const unsigned char *b)
{
    return (int32_t)((uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24));
}

/* One field each, in declaration order. Uses b, write, o and k. */
#define F8(x)    do { if (write) b[o] = (unsigned char)(x); else (x) = b[o]; o += 1; } while (0)
#define F16(x)   do { if (write) put16(b+o,(x)); else (x) = get16(b+o); o += 2; } while (0)
#define F32(x)   do { if (write) put32(b+o,(x)); else (x) = get32(b+o); o += 4; } while (0)
#define A8(a,n)  do { for (k = 0; k < (n); k++) F8((a)[k]); } while (0)
#define A16(a,n) do { for (k = 0; k < (n); k++) F16((a)[k]); } while (0)
#define A32(a,n) do { for (k = 0; k < (n); k++) F32((a)[k]); } while (0)

int32_t sg_wall(unsigned char *b, void *rec, int write)
{
    walltype *w = (walltype *)rec;
    int32_t o = 0;

    F32(w->x); F32(w->y);
    F16(w->point2); F16(w->nextwall); F16(w->nextsector); F16(w->cstat);
    F16(w->picnum); F16(w->overpicnum);
    F8(w->shade);
    F8(w->pal); F8(w->xrepeat); F8(w->yrepeat); F8(w->xpanning); F8(w->ypanning);
    F16(w->lotag); F16(w->hitag); F16(w->extra);
    return o;
}

int32_t sg_sector(unsigned char *b, void *rec, int write)
{
    sectortype *s = (sectortype *)rec;
    int32_t o = 0;

    F16(s->wallptr); F16(s->wallnum);
    F32(s->ceilingz); F32(s->floorz);
    F16(s->ceilingstat); F16(s->floorstat);
    F16(s->ceilingpicnum); F16(s->ceilingheinum);
    F8(s->ceilingshade);
    F8(s->ceilingpal); F8(s->ceilingxpanning); F8(s->ceilingypanning);
    F16(s->floorpicnum); F16(s->floorheinum);
    F8(s->floorshade);
    F8(s->floorpal); F8(s->floorxpanning); F8(s->floorypanning);
    F8(s->visibility); F8(s->filler);
    F16(s->lotag); F16(s->hitag); F16(s->extra);
    return o;
}

int32_t sg_sprite(unsigned char *b, void *rec, int write)
{
    spritetype *s = (spritetype *)rec;
    int32_t o = 0;

    F32(s->x); F32(s->y); F32(s->z);
    F16(s->cstat); F16(s->picnum);
    F8(s->shade);
    F8(s->pal); F8(s->clipdist); F8(s->filler);
    F8(s->xrepeat); F8(s->yrepeat);
    F8(s->xoffset); F8(s->yoffset);
    F16(s->sectnum); F16(s->statnum);
    F16(s->ang); F16(s->owner); F16(s->xvel); F16(s->yvel); F16(s->zvel);
    F16(s->lotag); F16(s->hitag); F16(s->extra);
    return o;
}

#if DUKE3D_GAMEVER < GAMEVER_13D
/*
 * The DOS 1.1 player_struct (0x34e bytes, docs/v10-v11/reference.md,
 * "player_struct (1.1)"): no bobposx/bobposy, subweapon, wantweaponfire or
 * weaprecs, 10 weapons, frag_ps and interface_toggle_flag are bytes, and the
 * 1.1-only fields sit between ours. on_sector_effector, no_autoaim and
 * select_dir aren't carried: written as zeros, ignored on reading.
 * The 1.0 one ("player_struct (1.0)") is the same size: it has a byte at
 * 0x322 and a per-player auto_run, and no no_autoaim or aim_mode. The beta
 * has 0x33f bytes (docs/v099/notes/0.99-player.md): the byte group precedes
 * inventory shorts, several later fields are absent, and display flags
 * are per-player. Only the active player maps to the native globals.
 */
int32_t sg_player(unsigned char *b, void *rec, int write)
{
    struct player_struct *p = (struct player_struct *)rec;
    int32_t o = 0, k;
#if DUKE3D_GAMEVER < GAMEVER_10
    int32_t display;
#endif

    if (!write)
    {
#if DUKE3D_GAMEVER < GAMEVER_10
        memset(p,0,sizeof(*p));
        p->wackedbyactor = p->scream_voice = -1;
#endif
        p->bobposx = p->bobposy = 0;
        p->subweapon = 0; p->wantweaponfire = 0;
        p->weapreccnt = 0;
        p->ammo_amount[10] = p->ammo_amount[11] = 0;
        p->gotweapon[10] = p->gotweapon[11] = 0;
    }

    F32(p->zoom); F32(p->exitx); F32(p->exity);
    A32(p->loogiex,64); A32(p->loogiey,64); F32(p->numloogs); F32(p->loogcnt);
    F32(p->posx); F32(p->posy); F32(p->posz); F32(p->horiz); F32(p->ohoriz);
    F32(p->ohorizoff); F32(p->invdisptime);
    F32(p->oposx); F32(p->oposy); F32(p->oposz);
    F32(p->pyoff);
#if DUKE3D_GAMEVER >= GAMEVER_10
    F32(p->opyoff);
#endif
    F32(p->posxv); F32(p->posyv); F32(p->poszv); F32(p->last_pissed_time);
#if DUKE3D_GAMEVER >= GAMEVER_10
    F32(p->truefz); F32(p->truecz);
#endif
    F32(p->player_par); F32(p->visibility);
    F32(p->bobcounter); F32(p->weapon_sway);
    F32(p->pals_time); F32(p->burn_time); F32(p->randomflamex); F32(p->crack_time);

    F16(p->ang); F16(p->oang); F16(p->angvel); F16(p->cursectnum); F16(p->look_ang);
    F16(p->last_extra);
    A16(p->ammo_amount,10);
#if DUKE3D_GAMEVER >= GAMEVER_10
    F16(p->wackedbyactor);
#endif
    F16(p->curr_weapon); F16(p->last_weapon); F16(p->tipincs); F16(p->horizoff);
    F16(p->holoduke_amount); F16(p->newowner); F16(p->hurt_delay); F16(p->hbomb_hold_delay);
    F16(p->jumping_counter); F16(p->airleft); F16(p->knee_incs); F16(p->access_incs);
    F16(p->fta); F16(p->ftq); F16(p->access_wallnum); F16(p->access_spritenum);
    F16(p->kickback_pic); F16(p->got_access); F16(p->weapon_ang); F16(p->firstaid_amount);
    F16(p->somethingonplayer); F16(p->on_crane); F16(p->i); F16(p->one_parallax_sectnum);
    F16(p->over_shoulder_on); F16(p->random_club_frame); F16(p->fist_incs);
    F16(p->frag); F16(p->fraggedself);
    F16(p->one_eighty_count); F16(p->cheat_phase);
    F16(p->dummyplayersprite); F16(p->extra_extra8); F16(p->quick_kick);
    F16(p->heat_amount); F16(p->actorsqu); F16(p->timebeforeexit); F16(p->customexitsound);
#if DUKE3D_GAMEVER >= GAMEVER_10
    F16(p->rotscrnang); F16(p->dead_flag); F16(p->show_empty_weapon);
    F16(p->scuba_amount); F16(p->jetpack_amount); F16(p->steroids_amount); F16(p->shield_amount);
    F16(p->spacesuit_amount);
    F16(p->holoduke_on); F16(p->pycount); F16(p->weapon_pos);
    F16(p->transporter_hold); F16(p->last_full_weapon); F16(p->footprintshade); F16(p->boot_amount);

    F32(p->scream_voice);

#endif
    F8(p->gm); F8(p->on_warping_sector); F8(p->footprintcount);
    F8(p->hbomb_on); F8(p->jumping_toggle);
    if (write) b[o] = 0;    /* on_sector_effector */
    o++;
    F8(p->rapid_fire_hold); F8(p->on_ground);
    A8(p->name,13); F8(p->inven_icon);
#if DUKE3D_GAMEVER < GAMEVER_11
    F8(p->unnamed_322);
#endif
    F8(p->buttonpalette);
    F8(p->jetpack_on); F8(p->spritebridge); F8(p->lastrandomspot);
#if DUKE3D_GAMEVER >= GAMEVER_11
    if (write) b[o] = 0;    /* no_autoaim */
    o++;
#endif
    F8(p->spacesuit_on); F8(p->scuba_on); F8(p->spacesuit_used);
    F8(p->footprintpal); F8(p->heat_on);
    F8(p->holster_weapon);
#if DUKE3D_GAMEVER >= GAMEVER_10
    F8(p->falling_counter); F8(p->frag_ps);
#endif
#if DUKE3D_GAMEVER >= GAMEVER_11
    F8(p->aim_mode);
#endif
    A8(p->gotweapon,10); F8(p->refresh_inventory);
    /* palette: a 32-bit DOS pointer. Written as zeros, never read back. */
    if (write) b[o] = b[o+1] = b[o+2] = b[o+3] = 0;
    o += 4;
#if DUKE3D_GAMEVER < GAMEVER_10
    F16(p->rotscrnang); F16(p->dead_flag); F16(p->show_empty_weapon);
    F16(p->scuba_amount); F16(p->jetpack_amount); F16(p->steroids_amount); F16(p->shield_amount);
    F16(p->spacesuit_amount);
    F16(p->holoduke_on); F16(p->pycount); F16(p->weapon_pos);
    display = p == &ps[myconnectindex] ? pus : 1; F16(display);
    if (!write && p == &ps[myconnectindex]) pus = display;
    display = p == &ps[myconnectindex] ? pub : 1; F16(display);
    if (!write && p == &ps[myconnectindex]) pub = display;
    F16(p->transporter_hold); F16(p->last_full_weapon); F16(p->footprintshade); F16(p->boot_amount);

#endif
    F8(p->toggle_key_flag); F8(p->knuckle_incs);
    if (write) b[o] = 0;    /* select_dir */
    o++;
    F8(p->walking_snd_toggle); F8(p->interface_toggle_flag); F8(p->palookup); F8(p->hard_landing);
    F8(p->max_secret_rooms); F8(p->secret_rooms); A8(p->pals,3);
#if DUKE3D_GAMEVER < GAMEVER_10
    display = p == &ps[myconnectindex] ? ud.show_help : 0; F8(display);
    if (!write && p == &ps[myconnectindex]) ud.show_help = display;
#endif
    F8(p->max_actors_killed); F8(p->actors_killed);
#if DUKE3D_GAMEVER < GAMEVER_11
    F8(p->auto_run);
#endif
    F8(p->return_to_center);
    return o;
}
#elif DUKE3D_GAMEVER < GAMEVER_14
/*
 * The DOS 1.3D player_struct (0x552 bytes, docs/v13d/reference.md and ledger
 * F-007): no subweapon or wantweaponfire, aim_mode is a char near the end,
 * 11 weapons, 256 weaprecs and a 13-byte name.
 */
int32_t sg_player(unsigned char *b, void *rec, int write)
{
    struct player_struct *p = (struct player_struct *)rec;
    int32_t o = 0, k;

    if (!write) { p->subweapon = 0; p->wantweaponfire = 0; }

    F32(p->zoom); F32(p->exitx); F32(p->exity);
    A32(p->loogiex,64); A32(p->loogiey,64); F32(p->numloogs); F32(p->loogcnt);
    F32(p->posx); F32(p->posy); F32(p->posz); F32(p->horiz); F32(p->ohoriz);
    F32(p->ohorizoff); F32(p->invdisptime);
    F32(p->bobposx); F32(p->bobposy); F32(p->oposx); F32(p->oposy); F32(p->oposz);
    F32(p->pyoff); F32(p->opyoff);
    F32(p->posxv); F32(p->posyv); F32(p->poszv); F32(p->last_pissed_time);
    F32(p->truefz); F32(p->truecz);
    F32(p->player_par); F32(p->visibility);
    F32(p->bobcounter); F32(p->weapon_sway);
    F32(p->pals_time); F32(p->randomflamex); F32(p->crack_time);

    F16(p->ang); F16(p->oang); F16(p->angvel); F16(p->cursectnum); F16(p->look_ang);
    F16(p->last_extra);
    A16(p->ammo_amount,11); F16(p->wackedbyactor); F16(p->frag); F16(p->fraggedself);
    F16(p->curr_weapon); F16(p->last_weapon); F16(p->tipincs); F16(p->horizoff);
    F16(p->holoduke_amount); F16(p->newowner); F16(p->hurt_delay); F16(p->hbomb_hold_delay);
    F16(p->jumping_counter); F16(p->airleft); F16(p->knee_incs); F16(p->access_incs);
    F16(p->fta); F16(p->ftq); F16(p->access_wallnum); F16(p->access_spritenum);
    F16(p->kickback_pic); F16(p->got_access); F16(p->weapon_ang); F16(p->firstaid_amount);
    F16(p->somethingonplayer); F16(p->on_crane); F16(p->i); F16(p->one_parallax_sectnum);
    F16(p->over_shoulder_on); F16(p->random_club_frame); F16(p->fist_incs);
    F16(p->one_eighty_count); F16(p->cheat_phase);
    F16(p->dummyplayersprite); F16(p->extra_extra8); F16(p->quick_kick);
    F16(p->heat_amount); F16(p->actorsqu); F16(p->timebeforeexit); F16(p->customexitsound);
    A16(p->weaprecs,256); F16(p->weapreccnt); F16(p->interface_toggle_flag);
    F16(p->rotscrnang); F16(p->dead_flag); F16(p->show_empty_weapon);
    F16(p->scuba_amount); F16(p->jetpack_amount); F16(p->steroids_amount); F16(p->shield_amount);
    F16(p->holoduke_on); F16(p->pycount); F16(p->weapon_pos); F16(p->frag_ps);
    F16(p->transporter_hold); F16(p->last_full_weapon); F16(p->footprintshade); F16(p->boot_amount);

    F32(p->scream_voice);

    F8(p->gm); F8(p->on_warping_sector); F8(p->footprintcount);
    F8(p->hbomb_on); F8(p->jumping_toggle); F8(p->rapid_fire_hold); F8(p->on_ground);
    A8(p->name,13); F8(p->inven_icon); F8(p->buttonpalette);
    F8(p->jetpack_on); F8(p->spritebridge); F8(p->lastrandomspot);
    F8(p->scuba_on); F8(p->footprintpal); F8(p->heat_on);
    F8(p->holster_weapon); F8(p->falling_counter); F8(p->aim_mode);
    A8(p->gotweapon,11); F8(p->refresh_inventory);
    /* palette: a 32-bit DOS pointer. Written as zeros, never read back. */
    if (write) b[o] = b[o+1] = b[o+2] = b[o+3] = 0;
    o += 4;
    F8(p->toggle_key_flag); F8(p->knuckle_incs);
    F8(p->walking_snd_toggle); F8(p->palookup); F8(p->hard_landing);
    F8(p->max_secret_rooms); F8(p->secret_rooms); A8(p->pals,3);
    F8(p->max_actors_killed); F8(p->actors_killed); F8(p->return_to_center);
    return o;
}
#else
int32_t sg_player(unsigned char *b, void *rec, int write)
{
    struct player_struct *p = (struct player_struct *)rec;
    int32_t o = 0, k;

    F32(p->zoom); F32(p->exitx); F32(p->exity);
    A32(p->loogiex,64); A32(p->loogiey,64); F32(p->numloogs); F32(p->loogcnt);
    F32(p->posx); F32(p->posy); F32(p->posz); F32(p->horiz); F32(p->ohoriz);
    F32(p->ohorizoff); F32(p->invdisptime);
    F32(p->bobposx); F32(p->bobposy); F32(p->oposx); F32(p->oposy); F32(p->oposz);
    F32(p->pyoff); F32(p->opyoff);
    F32(p->posxv); F32(p->posyv); F32(p->poszv); F32(p->last_pissed_time);
    F32(p->truefz); F32(p->truecz);
    F32(p->player_par); F32(p->visibility);
    F32(p->bobcounter); F32(p->weapon_sway);
    F32(p->pals_time); F32(p->randomflamex); F32(p->crack_time);
    F32(p->aim_mode);

    F16(p->ang); F16(p->oang); F16(p->angvel); F16(p->cursectnum); F16(p->look_ang);
    F16(p->last_extra); F16(p->subweapon);
    A16(p->ammo_amount,MAX_WEAPONS); F16(p->wackedbyactor); F16(p->frag); F16(p->fraggedself);
    F16(p->curr_weapon); F16(p->last_weapon); F16(p->tipincs); F16(p->horizoff);
    F16(p->wantweaponfire);
    F16(p->holoduke_amount); F16(p->newowner); F16(p->hurt_delay); F16(p->hbomb_hold_delay);
    F16(p->jumping_counter); F16(p->airleft); F16(p->knee_incs); F16(p->access_incs);
    F16(p->fta); F16(p->ftq); F16(p->access_wallnum); F16(p->access_spritenum);
    F16(p->kickback_pic); F16(p->got_access); F16(p->weapon_ang); F16(p->firstaid_amount);
    F16(p->somethingonplayer); F16(p->on_crane); F16(p->i); F16(p->one_parallax_sectnum);
    F16(p->over_shoulder_on); F16(p->random_club_frame); F16(p->fist_incs);
    F16(p->one_eighty_count); F16(p->cheat_phase);
    F16(p->dummyplayersprite); F16(p->extra_extra8); F16(p->quick_kick);
    F16(p->heat_amount); F16(p->actorsqu); F16(p->timebeforeexit); F16(p->customexitsound);
    A16(p->weaprecs,16); F16(p->weapreccnt); F16(p->interface_toggle_flag);
    F16(p->rotscrnang); F16(p->dead_flag); F16(p->show_empty_weapon);
    F16(p->scuba_amount); F16(p->jetpack_amount); F16(p->steroids_amount); F16(p->shield_amount);
    F16(p->holoduke_on); F16(p->pycount); F16(p->weapon_pos); F16(p->frag_ps);
    F16(p->transporter_hold); F16(p->last_full_weapon); F16(p->footprintshade); F16(p->boot_amount);

    F32(p->scream_voice);

    F8(p->gm); F8(p->on_warping_sector); F8(p->footprintcount);
    F8(p->hbomb_on); F8(p->jumping_toggle); F8(p->rapid_fire_hold); F8(p->on_ground);
    A8(p->name,32); F8(p->inven_icon); F8(p->buttonpalette);
    F8(p->jetpack_on); F8(p->spritebridge); F8(p->lastrandomspot);
    F8(p->scuba_on); F8(p->footprintpal); F8(p->heat_on);
    F8(p->holster_weapon); F8(p->falling_counter);
    A8(p->gotweapon,MAX_WEAPONS); F8(p->refresh_inventory);
    /* palette: a 32-bit DOS pointer. Written as zeros, never read back. */
    if (write) b[o] = b[o+1] = b[o+2] = b[o+3] = 0;
    o += 4;
    F8(p->toggle_key_flag); F8(p->knuckle_incs);
    F8(p->walking_snd_toggle); F8(p->palookup); F8(p->hard_landing);
    F8(p->max_secret_rooms); F8(p->secret_rooms); A8(p->pals,3);
    F8(p->max_actors_killed); F8(p->actors_killed); F8(p->return_to_center);
    return o;
}
#endif

int32_t sg_playerorig(unsigned char *b, void *rec, int write)
{
    struct player_orig *p = (struct player_orig *)rec;
    int32_t o = 0;

    F32(p->ox); F32(p->oy); F32(p->oz);
    F16(p->oa); F16(p->os);
    return o;
}

int32_t sg_weaponhit(unsigned char *b, void *rec, int write)
{
    struct weaponhit *h = (struct weaponhit *)rec;
    int32_t o = 0, k;

    F8(h->cgg);
    F16(h->picnum); F16(h->ang); F16(h->extra); F16(h->owner); F16(h->movflag);
    F16(h->tempang); F16(h->actorstayput); F16(h->dispicnum);
    F16(h->timetosleep);
    F32(h->floorz); F32(h->ceilingz); F32(h->lastvx); F32(h->lastvy);
    F32(h->bposx); F32(h->bposy); F32(h->bposz);
    A32(h->temp_data,6);
    return o;
}

int32_t sg_animwall(unsigned char *b, void *rec, int write)
{
    struct animwalltype *a = (struct animwalltype *)rec;
    int32_t o = 0;

    F16(a->wallnum); F32(a->tag);
    return o;
}

void sg_write8(FILE *fil, int32_t v) { unsigned char b = (unsigned char)v; dfwrite(&b,1,1,fil); }
void sg_write16(FILE *fil, int32_t v) { unsigned char b[2]; put16(b,v); dfwrite(b,2,1,fil); }
void sg_write32(FILE *fil, int32_t v) { unsigned char b[4]; put32(b,v); dfwrite(b,4,1,fil); }
int32_t sg_read8(int32_t fil) { unsigned char b; kdfread(&b,1,1,fil); return b; }
int32_t sg_read16(int32_t fil) { unsigned char b[2]; kdfread(b,2,1,fil); return get16(b); }
int32_t sg_read32(int32_t fil) { unsigned char b[4]; kdfread(b,4,1,fil); return get32(b); }

void sg_write16s(FILE *fil, short *a, int32_t dasizeof, int32_t count)
{
    int32_t i, n = dasizeof*count/2;

    for (i = 0; i < n; i++) put16(sgbuf+i*2,a[i]);
    dfwrite(sgbuf,dasizeof,count,fil);
}

void sg_read16s(int32_t fil, short *a, int32_t dasizeof, int32_t count)
{
    int32_t i, n = dasizeof*count/2;

    kdfread(sgbuf,dasizeof,count,fil);
    for (i = 0; i < n; i++) a[i] = (short)get16(sgbuf+i*2);
}

void sg_read16sn(int32_t fil, short *a, int32_t n, int32_t dasizeof, int32_t count)
{
    int32_t i, m;

    if (count < 0) count = 0;
    if (count > (int32_t)sizeof(sgbuf)/dasizeof) count = sizeof(sgbuf)/dasizeof;
    m = dasizeof*count/2;
    if (m > n) m = n;

    kdfread(sgbuf,dasizeof,count,fil);
    for (i = 0; i < m; i++) a[i] = (short)get16(sgbuf+i*2);
}

void sg_write32s(FILE *fil, int32_t *a, int32_t dasizeof, int32_t count)
{
    int32_t i, n = dasizeof*count/4;

    for (i = 0; i < n; i++) put32(sgbuf+i*4,a[i]);
    dfwrite(sgbuf,dasizeof,count,fil);
}

void sg_read32s(int32_t fil, int32_t *a, int32_t dasizeof, int32_t count)
{
    int32_t i, n = dasizeof*count/4;

    kdfread(sgbuf,dasizeof,count,fil);
    for (i = 0; i < n; i++) a[i] = get32(sgbuf+i*4);
}

void sg_writerecs(FILE *fil, void *recs, size_t stride, sg_rec fn, int32_t n, int32_t dasizeof, int32_t count)
{
    int32_t i, o = 0;

    for (i = 0; i < n; i++) o += fn(sgbuf+o,(char *)recs+i*stride,1);
    dfwrite(sgbuf,dasizeof,count,fil);
}

void sg_readrecs(int32_t fil, void *recs, size_t stride, sg_rec fn, int32_t n, int32_t dasizeof, int32_t count)
{
    int32_t i, o = 0;

    kdfread(sgbuf,dasizeof,count,fil);
    for (i = 0; i < n; i++) o += fn(sgbuf+o,(char *)recs+i*stride,0);
}

/* Mirrors dfwrite's chunking in build/cache1d.c. */
int32_t sg_dfblocks(int32_t dasizeof, int32_t count)
{
    int32_t i, k, n = 0;

    if (dasizeof > SG_LZWSIZE) { count *= dasizeof; dasizeof = 1; }
    k = dasizeof;
    if (k > SG_LZWSIZE-dasizeof) { n++; k = 0; }
    for (i = 1; i < count; i++)
    {
        k += dasizeof;
        if (k > SG_LZWSIZE-dasizeof) { n++; k = 0; }
    }
    if (k > 0) n++;
    return n;
}

#if DUKE3D_GAMEVER < GAMEVER_11
#define SG_LZWBLOCK (SG_LZWSIZE+(SG_LZWSIZE>>4))   /* the most a block may hold */

const int32_t sg_scriptslots10[3] = { 1, 4, 5 };

/* One 1.0 dfwrite call: each block must be there, inside the file, with the
   uncompressed size dfwrite gave it. With buf, the call is also decoded into
   buf. Leaves fil past the call. Returns 0 if it doesn't fit. */
static int32_t sg_call10(int32_t fil, int32_t dasizeof, int32_t count, void *buf)
{
    int32_t start = klseek(fil,0,SEEK_CUR), size = dasizeof, n = count, i, k = 0, end = start;
    short leng, uncompleng;

    if (size > SG_LZWSIZE) { n *= size; size = 1; }
    for (i = 0; i < n; i++)
    {
        k += size;
        if (k <= SG_LZWSIZE-size && i < n-1) continue;
        if (!kread16(fil,&leng) || leng < 4 || leng > SG_LZWBLOCK) return 0;
        if (!kread16(fil,&uncompleng) || uncompleng != k) return 0;
        end = klseek(fil,leng-2,SEEK_CUR);
        if (end < 0 || end > kfilelength(fil)) return 0;
        k = 0;
    }
    if (buf != NULL)
    {
        klseek(fil,start,SEEK_SET);
        kdfread(buf,dasizeof,count,fil);
        klseek(fil,end,SEEK_SET);
    }
    return 1;
}

static int32_t sg_calls10(int32_t fil, const int32_t (*calls)[2], int32_t n)
{
    int32_t i;

    for (i = 0; i < n; i++)
        if (!sg_call10(fil,calls[i][0],calls[i][1],NULL)) return 0;
    return 1;
}

int32_t sg_check10(int32_t fil)
{
    /* saveplayer's dfwrite calls (1.0 0x388d0), between the decoded ones. */
    static const int32_t head[][2] = { {19,1}, {4,1}, {4,1}, {4,1}, {160,100} };
    static const int32_t lists[][2] =
    {
        {2,MAXSECTORS+1}, {2,MAXSPRITES}, {2,MAXSPRITES},
        {2,MAXSTATUS+1}, {2,MAXSPRITES}, {2,MAXSPRITES},
        {2,1}, {12,MAXCYCLERS},
        {SG_PLAYER_SIZE,SG_DOS_MAXPLAYERS}, {SG_PLAYERORIG_SIZE,SG_DOS_MAXPLAYERS}
    };
    static const int32_t mid[][2] =
    {
        {2,1}, {SG_ANIMWALL_SIZE*MAXANIMWALLS,1}, {4,2048}, {4,2048},
        {2,1}, {2,64}, {2,1}, {2,64}, {2,64},
#if DUKE3D_GAMEVER >= GAMEVER_10
        {1,MAXSECTORS>>3},
#endif
        {1,SG_DOS_MAXSCRIPT}, {4,SG_DOS_MAXSCRIPT}
    };
    static const int32_t tail[][2] =
    {
        {4,1}, {2,1}, {2,MAXPSKYTILES},
        {4,1}, {2,MAXANIMATES}, {4,MAXANIMATES}, {4,MAXANIMATES}, {4,MAXANIMATES},
        {1,1}, {2,1},
#if DUKE3D_GAMEVER < GAMEVER_10
        {2,1}
#else
        {4,1}, {4,1}
#endif
    };
    static short picnum[SG_DOS_MAXHITTYPES];
    static unsigned char actors[SG_DOS_MAXTILES*4];
    unsigned char b[2];
    int32_t start = klseek(fil,0,SEEK_CUR), base = -1, n, i, k, v;

    if (!sg_calls10(fil,head,5)) goto done;
    if (!sg_call10(fil,2,1,b)) goto done;
    n = get16(b);
    if (n < 1 || n > MAXWALLS || !sg_call10(fil,SG_WALL_SIZE,n,NULL)) goto done;
    if (!sg_call10(fil,2,1,b)) goto done;
    n = get16(b);
    if (n < 1 || n > MAXSECTORS || !sg_call10(fil,SG_SECTOR_SIZE,n,NULL)) goto done;
    if (!sg_call10(fil,SG_SPRITE_SIZE,MAXSPRITES,sgbuf)) goto done;
    for (i = 0; i < SG_DOS_MAXHITTYPES; i++) picnum[i] = (short)get16(sgbuf+i*SG_SPRITE_SIZE+14);
    if (!sg_calls10(fil,lists,10)) goto done;
    /* hittype[] comes raw, straight after po[] */
    if (!sg_call10(fil,SG_WEAPONHIT_SIZE,SG_DOS_MAXHITTYPES,sgbuf)) goto done;
    if (!sg_calls10(fil,mid,sizeof(mid)/sizeof(mid[0]))) goto done;
    if (!sg_call10(fil,4,SG_DOS_MAXTILES,actors)) goto done;
    if (!sg_calls10(fil,tail,sizeof(tail)/sizeof(tail[0]))) goto done;
    if (klseek(fil,0,SEEK_CUR) != kfilelength(fil)) goto done;

    /* actorscrptr[]: every entry is stored minus script's address, so the
       empty ones all hold minus that address, and the others are offsets. */
    n = 0;
    for (i = 0; i < SG_DOS_MAXTILES; i++)
    {
        v = get32(actors+i*4);
        if (v < 0)
        {
            if (v < -0x7fff0000 || (n != 0 && v != -n)) goto done;
            n = -v;
        }
        else if (v == 0 || v >= SG_DOS_MAXSCRIPT*4 || (v&3)) goto done;
    }
    if (n == 0) goto done;

    /* hittype[]: an actor's temp_data[1], [4] and [5] at or above that
       address (1.0's own test for a script address) must point into the
       script. */
    for (i = 0; i < SG_DOS_MAXHITTYPES; i++)
    {
        if (picnum[i] < 0 || picnum[i] >= SG_DOS_MAXTILES || get32(actors+picnum[i]*4) < 0) continue;
        for (k = 0; k < 3; k++)
        {
            v = get32(sgbuf+i*SG_WEAPONHIT_SIZE+SG_WEAPONHIT_TEMPDATA+sg_scriptslots10[k]*4);
            if (v >= n && (v-n >= SG_DOS_MAXSCRIPT*4 || ((v-n)&3))) goto done;
        }
    }
    base = n;

done:
    klseek(fil,start,SEEK_SET);
    return base;
}
#endif

int32_t sg_playerblocksize(int32_t fil)
{
    /* The dfwrite calls between the version block and ps[]. */
    static const int32_t calls[][2] =
    {
#if DUKE3D_GAMEVER >= GAMEVER_14
        {4,1},  /* ud.multimode; 1.3D doesn't save it */
#endif
        {19,1}, {4,1}, {4,1}, {4,1}, {160,100},
        {2,1}, {SG_WALL_SIZE,MAXWALLS}, {2,1}, {SG_SECTOR_SIZE,MAXSECTORS},
        {SG_SPRITE_SIZE,MAXSPRITES},
        {2,MAXSECTORS+1}, {2,MAXSPRITES}, {2,MAXSPRITES},
        {2,MAXSTATUS+1}, {2,MAXSPRITES}, {2,MAXSPRITES},
        {2,1}, {12,MAXCYCLERS}
    };
    int32_t start = klseek(fil,0,SEEK_CUR), i, n = 0, size = -1;
    short leng, uncompleng;

    for (i = 0; i < (int32_t)(sizeof(calls)/sizeof(calls[0])); i++)
        n += sg_dfblocks(calls[i][0],calls[i][1]);
    for (i = 0; i < n; i++)
    {
        if (!kread16(fil,&leng)) break;
        klseek(fil,leng,SEEK_CUR);
    }
    if (i == n && kread16(fil,&leng) && kread16(fil,&uncompleng))
        size = (unsigned short)uncompleng;
    klseek(fil,start,SEEK_SET);
    return size;
}
