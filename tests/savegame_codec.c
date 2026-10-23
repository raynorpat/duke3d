#include "profile_test_support.h"
/*
 * Checks the save record codec against byte offsets worked out by hand
 * from the DOS 1.5 layout: struct fields packed to 1 byte, little-endian.
 */
#define SDL_MAIN_HANDLED  /* game.c's main() is renamed; this one is ours. */
#include "duke3d.h"
#include "savegame.h"

static int failures;

#define CHECK(c) do { if (!(c)) { printf("%s:%d: %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)

static int32_t le16(const unsigned char *b) { return (short)(b[0] | (b[1] << 8)); }
static int32_t le32(const unsigned char *b)
{
    return (int32_t)((uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24));
}

static void test_wall(void)
{
    static const unsigned char x[4] = { 0x44, 0x33, 0x22, 0x11 };
    walltype w, back;
    unsigned char b[64];

    memset(&w, 0, sizeof(w)); memset(&back, 0, sizeof(back)); memset(b, 0xee, sizeof(b));
    w.x = 0x11223344; w.y = -2; w.point2 = 0x0102; w.cstat = -3; w.shade = -4;
    w.ypanning = 0xab; w.extra = 0x7f01;
    CHECK(sg_wall(b, &w, 1) == 32);
    CHECK(memcmp(b, x, 4) == 0);
    CHECK(le32(b + 4) == -2);
    CHECK(b[8] == 0x02 && b[9] == 0x01);
    CHECK(le16(b + 14) == -3);
    CHECK(b[20] == 0xfc);
    CHECK(b[25] == 0xab);
    CHECK(le16(b + 30) == 0x7f01);
    CHECK(b[32] == 0xee);
    CHECK(sg_wall(b, &back, 0) == 32);
    CHECK(memcmp(&w, &back, sizeof(w)) == 0);
}

static void test_sector(void)
{
    sectortype s, back;
    unsigned char b[64];

    memset(&s, 0, sizeof(s)); memset(&back, 0, sizeof(back)); memset(b, 0xee, sizeof(b));
    s.wallptr = 0x0201; s.ceilingz = -0x10000; s.ceilingshade = -1; s.floorpal = 0x29;
    s.filler = 0x33; s.extra = -6;
    CHECK(sg_sector(b, &s, 1) == 40);
    CHECK(b[0] == 0x01 && b[1] == 0x02);
    CHECK(le32(b + 4) == -0x10000);
    CHECK(b[20] == 0xff);
    CHECK(b[29] == 0x29);
    CHECK(b[33] == 0x33);
    CHECK(le16(b + 38) == -6);
    CHECK(b[40] == 0xee);
    CHECK(sg_sector(b, &back, 0) == 40);
    CHECK(memcmp(&s, &back, sizeof(s)) == 0);
}

static void test_sprite(void)
{
    spritetype s, back;
    unsigned char b[64];

    memset(&s, 0, sizeof(s)); memset(&back, 0, sizeof(back)); memset(b, 0xee, sizeof(b));
    s.z = 0x01020304; s.shade = -1; s.xoffset = -2; s.sectnum = 300; s.extra = 0x0102;
    CHECK(sg_sprite(b, &s, 1) == 44);
    CHECK(b[8] == 0x04 && b[11] == 0x01);
    CHECK(b[16] == 0xff);
    CHECK(b[22] == 0xfe);
    CHECK(le16(b + 24) == 300);
    CHECK(b[42] == 0x02 && b[43] == 0x01);
    CHECK(b[44] == 0xee);
    CHECK(sg_sprite(b, &back, 0) == 44);
    CHECK(memcmp(&s, &back, sizeof(s)) == 0);
}

