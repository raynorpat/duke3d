/* Literal beta player cases from the EXE comparison in 0.99-player.md. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"

static int failures;
static void check(const char *name, int got, int want)
{
    if(got != want) { printf("FAIL %s: got %d, want %d\n",name,got,want); failures++; }
}
static struct player_struct *setup(void)
{
    int i, pi;
    static const int x[]={0,32768,32768,0}, y[]={0,0,32768,32768};
    memset(sector,0,sizeof(sector)); memset(wall,0,sizeof(wall));
    memset(ps,0,sizeof(ps)); memset(hittype,0,sizeof(hittype));
    memset(sync,0,sizeof(sync)); memset(&ud,0,sizeof(ud));
    numsectors=1; numwalls=4; sector[0].wallnum=4;
    sector[0].ceilingz=-32768; sector[0].floorz=32768;
    for(i=0;i<4;i++) {
        wall[i].x=x[i]; wall[i].y=y[i]; wall[i].point2=(i+1)&3;
        wall[i].nextwall=wall[i].nextsector=-1;
    }
    initspritelists(); pi=insertsprite(0,10);
    memset(&sprite[pi],0,sizeof(sprite[pi])); sprite[pi].statnum=10;
    sprite[pi].picnum=APLAYER; sprite[pi].extra=100;
    sprite[pi].xrepeat=48; sprite[pi].yrepeat=40;
    sprite[pi].x=sprite[pi].y=16384; sprite[pi].z=32768;
    ps[0].i=pi; ps[0].posx=ps[0].posy=16384;
    ps[0].oposx=ps[0].oposy=16384; ps[0].posz=32768-PHEIGHT;
    ps[0].last_extra=100; ps[0].on_ground=1; ps[0].horiz=100;
    ps[0].newowner=ps[0].on_crane=ps[0].holoduke_on=-1;
    ps[0].actorsqu=ps[0].dummyplayersprite=ps[0].last_weapon=-1;
    ps[0].airleft=390; ps[0].crack_time=777;
    ud.multimode=1; ud.player_skill=2; ud.fta_on=1; numplayers=1;
    connecthead=myconnectindex=screenpeek=0; connectpoint2[0]=-1;
    totalclock=100; earthquaketime=0; max_player_health=100;
    randomseed=17; dukefriction=0xd000;
    memset(sintable,0,sizeof(sintable)); sintable[512]=16384; sintable[1536]=-16384;
    return &ps[0];
}
static void button(int n)
{
    if(n<32) CONTROL_ButtonState1 |= 1u<<n;
    else CONTROL_ButtonState2 |= 1u<<(n-32);
}
int main(void)
{
    struct player_struct *p;
    initengine();
    p=setup(); forceplayerangle(p); check("force angle center",p->return_to_center,8);
    p=setup(); p->last_pissed_time=1509; p->holster_weapon=1; p->weapon_pos=-9;
    doincrements(p); check("toilet unholster",p->holster_weapon,0);
    check("toilet weapon position",p->weapon_pos,10);
    p=setup(); p->steroids_amount=1; p->firstaid_amount=50; p->inven_icon=2;
    doincrements(p); check("expiry clears inventory selection",p->inven_icon,0);
    p=setup(); p->knee_incs=1; doincrements(p);
    check("knee in increments",p->knee_incs,2); check("knee horizon",p->horiz,68);
    check("knee center",p->return_to_center,8);
    p=setup(); p->fist_incs=27; doincrements(p);
    check("fist advances in increments",p->fist_incs,28);
    check("fist flash expiry",p->pals_time,132);
    p=setup(); p->fist_incs=42; ud.level_number=5;
    check("fist exit returns",doincrements(p),1); check("fist level six exit",ud.level_number,6);
    p=setup(); p->pals_time=132; p->horiz=102; p->horizoff=12;
    processinput(0); check("flash clock retained",p->pals_time,132);
    check("flat horizon offset decay",p->horizoff,9); check("horizon no snap",p->horiz,102);
    p=setup(); sync[0].bits=1<<3; processinput(0); check("aim up",p->horiz,112);
    p=setup(); sync[0].bits=(1<<4)|(1<<5); processinput(0); check("running aim down",p->horiz,68);
    p=setup(); sync[0].bits=1<<13; processinput(0);
    check("look up",p->horiz,116); check("look center",p->return_to_center,8);
    p=setup(); p->rotscrnang=40; sync[0].bits=1<<6; processinput(0);
    check("look left",p->look_ang,-196); check("roll decay",p->rotscrnang,54);
    p=setup(); p->ang=256; p->on_ground=1; earthquaketime=15; processinput(0);
    check("earthquake angle",p->ang,252);
    p=setup(); sector[0].lotag=2; p->scuba_on=1; p->scuba_amount=100;
    sintable[32]=6400; processinput(0); check("underwater bob",p->pyoff,100);
    check("underwater scuba draws no random",randomseed,17);
    p=setup(); p->jetpack_on=11; p->jetpack_amount=100; sync[0].bits=2;
    processinput(0); check("jetpack descend and floor margin",p->posz,20992);
    p=setup(); p->holster_weapon=1; p->weapon_pos=-5; sync[0].bits=4;
    processinput(0); check("unholster without minus nine",p->holster_weapon,0);
    check("unholster position",p->weapon_pos,10);
    p=setup(); sector[0].lotag=2; tracers(16384,16384,0,16444,16384,0,5);
    { int i,n=0; for(i=headspritestat[5];i>=0;i=nextspritestat[i]) if(sprite[i].picnum==WATERBUBBLE)n++;
      check("short underwater tracer",n,5); }
    check("underwater tracer draws fifteen randoms",randomseed,1628720384);
    p=setup(); sintable[512]=16384; sintable[1536]=-16384;
    fricxv=123; fricyv=456; gamequit=1; ud.pause_on=1;
    CONTROL_ButtonState1=CONTROL_ButtonState2=0;
    button(gamefunc_Move_Forward); getinput(0);
    check("paused recording forward",loc.fvel,1280);
    check("recording no conveyor",loc.svel,0);
    check("recording no quit bit",(loc.bits>>26)&1,0);
    p=setup(); p->curr_weapon=SHRINKER_WEAPON; p->horiz=90;
    shoot(p->i,SHRINKER);
    check("unaimed shrinker slope",sprite[headspritestat[4]].zvel,1000);
    p=setup();
    { int a=insertsprite(0,1), j;
      memset(&sprite[a],0,sizeof(sprite[a])); sprite[a].sectnum=0; sprite[a].statnum=1;
      sprite[a].x=14384; sprite[a].y=16384; sprite[a].z=32768;
      sprite[a].picnum=LIZTROOP; sprite[a].extra=100;
      shoot(a,MORTER); j=headspritestat[1];
      check("mortar offset velocity",sprite[j].xvel,381);
      check("mortar offset z velocity",sprite[j].zvel,-3048);
    }
    p=setup(); p->newowner=1; p->horizoff=12; p->ang=512;
    sector[0].floorstat=2; sector[0].floorheinum=1024;
    processinput(0); check("slope easing groups shifts",p->horizoff,5);
    p=setup(); p->newowner=1; p->horizoff=12; p->ang=512; p->posy=32760;
    sector[0].floorstat=2; sector[0].floorheinum=1024;
    processinput(0); check("probe outside sector retains offset",p->horizoff,12);
    p=setup(); p->posz=-32000; p->on_ground=0; p->poszv=-512;
    processinput(0); check("jump ceiling margin",p->posz,-29696);
    p=setup(); sector[0].lotag=2; p->posz=-32000;
    processinput(0); check("underwater ceiling margin",p->posz,-31744);
    p=setup(); p->on_ground=0; p->posz=20000; p->poszv=5000;
    processinput(0); check("fall damage",sprite[p->i].extra,87);
    check("fall flash expiry",p->pals_time,132);
    p=setup(); sprite[p->i].extra=0; p->last_extra=0;
    p->pals_time=132; p->scream_voice=123; ud.recstat=1; processinput(0);
    check("death counts every death",p->fraggedself,1);
    check("death flag signed formula",p->dead_flag,307);
    check("death retains flash",p->pals_time,132);
    check("death leaves recording open",ud.recstat,1);
    check("death has no scream slot",p->scream_voice,123);
    p=setup(); p->cheat_phase=1; sync[0].bits=(1<<3)|3; processinput(0);
    check("cheat preserves aim bits",p->horiz,112);
    p=setup(); p->horiz=120; p->horizoff=12; p->curr_weapon=HANDBOMB_WEAPON;
    p->kickback_pic=11; p->ammo_amount[HANDBOMB_WEAPON]=10;
    processinput(0);
    { int a; for(a=headspritestat[1];a>=0;a=nextspritestat[a]) if(sprite[a].picnum==HEAVYHBOMB) break;
      check("bomb exists",a>=0,1); if(a>=0) check("bomb subtracts slope offset",sprite[a].zvel,-732); }
    p=setup(); p->curr_weapon=CHAINGUN_WEAPON; p->kickback_pic=1;
    p->ammo_amount[CHAINGUN_WEAPON]=10; sync[0].bits=4; processinput(0);
    check("chaingun fires at two",p->ammo_amount[CHAINGUN_WEAPON],9);
    { int a,n=0; for(a=headspritestat[5];a>=0;a=nextspritestat[a]) if(sprite[a].picnum==SHELL)n++;
      check("chaingun has no shell",n,0); }
    p=setup(); quickkill(p); check("quickkill expiry",p->pals_time,132);
    p=setup(); p->fist_incs=42; p->posz=20000; p->poszv=1000; p->on_ground=0;
    processinput(0); check("fist exit follows physics",p->posz,21256);
    check("fist marks level exit",p->gm,MODE_EOL);
    p=setup(); p->newowner=1; p->truefz=111; p->truecz=222; processinput(0);
    check("beta no true floor slot",p->truefz,111); check("beta no true ceiling slot",p->truecz,222);
    p=setup(); p->curr_weapon=RPG_WEAPON;
    { int a=insertsprite(0,1),j;
      memset(&sprite[a],0,sizeof(sprite[a])); sprite[a].sectnum=0; sprite[a].statnum=1;
      sprite[a].x=20480; sprite[a].y=16384; sprite[a].z=32768;
      sprite[a].picnum=LIZTROOP; sprite[a].extra=100; sprite[a].cstat=257;
      sprite[a].xrepeat=48; sprite[a].yrepeat=40; tilesizy[LIZTROOP]=32;
      sintable[464]=sintable[560]=16206; sintable[48]=2404; sintable[2000]=-2404;
      shoot(p->i,RPG); j=headspritestat[4]; check("RPG aimed beta height",sprite[j].zvel,535);
    }
    p=setup(); p->curr_weapon=CHAINGUN_WEAPON; p->kickback_pic=10;
    p->ammo_amount[CHAINGUN_WEAPON]=10; sync[0].bits=4; processinput(0);
    check("chaingun wraps past ten",p->kickback_pic,1);
    p=setup(); p->timebeforeexit=79; processinput(0);
    check("exit quote at78",p->ftq,102);
    p=setup(); ud.scrollmode=ud.overhead_on=1; fricxv=123; fricyv=456;
    CONTROL_ButtonState1=CONTROL_ButtonState2=0; button(gamefunc_Move_Forward); getinput(0);
    check("map scroll records movement",loc.fvel,1280);
    p=setup(); p->heat_on=1; p->heat_amount=1; p->inven_icon=5; p->firstaid_amount=50;
    doincrements(p); check("heat expiry clears icon",p->inven_icon,0);
    p=setup(); p->holoduke_on=1; p->holoduke_amount=2; p->inven_icon=3; p->firstaid_amount=50;
    doincrements(p); check("holo expiry clears icon",p->inven_icon,0);
    p=setup(); p->jetpack_on=1; p->jetpack_amount=2; p->inven_icon=4; p->firstaid_amount=50;
    doincrements(p); check("jetpack expiry clears icon",p->inven_icon,0);
    p=setup(); p->spacesuit_on=1; p->spacesuit_amount=1; p->inven_icon=6; p->firstaid_amount=50;
    doincrements(p); check("suit expiry clears icon",p->inven_icon,0);
    p=setup(); p->scuba_on=1; p->scuba_amount=1; p->inven_icon=6; p->firstaid_amount=50;
    doincrements(p); check("scuba expiry clears icon",p->inven_icon,0);
    p=setup(); randomseed=4; sprite[p->i].extra=p->last_extra=0; processinput(0);
    check("death flag remains signed",p->dead_flag,-313);
    p=setup(); sprite[p->i].extra=90; pus=0; incur_damage(p);
    check("damage requests status redraw",pus,1);
    if(failures) return 1;
    puts("beta player: matches EXE cases"); return 0;
}
