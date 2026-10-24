#include "profile_test_support.h"
/* Beta spawn differences, measured in docs/v099/notes/0.99-game.md. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"
static int failures;
static void check(const char *name,int got,int want)
{ if(got!=want) { printf("FAIL %s: %d want%d\n",name,got,want); failures++; } }
static int item(int pic)
{
    int i;
    memset(hittype,0,sizeof(hittype)); memset(actorscrptr,0,sizeof(actorscrptr));
    initspritelists(); i=insertsprite(0,0);
    memset(&sprite[i],0,sizeof(sprite[i])); sprite[i].sectnum=sprite[i].statnum=0;
    sprite[i].x=sprite[i].y=512; sprite[i].picnum=pic; sprite[i].extra=-1;
    sprite[i].cstat=8; sprite[i].xvel=7; sprite[i].zvel=9;
    ud.multimode=1; ud.player_skill=2; randomseed=17;
    return i;
}
int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,"0.99")) return 1;
    game_init_release_defaults();
    int i;
    initengine(); numsectors=1; numwalls=4; sector[0].wallptr=0; sector[0].wallnum=4;
    sector[0].ceilingz=-8192; sector[0].floorz=8192;
    wall[0].x=wall[0].y=0; wall[1].x=1024; wall[1].y=0;
    wall[2].x=wall[2].y=1024; wall[3].x=0; wall[3].y=1024;
    for(i=0;i<4;i++) { wall[i].point2=(i+1)&3; wall[i].nextwall=wall[i].nextsector=-1; }
    i=item(WATERDRIP); spawn(-1,i);
    check("map drip no RNG",randomseed,17); check("map drip z",sprite[i].z,0);
    check("map drip velocity",sprite[i].zvel,128);
    i=item(CAMERA1); spawn(-1,i); check("camera cstat",sprite[i].cstat,257);
    i=item(GENERICPOLE); spawn(-1,i); check("pole cstat",sprite[i].cstat,257);
    i=item(OCEANSPRITE4); spawn(-1,i); check("ocean clip",sprite[i].clipdist,32);
    check("ocean cstat",sprite[i].cstat,265);
    i=item(TRIPBOMB); sprite[i].lotag=3; spawn(-1,i);
    check("filtered tripbomb stays stat0",sprite[i].statnum,0);
    i=item(DUKELYINGDEAD); spawn(-1,i); check("dead duke xvel",sprite[i].xvel,7);
    check("dead duke zvel",sprite[i].zvel,9);
    i=item(WATERSPLASH2); spawn(-1,i);
    check("map splash at floor",sprite[i].z,sector[0].floorz);
    i=item(RESPAWN); spawn(-1,i); check("respawn shade",sprite[i].shade,53);
    check("respawn extra kept",sprite[i].extra,-1);
    i=item(FEM4); spawn(-1,i); check("absent FEM4 stat",sprite[i].statnum,0);
    check("absent FEM4 extra",sprite[i].extra,-1);
    i=item(DUKECAR); spawn(-1,i); check("absent DUKECAR stat",sprite[i].statnum,0);
    {
        char *args[] = { "duke3d", "/t2", "/s5" };
        ud.m_respawn_items=0; checkcommandline(3,args);
        check("beta respawns monsters only",ud.m_respawn_monsters,1);
        check("beta does not respawn items",ud.m_respawn_items,0);
        check("beta headache skill",ud.player_skill,4);
    }
    {
        char *args[] = { "duke3d", "-demodump", "/tmp/map-state.bin",
            "-loadverify", "/tmp/save.sav", "-saveverify", "/tmp/out.sav" };
        boardfilename[0]=0; checkcommandline(7,args);
        check("diagnostic paths do not enable respawn",ud.m_respawn_monsters,0);
        check("diagnostic paths do not select a map",boardfilename[0],0);
    }
    {
        char *args[] = { "duke3d", "oddmapname" };
        ud.warp_on=0; boardfilename[0]=0; checkcommandline(2,args);
        check("beta map substring",strcmp(boardfilename,"oddmapname"),0);
        check("beta user map warp",ud.warp_on,1);
        check("beta user map volume",ud.m_volume_number,3);
        check("beta user map level",ud.m_level_number,8);
    }
    i=item(GLASSPIECES); sprite[i].ang=256;
    wall[4].x=2048; wall[4].y=2048; wall[4].point2=5;
    wall[5].x=3072; wall[5].y=2048;
    {
        int seed, z, zv, vel, g;
        randomseed=17; krand(); z=-(32<<8)+(krand()&16383);
        zv=-(krand()&1023); vel=32+(krand()&63); seed=randomseed;
        randomseed=17; lotsofglass(i,4,1); g=headspritestat[5];
        check("outside glass four RNG draws",randomseed,seed);
        check("outside glass spawned",g>=0,1);
        if(g>=0) {
            check("outside glass source sector",sprite[g].sectnum,0);
            check("outside glass fallback z",sprite[g].z,z);
            check("outside glass z velocity",sprite[g].zvel,zv);
            check("outside glass velocity",sprite[g].xvel,vel);
        }
    }
    i=item(APLAYER); ps[0].i=i;
    {
        int32_t x=512,y=512,z=0;
        int obstacle=insertsprite(0,0);
        memset(&sprite[obstacle],0,sizeof(sprite[obstacle]));
        sprite[obstacle].x=256; sprite[obstacle].y=512;
        sprite[obstacle].sectnum=0; sprite[obstacle].statnum=0;
        sprite[obstacle].cstat=1; sprite[obstacle].clipdist=16;
        sprite[obstacle].picnum=APLAYER; sprite[obstacle].yrepeat=32;
        tilesizy[APLAYER]=32;
        short sect=0;
        sintable[0]=0; sintable[512]=16384;
        view(&ps[0],&x,&y,&z,&sect,0,120);
        check("camera sets source collision bits",sprite[i].cstat,265);
        check("camera preserves height",z,0);
        check("camera clipped by wall",x>=0 && x<512,1);
        check("camera sector",sect,0);
        check("camera sets hit sprite collision bits",sprite[obstacle].cstat,257);
    }
    if(failures) return 1;
    puts("beta game: matches measured spawn, CLI, glass and camera cases"); return 0;
}
