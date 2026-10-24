/* Conveyor exclusions and displacement from the existing DOS release branches. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"
#include "profile_test_support.h"

static int carry(int tile,int stat,int cstat)
{
    static const int x[]={0,4096,4096,0},y[]={0,0,4096,4096};
    int i,e,s;
    memset(sector,0,sizeof(sector)); memset(wall,0,sizeof(wall));
    memset(ps,0,sizeof(ps)); memset(hittype,0,sizeof(hittype));
    memset(sintable,0,sizeof(sintable)); sintable[512]=16384;
    numsectors=1; numwalls=4; sector[0].wallnum=4;
    sector[0].ceilingz=-8192; sector[0].floorz=8192;
    for(i=0;i<4;i++) {
        wall[i].x=x[i]; wall[i].y=y[i]; wall[i].point2=(i+1)&3;
        wall[i].nextwall=wall[i].nextsector=-1;
    }
    initspritelists(); e=insertsprite(0,3); s=insertsprite(0,stat);
    memset(&sprite[e],0,sizeof(sprite[e])); memset(&sprite[s],0,sizeof(sprite[s]));
    sprite[e].statnum=3; sprite[e].picnum=SECTOREFFECTOR;
    sprite[e].lotag=24; sprite[e].yvel=512;
    sprite[s].statnum=stat; sprite[s].picnum=tile; sprite[s].cstat=cstat;
    sprite[s].x=sprite[s].y=512; sprite[s].z=8192;
    sprite[s].xrepeat=sprite[s].yrepeat=32; hittype[s].floorz=8192;
    connecthead=0; connectpoint2[0]=-1; numplayers=1;
    moveeffectors();
    return sprite[s].x-512;
}
int main(int argc,char **argv)
{
    int i,bad=0,early,atomic;
    if(!test_select_profile(&argc,argv,"1.5")) return 1;
    game_init_release_defaults();
    early=game_profile->gamever<GAMEVER_13D;
    atomic=game_profile->gamever>=GAMEVER_14;
    for(i=0;i<4;i++) if(carry(CRANE+i,6,0)!=0 || carry(CRANE+i,0,0)!=0) {
        printf("FAIL crane %d carried\n",i); bad++;
    }
    if(carry(SLOTDOOR+1,0,16)!=0) { puts("FAIL aligned stat 0 carried"); bad++; }
    if(carry(BOX,0,0)!=(early?0:atomic?8:16)) { puts("FAIL stat 0 displacement"); bad++; }
    if(carry(BOX,6,16)!=(early?16:atomic?8:0)) { puts("FAIL aligned stat 6 displacement"); bad++; }
    return bad!=0;
}
