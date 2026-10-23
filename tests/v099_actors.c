#include "profile_test_support.h"
/* Literal actor expectations from docs/v099/notes/0.99-actors.md.
 * Calls the real actor/engine functions; no game data is needed. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"

static int failures, player;
static void check(const char *name, int got, int want)
{
    if(got != want) { printf("FAIL %s: %d, want %d\n",name,got,want); failures++; }
}
static int actor(int tile, int stat)
{
    int i=insertsprite(0,stat);
    memset(&sprite[i],0,sizeof(sprite[i]));
    sprite[i].statnum=stat; sprite[i].picnum=tile;
    sprite[i].x=sprite[i].y=512; sprite[i].xrepeat=sprite[i].yrepeat=48;
    sprite[i].extra=100; sprite[i].owner=i;
    hittype[i].actorstayput=-1; hittype[i].extra=-1;
    hittype[i].ceilingz=-8192; hittype[i].floorz=8192;
    return i;
}
static void setup(void)
{
    static const int x[]={0,4096,4096,0},y[]={0,0,4096,4096};
    int i;
    memset(sector,0,sizeof(sector)); memset(wall,0,sizeof(wall));
    memset(ps,0,sizeof(ps)); memset(hittype,0,sizeof(hittype));
    memset(sintable,0,sizeof(sintable));
    memset(script,0,sizeof(script)); memset(actorscrptr,0,sizeof(actorscrptr));
    numsectors=1; numwalls=4; sector[0].wallnum=4;
    sector[0].ceilingz=-8192; sector[0].floorz=8192;
    for(i=0;i<4;i++) {
        wall[i].x=x[i]; wall[i].y=y[i]; wall[i].point2=(i+1)&3;
        wall[i].nextwall=wall[i].nextsector=-1;
    }
    initspritelists(); player=actor(APLAYER,10); sprite[player].owner=player;
    ps[0].i=player; ps[0].posx=ps[0].posy=512;
    ps[0].oposx=ps[0].oposy=512; ps[0].oposz=-(28<<8);
    ps[0].newowner=-1; ps[0].holoduke_on=-1;
    connecthead=0; connectpoint2[0]=-1; myconnectindex=screenpeek=0;
    ud.multimode=1; ud.god=ud.clipping=ud.recstat=0;
    ud.respawn_monsters=ud.respawn_items=0; ud.monsters_off=0;
    fricxv=123; fricyv=456; myx=512; myy=512;
    totalclock=100; gc=176; randomseed=17; sintable[512]=16384;
}
/* A disconnected second sector makes visibility and warp destinations explicit. */
static void secondsector(void)
{
    int j;
    numsectors=2; numwalls=8; sector[1]=sector[0]; sector[1].wallptr=4;
    for(j=0;j<4;j++) {
        wall[j+4]=wall[j]; wall[j+4].x+=8192; wall[j+4].point2+=4;
    }
}
static int transporter(int floor)
{
    int dest=actor(SECTOREFFECTOR,9), src=actor(SECTOREFFECTOR,9);
    changespritesect(dest,1); sprite[dest].x=8704; sprite[dest].y=512;
    sprite[src].owner=dest; sprite[dest].owner=src;
    hittype[src].temp_data[4]=hittype[dest].temp_data[4]=floor;
    hittype[dest].temp_data[0]=100;
    return src;
}
static void movingsectorflags(void)
{
    int i,j,pivot;

    setup(); i=actor(SECTOREFFECTOR,3); sprite[i].lotag=0;
    pivot=actor(SECTOREFFECTOR,3); sprite[pivot].lotag=1; sprite[pivot].owner=pivot;
    hittype[pivot].temp_data[0]=1; sprite[i].owner=pivot;
    sector[0].floorstat=64; sector[0].extra=4096;
    ps[0].on_ground=1; ps[0].posx=1024;
    for(j=0;j<4;j++) { msx[j]=wall[j].x-512; msy[j]=wall[j].y-512; }
    moveeffectors();
    check("SE0 marks carried player",ps[0].on_sector_effector,1);
    ps[0].on_sector_effector=0; ps[0].on_ground=0; moveeffectors();
    check("SE0 ignores airborne player",ps[0].on_sector_effector,0);

    setup(); i=actor(SECTOREFFECTOR,3); sprite[i].lotag=14; sprite[i].xvel=32;
    j=actor(LOCATORS,7); sprite[j].x=16384; sprite[i].owner=j;
    for(j=0;j<4;j++) { msx[j]=wall[j].x-512; msy[j]=wall[j].y-512; }
    moveeffectors();
    check("SE14 marks carried player",ps[0].on_sector_effector,1);
    ps[0].on_sector_effector=0; sprite[i].extra=0; moveeffectors();
    check("SE14 extra zero does not carry",ps[0].on_sector_effector,0);

    setup(); i=actor(SECTOREFFECTOR,3); sprite[i].lotag=30; sprite[i].xvel=16;
    j=actor(LOCATORS,7); sprite[j].x=16384; sprite[i].owner=j;
    for(j=0;j<4;j++) { msx[j]=wall[j].x-512; msy[j]=wall[j].y-512; }
    moveeffectors();
    check("SE30 marks carried player",ps[0].on_sector_effector,1);
    ps[0].on_sector_effector=0; sprite[i].xvel=0; moveeffectors();
    check("stationary SE30 does not carry",ps[0].on_sector_effector,0);
}

