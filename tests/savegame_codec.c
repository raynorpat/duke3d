/*
 * Checks the save record codec against byte offsets worked out by hand
 * from the DOS 1.5 layout: struct fields packed to 1 byte, little-endian.
 */
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

#if DUKE3D_GAMEVER < GAMEVER_14
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

int main(void)
{
    test_wall();
    test_sector();
    test_sprite();
    test_player();
    test_small();
    test_dfblocks();
    if (failures)
        printf("%d check(s) failed\n", failures);
    else
        puts("savegame_codec passed");
    return failures ? 1 : 0;
}