#if DUKE3D_GAMEVER < GAMEVER_10
/* Independently measured DOS beta record offsets (0.99-player.md). */
static void test_player(void)
{
    static struct player_struct p, back;
    unsigned char b[2048];

    memset(&p, 0, sizeof(p)); memset(&back, 0x55, sizeof(back)); memset(b, 0xee, sizeof(b));
    p.posx = -5; p.posxv = 0x12345678; p.pals_time = 999; p.ang = 0x0605;
    p.ammo_amount[9] = 27; p.curr_weapon = 7; p.i = 0x0a0b; p.customexitsound = 15;
    p.gm = 0x11; p.on_sector_effector = 1; p.name[0] = 'D'; p.unnamed_322 = 29; p.gotweapon[9] = 1;
    p.refresh_inventory = 28; p.rotscrnang = 16; p.shield_amount = 100;
    p.transporter_hold = 19; p.boot_amount = 20; p.toggle_key_flag = 34;
    p.pals[2] = 39; p.actors_killed = 40; p.auto_run = 1; p.return_to_center = 51;
    CHECK(SG_PLAYER_SIZE == 0x33f);
    CHECK(sg_player(b, &p, 1) == 0x33f);
    CHECK(le32(b + 0x214) == -5 && le32(b + 0x240) == 0x12345678);
    CHECK(le32(b + 0x260) == 999 && le16(b + 0x270) == 0x0605);
    CHECK(le16(b + 0x28e) == 27 && le16(b + 0x290) == 7);
    CHECK(le16(b + 0x2bc) == 0x0a0b && le16(b + 0x2da) == 15);
    CHECK(b[0x2e1] == 1);
    CHECK(b[0x2dc] == 0x11 && b[0x2e4] == 'D' && b[0x2f2] == 29);
    CHECK(b[0x306] == 1 && b[0x307] == 28 && le32(b + 0x308) == 0);
    CHECK(le16(b + 0x30c) == 16 && le16(b + 0x318) == 100);
    CHECK(le16(b + 0x326) == 19 && le16(b + 0x32c) == 20);
    CHECK(b[0x32e] == 34 && b[0x339] == 39 && b[0x33c] == 40);
    CHECK(b[0x33d] == 1 && b[0x33e] == 51 && b[0x33f] == 0xee);
    b[0x308] = 0x99; /* Never restore a DOS palette pointer. */
    CHECK(sg_player(b, &back, 0) == 0x33f);
    CHECK(back.posx == -5 && back.shield_amount == 100 && back.actors_killed == 40);
    CHECK(back.on_sector_effector == 1);
    CHECK(back.palette == NULL && back.name[13] == 0 && back.weaprecs[0] == 0);
    CHECK(back.opyoff == 0 && back.truefz == 0 && back.truecz == 0);
    CHECK(back.wackedbyactor == -1 && back.scream_voice == -1);
    CHECK(back.falling_counter == 0 && back.frag_ps == 0 && back.aim_mode == 0);
    CHECK(back.ammo_amount[10] == 0 && back.gotweapon[11] == 0);

    myconnectindex = 0; pus = 3; pub = 4; ud.show_help = 2;
    CHECK(sg_player(b, &ps[0], 1) == 0x33f);
    CHECK(le16(b + 0x322) == 3 && le16(b + 0x324) == 4 && b[0x33a] == 2);
    pus = pub = ud.show_help = 0;
    sg_player(b, &ps[0], 0);
    CHECK(pus == 3 && pub == 4 && ud.show_help == 2);
    sg_player(b, &ps[7], 1);
    CHECK(le16(b + 0x322) == 1 && le16(b + 0x324) == 1 && b[0x33a] == 0);
    sg_player(b, &ps[7], 0);
    CHECK(pus == 3 && pub == 4 && ud.show_help == 2);
}
#elif DUKE3D_GAMEVER < GAMEVER_13D
/* The DOS 1.1 player_struct (docs/v10-v11/reference.md, "player_struct (1.1)"),
   or in 1.0 builds the 1.0 one ("player_struct (1.0)"): the same up to 0x321. */