int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    game_init_release_defaults();
    int i, j, k;
    movingsectorflags();
    setup(); ps[0].curr_weapon=RPG_WEAPON;
    ps[0].gotweapon[DEVISTATOR_WEAPON]=1; ps[0].ammo_amount[DEVISTATOR_WEAPON]=10;
    checkavailweapon(&ps[0]); check("registered shoot scan",ps[0].curr_weapon,DEVISTATOR_WEAPON);
    setup(); ps[0].curr_weapon=game_remote_weapon();
    ps[0].gotweapon[TRIPBOMB_WEAPON]=1; ps[0].ammo_amount[TRIPBOMB_WEAPON]=10;
    checkavailweapon(&ps[0]); check("downward bomb scan",ps[0].curr_weapon,TRIPBOMB_WEAPON);
    setup(); hittype[player].extra=150; hittype[player].picnum=RPG;
    hittype[player].owner=-1; ud.god=1;
    check("god does not skip hit",ifhitbyweapon(player),RPG);
    check("damage clamps health",sprite[player].extra,0);
    check("explosion push uses two",ps[0].posxv,4915200);
    setup(); sector[0].floorz=0; sector[0].ceilingz=-1024; ud.clipping=1;
    check("clip cheat does not bypass squish",ifsquished(player,0),1);
    setup(); i=actor(WATERDRIP,6); sprite[i].zvel=0; sprite[i].z=4096;
    hittype[i].temp_data[0]=1024; hittype[i].temp_data[1]=20;
    movestandables(); check("landed drip resets z",sprite[i].z,1024);
    check("landed drip consumes one random",randomseed,468938558);
    movestandables(); check("waiting drip consumes another random",randomseed,86936935);
    setup(); i=actor(MONEY,5); sprite[i].zvel=80; sector[0].lotag=2; hittype[i].temp_data[0]=600;
    moveexplosions(); check("underwater money accelerates to144",sprite[i].zvel,87);
    setup(); i=actor(SHELL,5); sprite[i].zvel=0; sprite[i].xvel=20;
    hittype[i].temp_data[0]=2; hittype[i].temp_data[1]=0;
    moveexplosions(); check("shell frame counter not ticked",hittype[i].temp_data[1],0);
    setup(); lotsofmoney(&sprite[player],1);
    check("money spawn uses two randoms",randomseed,86936935);
    check("money has no random cstat",sprite[headspritestat[5]].cstat,0);
    setup(); i=actor(RESPAWN,11); sprite[i].shade=65; sprite[i].extra=-1;
    sprite[i].hitag=BOX; movefx();
    check("respawn shade ticks",sprite[i].shade,66);
    check("respawn extra retained",sprite[i].extra,-1);
    movefx(); check("respawn fires on shade66",sprite[i].statnum,MAXSTATUS);
    setup(); i=actor(MUSICANDSFX,11); sprite[i].lotag=1;
    AmbienceToggle=1; soundm[1]=16; hittype[i].temp_data[4]=8;
    movefx(); check("random ambience countdown absent",hittype[i].temp_data[4],8);
    setup(); i=actor(WATERFOUNTAIN,6); sprite[i].owner=player;
    hittype[i].temp_data[0]=20; movestandables();
    check("fountain heals owner per cycle",sprite[player].extra,101);
    setup(); i=actor(BOX,6); hittype[i].temp_data[0]=1; hittype[i].temp_data[1]=3;
    movestandables(); check("flammable x shrinks one",sprite[i].xrepeat,47);
    check("flammable y shrinks zero",sprite[i].yrepeat,48);
    check("flammable consumes two randoms",randomseed,86936935);
    setup(); i=actor(TRASH,6); movestandables();
    check("trash consumes no random",randomseed,17);
    check("trash has no movement",sprite[i].xvel,0);
    setup(); i=actor(BOLT1,6); randomseed=2; movestandables();
    check("bolt skips short circuit random",randomseed,55169243);
    setup(); i=actor(TRIPBOMB,6); hittype[i].temp_data[0]=33;
    hittype[i].lastvx=3590; movestandables();
    check("tripbomb quantized beam unchanged",hittype[i].temp_data[2],0);
    check("tripbomb trace leaves x",sprite[i].x,512);
    check("tripbomb trace leaves z",sprite[i].z,0);
    hittype[i].lastvx=3400; movestandables();
    check("tripbomb quantized beam triggers",hittype[i].temp_data[2],13);
    setup(); i=actor(LIZTROOP,1); hittype[i].actorstayput=1;
    sector[0].lotag=1; sprite[i].zvel=0;
    movesprite(i,100,0,0,CLIPMASK0);
    check("stayput bounce needs zvel1024",sprite[i].x,562);
    sprite[i].zvel=1024; movesprite(i,100,0,0,CLIPMASK0);
    check("stayput bounce restores x",sprite[i].x,562);
    check("stayput bounce consumes no random",randomseed,17);
    sprite[i].picnum=OCTABRAIN; movesprite(i,100,0,0,CLIPMASK0);
    check("octabrain exempt from stayput bounce",sprite[i].x,612);
    setup(); i=actor(BOSS1,1); sprite[i].x=sprite[i].y=2048;
    movesprite(i,-3600,0,0,CLIPMASK0);
    check("boss clip width512",sprite[i].x>=512 && sprite[i].x<1024,1);
    check("wall collision does not turn actor",sprite[i].ang,0);
    setup(); i=actor(APLAYER,10); sprite[i].owner=-1;
    sprite[i].xrepeat=44; sprite[i].yrepeat=40; sprite[i].extra=9;
    ps[0].holoduke_on=i; moveplayers();
    check("holoduke grows to48",sprite[i].xrepeat,48);
    check("holoduke grows to44",sprite[i].yrepeat,44);
    setup(); hittype[player].owner=77; ud.god=1; sprite[player].cstat=0;
    moveplayers(); check("alive player keeps damage owner",hittype[player].owner,77);
    check("god keeps player cstat",sprite[player].cstat,0);
    setup(); i=actor(BOX,0); sprite[i].x=612; sprite[player].extra=0;
    hittype[player].owner=i; ps[0].wackedbyactor=-1; ps[0].ang=2100;
    moveplayers(); check("dead player follows hitter without angle mask",ps[0].ang,2074);
    setup(); secondsector(); i=actor(SHRINKSPARK,4); sprite[i].extra=5;
    j=actor(LIZTROOP,0); changespritesect(j,1); sprite[j].x=8704; sprite[j].cstat=257; hittype[j].extra=0;
    hitradius(i,20000,1,2,3,4);
    check("radius stat0 badguy ignores visibility",hittype[j].extra,5);
    setup(); i=actor(RPG,4); j=actor(QUEBALL,1);
    hitradius(i,1000,1,2,3,4); check("unblocked balls not radius targets",hittype[j].extra,-1);
    setup(); i=actor(RPG,4); j=actor(ROTATEGUN,1); sprite[j].cstat=257;
    hitradius(i,1000,1,2,3,4); check("radius pushes rotategun",sprite[j].xvel,400);
    setup(); secondsector(); i=transporter(1); k=sprite[i].owner;
    ps[0].on_ground=0; movetransports();
    check("floor teleport needs no on_ground",ps[0].posx,8704);
    check("floor teleport source hold26",hittype[i].temp_data[0],26);
    check("floor teleport destination hold26 ticks",hittype[k].temp_data[0],25);
    check("floor teleport player hold2",ps[0].transporter_hold,2);
    check("floor teleport updates current sector",ps[0].cursectnum,1);
    check("floor teleport retains sprite sector",sprite[player].sectnum,0);
    setup(); secondsector(); i=transporter(1); ps[0].transporter_hold=1;
    j=actor(BOX,1); movetransports();
    check("stationary sprite never warps",sprite[j].sectnum,0);
    sprite[j].zvel=128; movetransports();
    check("occupied floor warp changes sprite sector",sprite[j].sectnum,1);
    check("occupied floor warp keeps x",sprite[j].x,512);
    setup(); secondsector(); i=transporter(1); ps[0].transporter_hold=1;
    sector[0].lotag=2; sector[1].lotag=1;
    j=actor(PLAYERONWATER,13); sprite[j].z=-8192; sprite[j].zvel=128;
    sprite[j].cstat=32767; movetransports();
    check("dummy transport preserves statnum",sprite[j].statnum,13);
    check("dummy transport preserves statlist",headspritestat[13],j);
    check("dummy transport preserves cstat",sprite[j].cstat,32767);
    setup(); i=actor(SECTOREFFECTOR,3); sprite[i].lotag=24; sprite[i].yvel=512;
    ps[0].on_ground=1; ps[0].posz=8192-PHEIGHT; moveeffectors();
    check("conveyor moves player directly",ps[0].posx,528);
    check("conveyor copies opos",ps[0].oposx,528);
    check("conveyor moves prediction",myx,528);
    check("conveyor leaves friction",fricxv,123);
    check("conveyor leaves sprite x",sprite[player].x,512);
    setup(); i=actor(SECTOREFFECTOR,3); sprite[i].lotag=26; sprite[i].zvel=128;
    for(j=0;j<4;j++) { msx[j]=wall[j].x-512; msy[j]=wall[j].y-512; }
    ps[0].on_ground=1; moveeffectors();
    check("SE26 accelerates player directly",ps[0].posxv,1048576);
    check("SE26 moves player vertically",ps[0].posz,128);
    check("SE26 leaves friction",fricxv,123);
    setup(); i=actor(SECTOREFFECTOR,3); sprite[i].lotag=29; sprite[i].hitag=2020;
    moveeffectors(); check("SE29 stores masked phase",sprite[i].hitag,36);
    setup(); i=actor(SECTOREFFECTOR,3); sprite[i].lotag=31; sprite[i].ang=1536;
    sprite[i].yvel=16; sprite[i].z=8200; hittype[i].temp_data[0]=1;
    j=actor(BOX,0); hittype[j].floorz=1234; moveeffectors();
    check("SE31 arrival skips sprite floor cache",hittype[j].floorz,1234);
    setup(); i=actor(RESPAWNMARKERRED,1); hittype[i].temp_data[0]=3;
    moveactors(); check("respawn marker runs no hardcoded timer",hittype[i].temp_data[0],3);
    setup(); i=actor(DUKECAR,1); sprite[i].zvel=128; moveactors();
    check("car runs no hardcoded motion",sprite[i].z,0);
    setup(); i=actor(JIBS1,5); sprite[i].zvel=100; sector[0].lotag=2;
    hittype[i].temp_data[1]=2; moveexplosions();
    check("underwater gib adds128 then halves",sprite[i].zvel,114);
    check("underwater gib moves by halved velocity",sprite[i].z,114);
    check("underwater gib advances frames",hittype[i].temp_data[0],1);
    setup(); i=actor(JIBS1,5); sector[0].ceilingz=sector[0].floorz;
    moveexplosions(); check("gib not killed by equal ceiling floor",sprite[i].statnum,5);
    setup(); i=actor(INNERJAW,5); moveexplosions();
    check("jaw flash stores expiry",ps[0].pals_time,132);
    setup(); i=actor(RADIUSEXPLOSION,5); actorscrptr[RADIUSEXPLOSION]=16;
    script[8]=11; script[9]=77; script[10]=4;
    moveexplosions(); check("radius explosion runs CON",sprite[i].extra,77);
    if(failures) return 1;
    puts("beta actors: matches EXE cases"); return 0;
}
