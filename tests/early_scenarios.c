#include "profile_test_support.h"
/* Literal DOS cases: docs/v10-v11/notes/1.1-player.md (0x4517a,
 * 0x45bdc, 0x449a5, 0x4625c), 1.1-actors.md (0x25634), and ledger
 * G10-L14 (0x43c52) / G11-A35 (SE24 0x2c92f). No native goldens. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"

static int failures;
static void check(const char *name, int got, int want)
{
    if(got != want) { printf("FAIL %s: %d, want %d\n",name,got,want); failures++; }
}
static int actor(int tile, int stat, int sect, int x)
{
    int i=insertsprite(sect,stat);
    memset(&sprite[i],0,sizeof(sprite[i]));
    sprite[i].sectnum=sect; sprite[i].statnum=stat; sprite[i].picnum=tile;
    sprite[i].x=x; sprite[i].y=16384; sprite[i].z=32768;
    sprite[i].xrepeat=48; sprite[i].yrepeat=40; sprite[i].extra=100;
    sprite[i].owner=i; hittype[i].floorz=32768; hittype[i].ceilingz=-32768;
    return i;
}
static struct player_struct *setup(void)
{
    static const int x[]={0,32768,32768,0},y[]={0,0,32768,32768};
    int i,pi;
    memset(sector,0,sizeof(sector)); memset(wall,0,sizeof(wall));
    memset(ps,0,sizeof(ps)); memset(hittype,0,sizeof(hittype));
    memset(sync,0,sizeof(sync)); memset(&ud,0,sizeof(ud));
    numsectors=1; numwalls=4; sector[0].wallnum=4;
    sector[0].ceilingz=-32768; sector[0].floorz=32768;
    for(i=0;i<4;i++) {
        wall[i].x=x[i]; wall[i].y=y[i]; wall[i].point2=(i+1)&3;
        wall[i].nextwall=wall[i].nextsector=-1;
    }
    initspritelists(); pi=actor(APLAYER,10,0,16384);
    ps[0].i=pi; ps[0].posx=ps[0].posy=16384;
    ps[0].oposx=ps[0].oposy=16384; ps[0].posz=32768-PHEIGHT;
    ps[0].last_extra=100; ps[0].on_ground=1; ps[0].horiz=100;
    ps[0].newowner=ps[0].on_crane=ps[0].holoduke_on=-1;
    ps[0].actorsqu=ps[0].dummyplayersprite=ps[0].last_weapon=-1;
    ps[0].airleft=390; ps[0].crack_time=777;
    ud.multimode=1; ud.player_skill=2; numplayers=1;
    connecthead=myconnectindex=screenpeek=0; connectpoint2[0]=-1;
    totalclock=100; earthquaketime=0; max_player_health=100;
    randomseed=17; dukefriction=0xd000; restorepalette=0;
    memset(sintable,0,sizeof(sintable)); sintable[512]=16384; sintable[1536]=-16384;
    return &ps[0];
}
int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    game_init_release_defaults();
    struct player_struct *p;
    int i,j,k;
    initengine();
    p=setup(); sync[0].avel=-64; doincrements(p);
if (game_profile->gamever < GAMEVER_11) {
    check("G10-L14 negative turn",p->weapon_ang,12);
    sync[0].avel=64; doincrements(p); check("G10-L14 positive turn",p->weapon_ang,-12);
} else {
    check("1.1 weapon angle stays zero",p->weapon_ang,0);
}
    p=setup(); p->burn_time=2; processinput(0);
    check("burn countdown",p->burn_time,1); check("burn damage fraction",p->extra_extra8,48);
    check("burn draws one random",randomseed,468938558);
    p=setup(); p->burn_time=1; processinput(0);
    check("burn expires",p->burn_time,0); check("burn restores palette",restorepalette,1);
    p=setup(); p->burn_time=50; sector[0].lotag=2; p->scuba_on=1; p->scuba_amount=100;
    processinput(0); check("water extinguishes fire",p->burn_time,0);
    p=setup(); sector[0].lotag=2; i=actor(THROWFLAME,4,0,12000);
    sprite[i].owner=p->i; moveweapons();
    check("water extinguishes flame projectile",sprite[i].statnum,MAXSTATUS);
    p=setup(); sprite[p->i].cstat=257; sprite[p->i].clipdist=32; tilesizy[APLAYER]=40;
    j=actor(LIZMAN,1,0,5000); i=actor(THROWFLAME,4,0,15000);
    sprite[i].owner=j; sprite[i].z=30000; sprite[i].xvel=4096;
    moveweapons(); check("flame hit ignites player",p->burn_time,120);
    p=setup(); sector[0].ceilingstat=1; sector[0].ceilingpicnum=MOONSKY1;
    p->spacesuit_amount=2; moveplayers();
    check("space equips suit",p->spacesuit_on,1); check("space leaves player alive",sprite[p->i].extra,100);
    doincrements(p); check("suit countdown",p->spacesuit_amount,1);
    doincrements(p); check("suit exhausted",p->spacesuit_on,0);
    check("suit marked used",p->spacesuit_used,1);
    moveplayers(); check("space hurts after exhaustion",p->extra_extra8,32);
    p=setup(); sector[0].ceilingstat=1; sector[0].ceilingpicnum=MOONSKY1;
    moveplayers(); check("space without suit kills",sprite[p->i].extra,0);
    p=setup(); p->spacesuit_on=1; p->spacesuit_amount=20; processinput(0);
    check("leaving space removes suit",p->spacesuit_on,0);
    p=setup(); sector[0].ceilingstat=1; sector[0].ceilingpicnum=MOONSKY1;
    p->on_ground=0; p->posz=10000; processinput(0);
    check("space gravity",p->poszv,42);
    p=setup(); sector[0].ceilingstat=1; sector[0].ceilingpicnum=MOONSKY1;
    p->jumping_counter=1; sintable[81]=6400; processinput(0);
    check("space jump advances eighty",p->jumping_counter,81);
    check("space jump impulse",p->poszv,-100);
    /* Crossing a conveyor's sector carries the sprite walk into its new list. */
    p=setup(); numsectors=2; numwalls=8; sector[1]=sector[0]; sector[1].wallptr=4;
    for(i=0;i<4;i++) { wall[i+4]=wall[i]; wall[i+4].x+=32768; wall[i+4].point2+=4; }
    i=actor(SECTOREFFECTOR,3,0,16384); sprite[i].lotag=24; sprite[i].yvel=512;
    j=actor(NUKEBARREL,1,1,40000); k=actor(NUKEBARREL,1,0,32765);
    moveeffectors();
    check("G11-A35 barrel crosses sector",sprite[k].sectnum,1);
    check("G11-A35 new sector barrel follows",sprite[j].x,40016);
    if(failures) return 1;
    puts("early scenarios: matches DOS cases"); return 0;
}
