#include "profile_test_support.h"
/* Real beta sector/setup functions on a small in-memory board; no game data. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"

static int failures, player, projectile;
static void check(const char *name, int got, int want)
{
    if(got != want) { printf("FAIL %s: %d, want %d\n",name,got,want); failures++; }
}
static int add(int sect, int stat, int tile)
{
    int i=insertsprite(sect,stat);
    memset(&sprite[i],0,sizeof(sprite[i]));
    sprite[i].sectnum=sect; sprite[i].statnum=stat; sprite[i].picnum=tile;
    sprite[i].x=sprite[i].y=512; sprite[i].owner=-1;
    return i;
}
static int count_tile(int tile)
{
    int i,n=0;
    for(i=0;i<MAXSPRITES;i++) if(sprite[i].statnum < MAXSTATUS && sprite[i].picnum == tile) n++;
    return n;
}
static void setup(void)
{
    int i;
    static const int x[]={0,1024,1024,0},y[]={0,0,1024,1024};
    memset(sector,0,sizeof(sector)); memset(wall,0,sizeof(wall));
    memset(ps,0,sizeof(ps)); memset(hittype,0,sizeof(hittype));
    memset(sync,0,sizeof(sync)); memset(&ud,0,sizeof(ud));
    numsectors=3; numwalls=12;
    for(i=0;i<3;i++) { sector[i].wallptr=i*4; sector[i].wallnum=4;
        sector[i].ceilingz=-8192; sector[i].floorz=8192; }
    for(i=0;i<12;i++) { wall[i].x=x[i&3]; wall[i].y=y[i&3]; wall[i].point2=(i&~3)+((i+1)&3);
        wall[i].nextwall=wall[i].nextsector=-1; }
    initspritelists(); player=add(0,10,APLAYER); projectile=add(0,4,SHOTSPARK1);
    sprite[player].extra=100; sprite[player].xrepeat=48;
    sprite[projectile].extra=10; sprite[projectile].owner=projectile;
    ps[0].i=player; ps[0].newowner=-1; ps[0].somethingonplayer=-1;
    ps[0].cursectnum=0; ps[0].posx=ps[0].posy=512;
    ps[0].oposx=300; ps[0].oposy=400; ps[0].oposz=500; ps[0].oang=100;
    connecthead=0; connectpoint2[0]=-1; myconnectindex=screenpeek=0;
    ud.multimode=numplayers=1; max_player_health=100; max_armour_amount=100;
    animatecnt=numinterpolations=numcyclers=numanimwalls=0;
    totalclock=100; randomseed=17; SoundToggle=0; MusicToggle=0;
    sintable[0]=0; sintable[512]=16384;
}
int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    game_init_release_defaults();
    int i,j,k; int32_t seed;
    static const int32_t activate[]={69,777,4};
    setup(); ps[0].steroids_amount=400; resetinventory(0);
    check("unused steroids survive inventory reset",ps[0].steroids_amount,400);
    ps[0].over_shoulder_on=1; resetplayerstats(0);
    check("initial recenter",ps[0].return_to_center,4);
    check("third person cleared",ps[0].over_shoulder_on,0);
    setup(); ps[0].auto_run=1; ps[0].frag=4; ps[0].fraggedself=2; show_shareware=123;
    newgame(0,0,2); check("new game autorun",ps[0].auto_run,0);
    check("new game frags",ps[0].frag,0); check("new game self frags",ps[0].fraggedself,0);
    check("no shareware countdown",show_shareware,123);
    setup(); resetpspritevars(MODE_GAME);
    check("player width",sprite[ps[0].i].xrepeat,48);
    check("player height",sprite[ps[0].i].yrepeat,44);
    setup(); i=add(0,0,MUSICANDSFX); sprite[i].lotag=1; soundm[1]=16;
    sector[0].lotag=22; callsound(0,-1);
    check("callsound toggles flagged sound on split door",hittype[i].temp_data[0],1);
    setup(); i=add(0,0,MUSICANDSFX); j=add(1,0,MUSICANDSFX);
    sector[0].lotag=22; animatecnt=2;
    animateptr[0]=&sector[0].ceilingz; animategoal[0]=sector[0].ceilingz;
    animateptr[1]=&sector[1].ceilingz; animategoal[1]=1000; animatevel[1]=1;
    animatesect[0]=0; animatesect[1]=1; doanimations();
    check("completed animation sounds swapped sector",hittype[j].temp_data[0],1);
    check("completed sector stays silent",hittype[i].temp_data[0],0);
    setup(); numanimwalls=1; animwall[0].wallnum=0; animwall[0].tag=SCREENBREAK1;
    wall[0].picnum=SCREENBREAK6; wall[0].extra=FEMPIC1; animatewalls();
    check("FEMPIC1 screen remains broken",wall[0].picnum,SCREENBREAK7);
    setup(); sector[0].lotag=16; sector[0].extra=16; wall[0].nextsector=1;
    sector[1].floorz=1000; sector[2].floorz=16000; wall[1].nextsector=2;
    animatecnt=1; animateptr[0]=&sector[0].floorz; animatesect[0]=0;
    animatevel[0]=-16; animategoal[0]=1000; operatesectors(0,player);
    check("active lift reverses destination",animategoal[0],16000);
    check("active lift reverses velocity",animatevel[0],16);
    setup(); i=add(0,3,SECTOREFFECTOR); j=add(1,3,SECTOREFFECTOR);
    sector[1].lotag=23;
    sprite[i].lotag=sprite[j].lotag=11; sprite[i].hitag=sprite[j].hitag=42;
    k=add(0,0,MUSICANDSFX); sector[0].lotag=23; operatesectors(0,player);
    check("swing halves both start",hittype[i].temp_data[4]+hittype[j].temp_data[4],2);
    check("swing halves each toggle sound",hittype[k].temp_data[0],0);
    setup(); i=add(0,11,RESPAWN); sprite[i].lotag=42; sprite[i].hitag=BOX;
    sprite[i].extra=99; ud.monsters_off=1; operaterespawns(42);
    check("respawn timer in shade",sprite[i].shade,54);
    check("respawn keeps extra",sprite[i].extra,99);
    check("respawn always spawns star",headspritestat[5]>=0,1);
    setup(); numcyclers=1; cyclers[0][0]=0; cyclers[0][3]=12; cyclers[0][4]=777;
    cyclers[0][5]=0; i=add(0,8,ACTIVATORLOCKED); sprite[i].lotag=777;
    actorscrptr[BOX]=16; memset(script,0,sizeof(script)); memcpy(&script[8],activate,sizeof(activate));
    j=add(0,5,BOX); execute(j,0,0);
    check("CON activate toggles matching cycler",cyclers[0][5],1);
    check("CON activate applies cycler shade",sector[0].floorshade,12);
    check("CON activate toggles matching lock",sector[0].lotag&16384,16384);
    setup(); ud.fta_on=1; i=add(0,8,ACTIVATORLOCKED); sprite[i].lotag=777;
    operateactivators(777,-1); check("sector activation quotes to global player",ps[0].ftq,4);
    setup(); sector[0].lotag=22|0x8000; sector[0].extra=16;
    i=add(0,8,ACTIVATOR); sprite[i].lotag=777; j=add(0,0,MUSICANDSFX);
    operateactivators(777,0);
    check("activator has no extra split-door sound",hittype[j].temp_data[0],1);
    setup(); sector[0].lotag=18; sector[0].extra=16; wall[0].nextsector=1;
    wall[1].nextsector=2; sector[1].floorz=1000; sector[2].floorz=16000;
    animatecnt=1; animateptr[0]=&sector[0].floorz; animatesect[0]=0;
    animatevel[0]=-16; animategoal[0]=1000; i=add(0,0,MUSICANDSFX);
    operatesectors(0,player);
    check("ceiling lift reverses floor",animategoal[0],16000);
    j=getanimationgoal(&sector[0].ceilingz);
    check("ceiling lift creates ceiling animation",j>=0,1);
    if(j>=0) check("ceiling lift keeps gap",animategoal[j],-384);
    check("running ceiling lift sounds twice",hittype[i].temp_data[0],0);
    setup(); wall[0].picnum=FEMPIC1; wall[1].picnum=FEMPIC2; wall[2].picnum=FEMPIC3;
    ud.lockout=1; prelevel(MODE_GAME);
    check("lockout first picture",wall[0].picnum,VIEWSCREENBROKE);
    check("lockout second picture",wall[1].picnum,VIEWSCREENBROKE);
    check("lockout third picture",wall[2].picnum,VIEWSCREENBROKE);
    setup(); wall[0].overpicnum=wall[1].overpicnum=wall[2].overpicnum=W_FORCEFIELD+1;
    wall[1].cstat=256; wall[2].cstat=341; wall[2].shade=32;
    prelevel(MODE_GAME);
    check("forcefield keeps original texture orientation",wall[0].cstat,85);
    check("forcefield preserves mapped flip",wall[1].cstat,341);
    check("disabled forcefield clears all flags",wall[2].cstat,0);
    setup(); wall[0].overpicnum=MIRROR; wall[0].cstat=16;
    checkhitwall(projectile,0,0,512,0,HYDRENT);
    check("hydrant does not break mirror",wall[0].overpicnum,MIRROR);
    setup(); numwalls=14; wall[12].x=256; wall[12].y=256; wall[12].point2=13;
    wall[13].x=768; wall[13].y=256; wall[12].picnum=FEMPIC2;
    checkhitwall(projectile,12,256,256,0,SHOTSPARK1);
    check("picture breaks to viewscreen",wall[12].picnum,VIEWSCREENBROKE);
    check("screen throws40 glass pieces",count_tile(GLASSPIECES)+count_tile(GLASSPIECES+1)+count_tile(GLASSPIECES+2),40);
    setup(); wall[0].overpicnum=FANSPRITE; wall[0].nextsector=1; wall[0].cstat=16;
    sector[1].floorz=8192; sector[1].ceilingz=-8192;
    checkhitwall(projectile,0,0,512,0,SHOTSPARK1);
    for(i=headspritestat[5],k=15;i>=0;i=nextspritestat[i],k--)
        check("fan scrap owns loop index",sprite[i].owner,k);
    check("fan makes16 scrap pieces",k,-1);
    setup(); wall[1].overpicnum=GLASS; wall[1].nextsector=1;
    wall[1].nextwall=4; wall[1].cstat=wall[4].cstat=wall[5].cstat=0x2dd;
    checkhitwall(projectile,1,512,512,0,SHOTSPARK1);
    check("glass clears hit wall",wall[1].cstat,0);
    check("beta glass leaves opposite wall blocking",wall[4].cstat,0x2dd);
    check("beta glass clears wall relative to hit wall",wall[5].cstat,0);
    setup(); wall[1].overpicnum=GLASS; wall[1].nextsector=1;
    wall[1].nextwall=MAXWALLS-1; wall[MAXWALLS-1].cstat=0x2dd;
    checkhitwall(projectile,1,512,512,0,SHOTSPARK1);
    check("unsafe glass target leaves opposite wall",wall[MAXWALLS-1].cstat,0x2dd);
    setup(); wall[1].overpicnum=STAINGLASS1; wall[1].nextsector=1;
    wall[1].nextwall=4; wall[1].cstat=wall[4].cstat=wall[5].cstat=0x2dd;
    checkhitwall(projectile,1,512,512,0,SHOTSPARK1);
    check("stained glass clears opposite wall",wall[4].cstat,0);
    check("stained glass keeps unrelated wall",wall[5].cstat,0x2dd);
    setup(); i=add(0,0,MONK); checkhitsprite(i,projectile);
    check("MONK fifth gib count",count_tile(JIBS5),7);
    setup(); i=add(0,1,ROTATEGUN); sprite[i].xrepeat=48; sprite[projectile].picnum=RPG;
    checkhitsprite(i,projectile); check("RPG damage unchanged",sprite[projectile].extra,10);
    check("rotategun is knocked back",sprite[i].xvel,-40);
    check("rotategun sheds gib",count_tile(JIBS6),1);
    setup(); i=add(0,0,QUEBALL); sprite[projectile].picnum=KNEE; seed=randomseed;
    checkhitsprite(i,projectile); check("kick ball speed",sprite[i].xvel,128);
    check("kick ball consumes no random",randomseed,seed);
    setup(); ud.player_skill=3; checkhitsprite(player,projectile);
    check("hit snaps player x",ps[0].posx,300); check("hit snaps player angle",ps[0].ang,100);
    check("no skill damage increase",sprite[projectile].extra,10);
    setup(); sprite[player].xrepeat=47; sprite[projectile].picnum=SHRINKSPARK;
    checkhitsprite(player,projectile); check("shrunk player ignores shrink damage",hittype[player].extra,0);
    check("ignored shrink still leaves camera",ps[0].posx,300);
    setup(); sprite[projectile].picnum=SHRINKSPARK; checkhitsprite(player,projectile);
    check("normal width48 accepts shrink",hittype[player].extra,10);
    setup(); ps[0].oang=0; i=add(0,0,TOILET); sprite[i].lotag=1; sprite[i].x=800;
    sprite[i].xrepeat=sprite[i].yrepeat=32; sprite[player].extra=85;
    ps[0].last_extra=85; sync[0].bits=1<<29; checksectors(0);
    check("toilet beta duration",ps[0].last_pissed_time,1560);
    check("toilet heals ten percent",sprite[player].extra,95);
    check("toilet preserves last_extra",ps[0].last_extra,85);
    ps[0].toggle_key_flag=0; checksectors(0);
    check("flush resets toilet timer",ps[0].last_pissed_time,26);
    setup(); ps[0].oang=0; i=add(0,0,TOILET); sprite[i].lotag=1; sprite[i].x=800;
    sprite[i].xrepeat=sprite[i].yrepeat=32; sprite[player].extra=95;
    sync[0].bits=1<<29; checksectors(0);
    check("toilet has no health topup",sprite[player].extra,95);
    setup(); ps[0].oang=0; i=add(0,0,TOILET); sprite[i].lotag=1; sprite[i].x=800;
    sprite[i].xrepeat=sprite[i].yrepeat=32; sprite[player].extra=50;
    ps[0].holster_weapon=1; sync[0].bits=1<<29; checksectors(0);
    check("holstered toilet gives no health",sprite[player].extra,50);
    setup(); ps[0].cheat_phase=2; sync[0].bits=1<<17; cheatkeys(0);
    check("all cheat phases suppress keys",ps[0].auto_run,0);
    setup(); ps[0].curr_weapon=PISTOL_WEAPON; ps[0].weapon_pos=5;
    ps[0].show_empty_weapon=2; sync[0].bits=1<<22; cheatkeys(0);
    check("quick kick outside weapon gate",ps[0].quick_kick,24);
    check("quick kick changes weapon",ps[0].curr_weapon,KNEE_WEAPON);
    setup(); ud.fta_on=1; ps[0].ftq=42; ps[0].curr_weapon=PISTOL_WEAPON;
    sync[0].bits=1<<22; cheatkeys(0);
    check("quick kick has no quote80",ps[0].ftq,42);
    setup(); ps[0].curr_weapon=PISTOL_WEAPON; ps[0].show_empty_weapon=2;
    sync[0].bits=1<<19; cheatkeys(0);
    check("holster outside weapon gate",ps[0].holster_weapon,1);
    setup(); ps[0].curr_weapon=PISTOL_WEAPON; ps[0].last_pissed_time=9999;
    sprite[player].xrepeat=16; ps[0].gotweapon[SHOTGUN_WEAPON]=1;
    ps[0].ammo_amount[SHOTGUN_WEAPON]=10; sync[0].bits=3<<8; cheatkeys(0);
    check("shrunk toilet player can select weapon",ps[0].curr_weapon,SHOTGUN_WEAPON);
    setup(); ps[0].curr_weapon=PISTOL_WEAPON; ps[0].gotweapon[HANDBOMB_WEAPON]=1;
    ps[0].gotweapon[game_remote_weapon()]=1; i=add(0,1,HEAVYHBOMB); sprite[i].owner=player;
    sync[0].bits=6<<8; cheatkeys(0);
    check("no live bomb remote selection",ps[0].curr_weapon,PISTOL_WEAPON);
    setup(); ps[0].curr_weapon=PISTOL_WEAPON; ps[0].gotweapon[game_remote_weapon()]=1;
    sync[0].bits=10<<8; cheatkeys(0);
    check("key10 does nothing",ps[0].curr_weapon,PISTOL_WEAPON);
    setup(); sprite[player].extra=50; ps[0].firstaid_amount=10; ps[0].jetpack_amount=100;
    ps[0].inven_icon=1; sync[0].bits=1<<16; cheatkeys(0);
    check("partial medkit clears inventory icon",ps[0].inven_icon,0);
    setup(); i=add(0,0,CACTUS); checkplayerhurt(&ps[0],49152|i);
    check("hurt flash expiry",ps[0].pals_time,132);
    setup(); wall[0].overpicnum=W_FORCEFIELD; checkplayerhurt(&ps[0],32768);
    check("forcefield flash expiry",ps[0].pals_time,132);
    if(failures) return 1;
    puts("beta sector/setup: matches EXE cases"); return 0;
}
