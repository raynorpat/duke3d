#include "profile_test_support.h"
/* Beta CON runtime cases from docs/v099/notes/0.99-con-interpreter.md.
 * No game data: run the real interpreter on a square sector. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"

static int failures, actor, player;
static void check(const char *name, int got, int want)
{
    if(got != want) { printf("FAIL %s: %d, want %d\n", name, got, want); failures++; }
}
static void setup(const int32_t *code, int words)
{
    int i;
    static const int x[] = {0,1024,1024,0}, y[] = {0,0,1024,1024};
    memset(sector,0,sizeof(sector)); memset(wall,0,sizeof(wall));
    memset(ps,0,sizeof(ps)); memset(hittype,0,sizeof(hittype));
    memset(script,0,sizeof(script)); memset(actorscrptr,0,sizeof(actorscrptr));
    numsectors=1; numwalls=4; sector[0].wallnum=4;
    sector[0].ceilingz=-8192; sector[0].floorz=8192;
    for(i=0;i<4;i++) {
        wall[i].x=x[i]; wall[i].y=y[i]; wall[i].point2=(i+1)&3;
        wall[i].nextwall=wall[i].nextsector=-1;
    }
    initspritelists(); player=insertsprite(0,10); actor=insertsprite(0,5);
    memset(&sprite[player],0,sizeof(sprite[player]));
    memset(&sprite[actor],0,sizeof(sprite[actor]));
    sprite[player].statnum=10; sprite[actor].statnum=5;
    sprite[player].x=sprite[player].y=512; sprite[player].picnum=APLAYER;
    sprite[player].extra=100; sprite[player].xrepeat=48;
    sprite[actor].x=sprite[actor].y=256; sprite[actor].picnum=BOX;
    sprite[actor].extra=100; sprite[actor].xrepeat=40;
    ps[0].i=player; ps[0].posx=ps[0].posy=512;
    ud.multimode=1; ud.recstat=0; ud.respawn_monsters=ud.respawn_items=0;
    actorscrptr[BOX]=16; memcpy(&script[8],code,words*sizeof(*code));
    totalclock=100; numcyclers=0; gc=176;
}
int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,"0.99")) return 1;
    game_init_release_defaults();
    static const int32_t pal[]={14,12,34,56,4,4};
    static const int32_t alive[]={51,16384,56,11,77,4,11,88,4};
    static const int32_t respawn[]={71,52,11,77,4,11,88,4};
    static const int32_t gap[]={62,100,56,11,77,4,11,88,4};
    static const int32_t distance[]={9,1000,56,11,77,4,11,88,4};
    static const int32_t reset[]={42,4}, activate[]={69,777,4}, fall[]={16,4};
    setup(pal,6); execute(actor,0,0);
    check("palfrom expiry",ps[0].pals_time,132);
    check("palfrom red",ps[0].pals[0],12); check("palfrom green",ps[0].pals[1],34);
    check("palfrom blue",ps[0].pals[2],56);
    setup(alive,9); sprite[player].xrepeat=16; execute(actor,0,0);
    check("shrunk player is alive",sprite[actor].extra,77);
    setup(respawn,8); ud.respawn_monsters=1; execute(actor,0,0);
    check("item uses monster respawn",sprite[actor].extra,77);
    setup(respawn,8); ud.respawn_monsters=2; ud.respawn_items=1; execute(actor,0,0);
    check("respawn requires exactly one",sprite[actor].extra,88);
    setup(gap,9); hittype[actor].floorz=100000; execute(actor,0,0);
    check("gap refreshes heights",sprite[actor].extra,77);
    setup(distance,9); execute(actor,0,9000);
    check("sleep beyond8192",hittype[actor].timetosleep,384);
    setup(reset,2); ud.recstat=2; lastsavedpos=0; execute(actor,0,0);
    check("resetplayer offers last save in demo",ps[0].gm,MODE_MENU);
    setup(activate,3); ps[0].timebeforeexit=123; execute(actor,0,0);
    check("activate does not schedule exit",ps[0].timebeforeexit,123);
    setup(fall,2); sector[0].lotag=2; hittype[actor].floorz=8192;
    hittype[actor].cgg=1; execute(actor,0,0);
    check("underwater fall acceleration",sprite[actor].zvel,176);
    /* C099-025/026: flags64 exists in beta, and z easing has no648 gate. */
    { static const int32_t end[]={4};
      setup(end,1); sprite[actor].x=sprite[actor].y=512;
      sprite[actor].hitag=64; sprite[actor].ang=512; ps[0].posxv=262144;
      execute(actor,0,2000);
      check("smart facing leads by velocity>>9",sprite[actor].ang,384);
      hittype[actor].temp_data[1]=256; script[64]=0; script[65]=100;
      sprite[actor].zvel=1000; alterang(0);
      check("vertical easing above648",sprite[actor].zvel,1120);
    }
    if(failures) return 1;
    puts("beta CON runtime: matches EXE cases"); return 0;
}