static void test_player(void)
{
    static struct player_struct p, back;
    static unsigned char b[2048];

    memset(&p, 0, sizeof(p)); memset(&back, 0, sizeof(back)); memset(b, 0xee, sizeof(b));
    p.zoom = 0x01020304; p.loogiey[63] = 0x44; p.posx = -5; p.oposx = 0x0708;
    p.pals_time = 0x0909; p.burn_time = 120; p.crack_time = 777; p.ang = 0x0605;
    p.ammo_amount[9] = 0x0a; p.wackedbyactor = 0x0b; p.curr_weapon = 7; p.i = 0x0a0b;
    p.fist_incs = 0x0c; p.frag = 0x0d; p.fraggedself = 0x0e; p.customexitsound = 0x0f;
    p.rotscrnang = 0x10; p.shield_amount = 100; p.spacesuit_amount = 1600;
    p.holoduke_on = 0x12; p.transporter_hold = 0x13; p.boot_amount = 0x14;
    p.scream_voice = 0x0c0d0e0f; p.gm = 0x11; p.jumping_toggle = 0x15; p.rapid_fire_hold = 0x16;
    p.name[0] = 'D'; p.inven_icon = 0x17; p.lastrandomspot = 0x18; p.spacesuit_on = 1;
    p.scuba_on = 0x19; p.spacesuit_used = 1; p.footprintpal = 0x1a; p.falling_counter = 0x1b;
#if DUKE3D_GAMEVER < GAMEVER_11
    p.frag_ps = 3; p.unnamed_322 = 0x1d; p.auto_run = 1; p.gotweapon[0] = 1; p.gotweapon[9] = 1;
#else
    p.frag_ps = 3; p.aim_mode = 1; p.gotweapon[0] = 1; p.gotweapon[9] = 1;
#endif
    p.refresh_inventory = 0x1c; p.palette = (char *)b; p.toggle_key_flag = 0x22;
    p.knuckle_incs = 0x23; p.walking_snd_toggle = 0x24; p.interface_toggle_flag = 0x25;
    p.palookup = 0x26; p.pals[2] = 0x27; p.actors_killed = 0x28; p.return_to_center = 0x33;
    CHECK(sg_player(b, &p, 1) == 0x34e);
    CHECK(b[0] == 0x04 && b[1] == 0x03 && b[2] == 0x02 && b[3] == 0x01);
    CHECK(le32(b + 520) == 0x44);
    CHECK(le32(b + 0x214) == -5);
    CHECK(le32(b + 0x230) == 0x0708);
    CHECK(le32(b + 0x26c) == 0x0909);
    CHECK(le32(b + 0x270) == 120);
    CHECK(le32(b + 0x278) == 777);
    CHECK(le16(b + 0x27c) == 0x0605);
    CHECK(le16(b + 0x29a) == 0x0a);
    CHECK(le16(b + 0x29c) == 0x0b);
    CHECK(le16(b + 0x29e) == 7);
    CHECK(le16(b + 0x2ca) == 0x0a0b);
    CHECK(le16(b + 0x2d2) == 0x0c);
    CHECK(le16(b + 0x2d4) == 0x0d);
    CHECK(le16(b + 0x2d6) == 0x0e);
    CHECK(le16(b + 0x2e8) == 0x0f);
    CHECK(le16(b + 0x2ea) == 0x10);
    CHECK(le16(b + 0x2f6) == 100);
    CHECK(le16(b + 0x2f8) == 1600);
    CHECK(le16(b + 0x2fa) == 0x12);
    CHECK(le16(b + 0x300) == 0x13);
    CHECK(le16(b + 0x306) == 0x14);
    CHECK(le32(b + 0x308) == 0x0c0d0e0f);
    CHECK(b[0x30c] == 0x11);
    CHECK(b[0x310] == 0x15);
    CHECK(b[0x311] == 0);       /* on_sector_effector: not carried */
    CHECK(b[0x312] == 0x16);
    CHECK(b[0x314] == 'D');
    CHECK(b[0x321] == 0x17);
#if DUKE3D_GAMEVER < GAMEVER_11
    CHECK(b[0x322] == 0x1d);
    CHECK(b[0x326] == 0x18);
#else
    CHECK(b[0x325] == 0x18);
    CHECK(b[0x326] == 0);       /* no_autoaim: never set */
#endif
    CHECK(b[0x327] == 1);
    CHECK(b[0x328] == 0x19);
    CHECK(b[0x329] == 1);
    CHECK(b[0x32a] == 0x1a);
    CHECK(b[0x32d] == 0x1b);
    CHECK(b[0x32e] == 3);
#if DUKE3D_GAMEVER < GAMEVER_11
    /* No aim_mode: gotweapon and everything after it sit one byte lower, up
       to the 1.0-only auto_run at 0x34c. */
    CHECK(b[0x32f] == 1 && b[0x338] == 1);
    CHECK(b[0x339] == 0x1c);
    CHECK(b[0x33a] == 0 && b[0x33b] == 0 && b[0x33c] == 0 && b[0x33d] == 0);
    CHECK(b[0x33e] == 0x22);
    CHECK(b[0x33f] == 0x23);
    CHECK(b[0x340] == 0);       /* select_dir: not carried */
    CHECK(b[0x341] == 0x24);
    CHECK(b[0x342] == 0x25);
    CHECK(b[0x343] == 0x26);
    CHECK(b[0x349] == 0x27);
    CHECK(b[0x34b] == 0x28);
    CHECK(b[0x34c] == 1);
    CHECK(b[0x34d] == 0x33);
    CHECK(b[0x34e] == 0xee);
    b[0x33a] = 0x99;   /* a DOS pointer: must not be restored */
#else
    CHECK(b[0x32f] == 1);
    CHECK(b[0x330] == 1 && b[0x339] == 1);
    CHECK(b[0x33a] == 0x1c);
    CHECK(b[0x33b] == 0 && b[0x33c] == 0 && b[0x33d] == 0 && b[0x33e] == 0);
    CHECK(b[0x33f] == 0x22);
    CHECK(b[0x340] == 0x23);
    CHECK(b[0x341] == 0);       /* select_dir: not carried */
    CHECK(b[0x342] == 0x24);
    CHECK(b[0x343] == 0x25);
    CHECK(b[0x344] == 0x26);
    CHECK(b[0x34a] == 0x27);
    CHECK(b[0x34c] == 0x28);
    CHECK(b[0x34d] == 0x33);
    CHECK(b[0x34e] == 0xee);
    b[0x33b] = 0x99;   /* a DOS pointer: must not be restored */
#endif
    CHECK(sg_player(b, &back, 0) == 0x34e);
    CHECK(back.palette == NULL);
    back.palette = p.palette;
    CHECK(memcmp(&p, &back, sizeof(p)) == 0);
}
#elif DUKE3D_GAMEVER < GAMEVER_14
/* The DOS 1.3D player_struct (docs/v13d/reference.md, ledger F-007). */
static void test_player(void)
{
    static struct player_struct p, back;
    static unsigned char b[2048];

    memset(&p, 0, sizeof(p)); memset(&back, 0, sizeof(back)); memset(b, 0xee, sizeof(b));
    p.zoom = 0x01020304; p.loogiey[63] = 0x44; p.posx = -5; p.ang = 0x0605;
    p.curr_weapon = 7; p.i = 0x0a0b; p.shield_amount = 100; p.scream_voice = 0x0c0d0e0f;
    p.gm = 0x11; p.name[0] = 'D'; p.aim_mode = 1; p.gotweapon[10] = 1; p.palette = (char *)b;
    p.toggle_key_flag = 0x22; p.return_to_center = 0x33;
    CHECK(sg_player(b, &p, 1) == 0x552);
    CHECK(b[0] == 0x04 && b[1] == 0x03 && b[2] == 0x02 && b[3] == 0x01);
    CHECK(le32(b + 520) == 0x44);
    CHECK(le32(b + 0x214) == -5);
    CHECK(le16(b + 0x280) == 0x0605);
    CHECK(le16(b + 0x2a8) == 7);
    CHECK(le16(b + 0x2d4) == 0x0a0b);
    CHECK(le16(b + 0x500) == 100);
    CHECK(le32(b + 0x512) == 0x0c0d0e0f);
    CHECK(b[0x516] == 0x11);
    CHECK(b[0x51d] == 'D');
    CHECK(b[0x534] == 1);
    CHECK(b[0x53f] == 1);
    CHECK(b[0x541] == 0 && b[0x542] == 0 && b[0x543] == 0 && b[0x544] == 0);
    CHECK(b[0x545] == 0x22);
    CHECK(b[0x551] == 0x33);
    CHECK(b[0x552] == 0xee);
    b[0x541] = 0x99;   /* a DOS pointer: must not be restored */
    CHECK(sg_player(b, &back, 0) == 0x552);
    CHECK(back.palette == NULL);
    back.palette = p.palette;
    CHECK(memcmp(&p, &back, sizeof(p)) == 0);
}
#else
static void test_player(void)
{
    static struct player_struct p, back;
    static unsigned char b[1024];

    memset(&p, 0, sizeof(p)); memset(&back, 0, sizeof(back)); memset(b, 0xee, sizeof(b));
    p.zoom = 0x01020304; p.loogiey[63] = 0x44; p.posx = -5; p.ang = 0x0605;
    p.curr_weapon = 7; p.i = 0x0a0b; p.shield_amount = 100; p.scream_voice = 0x0c0d0e0f;
    p.gm = 0x11; p.name[0] = 'D'; p.gotweapon[11] = 1; p.palette = (char *)b;
    p.toggle_key_flag = 0x22; p.return_to_center = 0x33;
    CHECK(sg_player(b, &p, 1) == 911);
    CHECK(b[0] == 0x04 && b[1] == 0x03 && b[2] == 0x02 && b[3] == 0x01);
    CHECK(le32(b + 520) == 0x44);
    CHECK(le32(b + 532) == -5);
    CHECK(le16(b + 644) == 0x0605);
    CHECK(le16(b + 688) == 7);
    CHECK(le16(b + 734) == 0x0a0b);
    CHECK(le16(b + 810) == 100);
    CHECK(le32(b + 828) == 0x0c0d0e0f);
    CHECK(b[832] == 0x11);
    CHECK(b[839] == 'D');
    CHECK(b[892] == 1);
    CHECK(b[894] == 0 && b[895] == 0 && b[896] == 0 && b[897] == 0);
    CHECK(b[898] == 0x22);
    CHECK(b[910] == 0x33);
    CHECK(b[911] == 0xee);
    b[894] = 0x99;   /* a DOS pointer: must not be restored */
    CHECK(sg_player(b, &back, 0) == 911);
    CHECK(back.palette == NULL);
    back.palette = p.palette;
    CHECK(memcmp(&p, &back, sizeof(p)) == 0);
}
#endif

