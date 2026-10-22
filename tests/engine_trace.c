/*
 * Checks cansee(), hitscan() and clipmove() against each engine revision on
 *  a tiny map: two 1024-unit squares side by side, sector 0 on the left and
 *  sector 1 on the right, joined by a red wall. The 1.0 engine (before
 *  19960220) differs from the later ones in four ways (ledger E10-001 to
 *  E10-004); each case below shows one of them.
 */
#include <stdio.h>
#include <stdint.h>
#include "build/build.h"
#include "build/engine_protos.h"

#define CLIPMASK0 (((1L)<<16)+1L)

static int failures;

/* The engine calls these into the game. */
void timerhandler(void) { }
void keyhandler(void) { }
void faketimerhandler(void) { }

static void check(const char *what, int got, int want)
{
    if (got != want) {
        printf("FAIL %s: got %d, want %d\n", what, got, want);
        failures++;
    }
}

/* Fills sector s with the square [x0,x0+1024]x[0,1024] from wall w, clockwise. */
static void square(int s, int w, int32_t x0)
{
    static const int32_t dx[4] = { 0, 1024, 1024, 0 }, dy[4] = { 0, 0, 1024, 1024 };
    int i;

    for (i = 0; i < 4; i++) {
        wall[w+i].x = x0+dx[i]; wall[w+i].y = dy[i];
        wall[w+i].point2 = w + ((i+1) & 3);
        wall[w+i].nextsector = -1; wall[w+i].nextwall = -1; wall[w+i].cstat = 0;
    }
    sector[s].wallptr = w; sector[s].wallnum = 4;
    sector[s].ceilingz = -8192; sector[s].floorz = 8192;
    sector[s].ceilingstat = 0; sector[s].floorstat = 0;
}

