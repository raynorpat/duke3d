#include "profile_test_support.h"
/* DOS death rolls draw the side bit before the low-byte angle. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"

int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    game_init_release_defaults();
    static const int seeds[]={1,17,1047}, angles[]={82,1070,58};
    static const uint32_t ends[]={2052237015u,86936935u,2419763165u};
    static const int x[]={0,32768,32768,0}, y[]={0,0,32768,32768};
    int n,i,pi;
    initengine();
    for(n=0;n<3;n++) {
        struct player_struct *p=&ps[0];
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
        memset(&sprite[pi],0,sizeof(sprite[pi]));
        sprite[pi].statnum=10; sprite[pi].picnum=APLAYER;
        sprite[pi].x=sprite[pi].y=16384; sprite[pi].z=32768;
        sprite[pi].xrepeat=48; sprite[pi].yrepeat=40;
        p->i=pi; p->posx=p->posy=16384; p->posz=32768-PHEIGHT;
        p->on_ground=1; p->newowner=p->on_crane=p->holoduke_on=-1;
        p->actorsqu=p->dummyplayersprite=p->last_weapon=-1;
        ud.multimode=numplayers=1; connecthead=myconnectindex=screenpeek=0;
        connectpoint2[0]=-1; randomseed=seeds[n];
        processinput(0);
        if(p->dead_flag!=angles[n] || (uint32_t)randomseed!=ends[n]) {
            printf("FAIL death seed %d: angle %d RNG %u\n",seeds[n],
                   p->dead_flag,(unsigned)randomseed);
            return 1;
        }
    }
    puts("death: DOS random draw order checked");
    return 0;
}