static void test_small(void)
{
    struct player_orig o, oback;
    struct weaponhit h, hback;
    struct animwalltype a, aback;
    unsigned char b[128];

    memset(&o, 0, sizeof(o)); memset(&oback, 0, sizeof(oback)); memset(b, 0xee, sizeof(b));
    o.oz = -1; o.oa = 0x0102; o.os = -2;
    CHECK(sg_playerorig(b, &o, 1) == 16);
    CHECK(le32(b + 8) == -1 && le16(b + 12) == 0x0102 && le16(b + 14) == -2 && b[16] == 0xee);
    CHECK(sg_playerorig(b, &oback, 0) == 16);
    CHECK(memcmp(&o, &oback, sizeof(o)) == 0);

    memset(&h, 0, sizeof(h)); memset(&hback, 0, sizeof(hback)); memset(b, 0xee, sizeof(b));
    h.cgg = 0x81; h.picnum = 0x0102; h.timetosleep = -1; h.floorz = 0x0a0b0c0d;
    h.bposz = -3; h.temp_data[1] = 0x1234; h.temp_data[5] = -4;
    CHECK(sg_weaponhit(b, &h, 1) == 71);
    CHECK(b[0] == 0x81 && b[1] == 0x02 && b[2] == 0x01);
    CHECK(le16(b + 17) == -1);
    CHECK(le32(b + 19) == 0x0a0b0c0d);
    CHECK(le32(b + 43) == -3);
    CHECK(le32(b + 51) == 0x1234);
    CHECK(le32(b + 67) == -4);
    CHECK(b[71] == 0xee);
    CHECK(sg_weaponhit(b, &hback, 0) == 71);
    CHECK(memcmp(&h, &hback, sizeof(h)) == 0);

    memset(&a, 0, sizeof(a)); memset(&aback, 0, sizeof(aback)); memset(b, 0xee, sizeof(b));
    a.wallnum = 0x0102; a.tag = -7;
    CHECK(sg_animwall(b, &a, 1) == 6);
    CHECK(b[0] == 0x02 && b[1] == 0x01 && le32(b + 2) == -7 && b[6] == 0xee);
    CHECK(sg_animwall(b, &aback, 0) == 6);
    CHECK(memcmp(&a, &aback, sizeof(a)) == 0);
}