int main(void)
{
    short hitsect, hitwall, hitsprite, sect;
    int32_t hitx, hity, hitz, x, y, z;
    int i, r;

    initengine(); // initialize the 1996 square-root tables for the slope cases
    square(0, 0, 0);
    square(1, 4, 1024);
    /* Wall 1 (sector 0's east side) and wall 7 (sector 1's west side) meet. */
    wall[1].nextsector = 1; wall[1].nextwall = 7;
    wall[7].nextsector = 0; wall[7].nextwall = 1;
    numsectors = 2;
    initspritelists();

    /* E10-001: looking in from a point on a one-sided wall's line. 1.0 lets
     *  that wall through at t = 0 and stops there; later engines skip it. */
#if BUILD_ENGINEREV < 19960220L
    check("cansee from a wall line", cansee(0, 512, 0, 0, 512, 512, 0, 0), 0);
#else
    check("cansee from a wall line", cansee(0, 512, 0, 0, 512, 512, 0, 0), 1);
#endif
    check("cansee across the red wall", cansee(512, 512, 0, 0, 1536, 512, 0, 1), 1);

    /* E10-002: 1.0 ignores cliptype (always CLIPMASK1, walls cstat&64), so a
     *  cstat&1 red wall doesn't stop the CLIPMASK0 ray. */
    wall[1].cstat = wall[7].cstat = 1;
    hitscan(512, 512, 0, 0, 1024, 0, 0, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, CLIPMASK0);
#if BUILD_ENGINEREV < 19960220L
    check("hitscan cstat 1 wall: sector", hitsect, 1);
    check("hitscan cstat 1 wall: wall", hitwall, 5);
#else
    check("hitscan cstat 1 wall: sector", hitsect, 0);
    check("hitscan cstat 1 wall: wall", hitwall, 1);
#endif
    wall[1].cstat = wall[7].cstat = 0;

    /* E10-002: a ray that hits nothing. 1.0 resets only *hitsect. */
    hitwall = 123; hitsprite = 456;
    hitscan(512, 512, 0, 0, 0, 0, 0, &hitsect, &hitwall, &hitsprite, &hitx, &hity, &hitz, CLIPMASK0);
    check("hitscan no hit: sector", hitsect, -1);
#if BUILD_ENGINEREV < 19960220L
    check("hitscan no hit: wall kept", hitwall, 123);
    check("hitscan no hit: sprite kept", hitsprite, 456);
#else
    check("hitscan no hit: wall reset", hitwall, -1);
    check("hitscan no hit: sprite reset", hitsprite, -1);
#endif

    /* E10-004: a sprite of sector 0 whose clip line lies on sector 1's east
     *  wall's clip line (x = 1920), right on the path. 1.0 adds every wall
     *  before any sprite, so raytrace meets the sprite first and keeps it on
     *  the tie; later engines add sector 1's walls last and keep the wall.
     *  clipmove returns the short clipobjectval. */
    i = insertsprite(0, 0);
    sprite[i].x = 1920+256; sprite[i].y = 512; sprite[i].z = 0;
    sprite[i].cstat = 1; sprite[i].picnum = 0; sprite[i].clipdist = 32;
    sprite[i].xrepeat = sprite[i].yrepeat = 64;
    tilesizx[0] = tilesizy[0] = 64; picanm[0] = 0;
    x = 900; y = 512; z = -256; sect = 0;
    r = clipmove(&x, &y, &z, &sect, 1100<<14, 0, 128, 4<<8, 4<<8, CLIPMASK0);
#if BUILD_ENGINEREV < 19960220L
    check("clipmove tie: sprite first", r, (short)(49152+i));
#else
    check("clipmove tie: wall first", r, (short)(32768+5));
#endif
    check("clipmove tie: stopped before x 1920", x < 1920, 1);

    /* E10-003: a bad sector. 1.0 moves to the goal with no clipping and
     *  finds the sector afterwards; later engines refuse the move. */
    x = 300; y = 512; z = 0; sect = -1;
    r = clipmove(&x, &y, &z, &sect, 200<<14, 0, 128, 4<<8, 4<<8, CLIPMASK0);
    check("clipmove bad sector: return", r, 0);
#if BUILD_ENGINEREV < 19960220L
    check("clipmove bad sector: x", x, 500);
    check("clipmove bad sector: sector", sect, 0);
#else
    check("clipmove bad sector: x", x, 300);
    check("clipmove bad sector: sector", sect, -1);
#endif

    /* E099-001/002: measured by running beta hitscan at0x5c1c1 on this
     * square in x86 emulation (docs/v099/reference.md). */
    square(0,0,0); numsectors=1; initspritelists();
    sector[0].ceilingz=-32768; sector[0].floorstat=2; sector[0].floorheinum=1024;
    hitscan(512,512,0,0,100,0,-8192,&hitsect,&hitwall,&hitsprite,&hitx,&hity,&hitz,CLIPMASK0);
    check("sloped floor backwards: sector",hitsect,0);
#if BUILD_ENGINEREV < 19960129L
    check("sloped floor backwards: x",hitx,387);
    check("sloped floor backwards: z",hitz,10240);
#else
    check("sloped floor backwards: forward wall",hitwall,1);
    check("sloped floor backwards: forward x",hitx>512,1);
#endif
    sector[0].ceilingz=-8192; sector[0].floorz=32768;
    sector[0].ceilingstat=2; sector[0].floorstat=0; sector[0].ceilingheinum=1024;
    hitscan(512,512,0,0,100,0,8192,&hitsect,&hitwall,&hitsprite,&hitx,&hity,&hitz,CLIPMASK0);
#if BUILD_ENGINEREV < 19960129L
    check("sloped ceiling backwards: x",hitx,437);
    check("sloped ceiling backwards: z",hitz,-6144);
#else
    check("sloped ceiling backwards: forward wall",hitwall,1);
    check("sloped ceiling backwards: forward x",hitx>512,1);
#endif
    for(i=0;i<3;i++) {
        sector[0].ceilingstat=(i==1)?2:0; sector[0].floorstat=(i==2)?2:0;
        sector[0].ceilingheinum=sector[0].floorheinum=-1;
        sector[0].ceilingz=(i==0)?-511:((i==1)?0:-32768);
        sector[0].floorz=(i==2)?0:8192;
        hitscan(512,511,0,0,0,0,(i==0)?-256:-1,&hitsect,&hitwall,&hitsprite,&hitx,&hity,&hitz,CLIPMASK0);
#if BUILD_ENGINEREV < 19960129L
        check("negative odd height boundary rejected",hitsect,-1);
#else
        check("negative odd height boundary accepted",hitsect,0);
#endif
    }

    if (failures) return 1;
    printf("engine trace: ok\n");
    return 0;
}