static void test_dfblocks(void)
{
    /* Block counts of real DOS 1.5 save fields. */
    CHECK(sg_dfblocks(4, 1) == 1);
    CHECK(sg_dfblocks(160, 100) == 1);
    CHECK(sg_dfblocks(32, 8192) == 16);
    CHECK(sg_dfblocks(40, 1024) == 3);
    CHECK(sg_dfblocks(44, 4096) == 12);
    CHECK(sg_dfblocks(2, 1025) == 1);
    CHECK(sg_dfblocks(911 * 16, 1) == 1);
    CHECK(sg_dfblocks(1, 20460) == 2);
    CHECK(sg_dfblocks(4, 20460) == 5);
    CHECK(sg_dfblocks(71, 4096) == 18);
}

static void test_animation_offsets(void)
{
    int32_t off = (int32_t)((intptr_t)&wall[MAXWALLS-1].y-(intptr_t)sector);

    CHECK(sg_animptr(offsetof(sectortype,ceilingz),0) == &sector[0].ceilingz);
    CHECK(sg_animptr((MAXSECTORS-1)*sizeof(sectortype)+offsetof(sectortype,floorz),0) == &sector[MAXSECTORS-1].floorz);
    CHECK(sg_animptr(off,0) == &wall[MAXWALLS-1].y);
#if DUKE3D_GAMEVER < GAMEVER_14
    /* Measured DOS wall base minus sector base, not native linker placement. */
    CHECK(sg_animptr(-0x4b300,1) == &wall[0].x);
    CHECK(sg_animptr(-0x4b300+17*32+4,1) == &wall[17].y);
    CHECK(sg_animptr(-0x4b300+(MAXWALLS-1)*32+4,1) == &wall[MAXWALLS-1].y);
    CHECK(sg_animptr(-0x4b300-32,1) == NULL);
    CHECK(sg_animptr(-0x4b300+8,1) == NULL);
#elif DUKE3D_GAMEVER >= GAMEVER_15
    CHECK(sg_animptr(-0x4b800,1) == &wall[0].x);
    CHECK(sg_animptr(-0x4b800+17*32+4,1) == &wall[17].y);
    CHECK(sg_animptr(-0x4b800+(MAXWALLS-1)*32+4,1) == &wall[MAXWALLS-1].y);
    CHECK(sg_animptr(-0x4b800-32,1) == NULL);
    CHECK(sg_animptr(-0x4b800+8,1) == NULL);
#endif
    CHECK(sg_animptr(0,0) == NULL);
    CHECK(sg_animptr(1,0) == NULL);
    CHECK(sg_animptr(offsetof(sectortype,ceilingz)+1,0) == NULL);
    CHECK(sg_animptr(INT32_MAX,0) == NULL);
#if DUKE3D_GAMEVER < GAMEVER_14 || DUKE3D_GAMEVER >= GAMEVER_15
#if DUKE3D_GAMEVER < GAMEVER_14
    off = -0x4b300;
#else
    off = -0x4b800;
#endif
    /* Re-encoding wall[17].y natively can overlap a different DOS field.
       Supply the native bases explicitly, independently of linker placement. */
    CHECK(sg_walloffset(17*32+4,0,0) == 17*32+4); /* disjoint layouts */
    CHECK(sg_walloffset(off+17*32+4,off,0) == 17*32+4); /* same field */
    CHECK(sg_walloffset(-0x40000+17*32+4,-0x40000,0) == -1); /* different fields */
    CHECK(sg_walloffset(off+64,off+32,0) == -1); /* different valid walls */
    CHECK(sg_walloffset(off+64,off+32,1) == 64);  /* positive DOS evidence */
    CHECK(sg_walloffset(off+64,off,0) == 64);    /* identical interpretations */
    CHECK(sg_walloffset(off+64,0,0) == 64);      /* only DOS fits */
    CHECK(sg_walloffset(64,0,0) == 64);          /* only native fits */
    CHECK(sg_walloffset(64,0,1) == -1);         /* DOS cannot use native fallback */
#endif
}

int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    game_init_release_defaults();
    if (argc > 1)
    {
        static unsigned char cache[4<<20];
        int32_t fil, ok;

        initcache((intptr_t)cache,sizeof(cache));
        fil = kopen4load(argv[1],0);
        if (fil < 0) return 1;
#if DUKE3D_GAMEVER < GAMEVER_11
        ok = sg_check10(fil) >= 0;
#else
        sg_read32(fil); /* The normal loader checks the edition separately. */
        ok = sg_checksave(fil);
#endif
        kclose(fil);
        return ok ? 0 : 2;
    }
    test_wall();
    test_sector();
    test_sprite();
    test_player();
    test_small();
    test_dfblocks();
    test_animation_offsets();
    if (failures)
        printf("%d check(s) failed\n", failures);
    else
        puts("savegame_codec passed");
    return failures ? 1 : 0;
}
