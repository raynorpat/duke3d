#include "profile_test_support.h"
/* Early prediction must apply the same movement friction as processinput. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"

static void setup(int water, int shrunk)
{
    static const int x[]={0,32768,32768,0}, y[]={0,0,32768,32768};
    struct player_struct *p=&ps[0];
    int i,pi;
    memset(sector,0,sizeof(sector)); memset(wall,0,sizeof(wall));
    memset(ps,0,sizeof(ps)); memset(hittype,0,sizeof(hittype));
    memset(sync,0,sizeof(sync)); memset(&ud,0,sizeof(ud));
    numsectors=1; numwalls=4; sector[0].wallnum=4; sector[0].lotag=water?2:0;
    sector[0].ceilingz=-32768; sector[0].floorz=32768;
    for(i=0;i<4;i++) {
        wall[i].x=x[i]; wall[i].y=y[i]; wall[i].point2=(i+1)&3;
        wall[i].nextwall=wall[i].nextsector=-1;
    }
    initspritelists(); pi=insertsprite(0,10);
    memset(&sprite[pi],0,sizeof(sprite[pi]));
    sprite[pi].statnum=10; sprite[pi].picnum=APLAYER; sprite[pi].extra=100;
    sprite[pi].x=sprite[pi].y=16384; sprite[pi].z=32768;
    sprite[pi].xrepeat=48; sprite[pi].yrepeat=shrunk?20:40;
    p->i=pi; p->posx=p->posy=16384; p->posz=32768-PHEIGHT;
    p->last_extra=100; p->on_ground=1;
    p->newowner=p->on_crane=p->holoduke_on=-1;
    p->actorsqu=p->dummyplayersprite=p->last_weapon=-1;
    p->scuba_on=water; p->scuba_amount=100;
    ud.multimode=numplayers=1; connecthead=myconnectindex=screenpeek=0;
    connectpoint2[0]=-1; dukefriction=0xd000; randomseed=1;
    sync[0].fvel=128; sync[0].svel=64;
}

int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,"1.1")) return 1;
    game_init_release_defaults();
    int water,shrunk;
    initengine();
    for(water=0;water<2;water++) for(shrunk=0;shrunk<2;shrunk++) {
        int32_t vx,vy;
        setup(water,shrunk); processinput(0);
        vx=ps[0].posxv; vy=ps[0].posyv;
        setup(water,shrunk);
        myx=omyx=ps[0].posx; myy=omyy=ps[0].posy; myz=omyz=ps[0].posz;
        myxvel=myyvel=myzvel=0; myang=0; mycursectnum=0;
        myhoriz=100; myhorizoff=0; myonground=1;
        myjumpingcounter=myjumpingtoggle=myhardlanding=myreturntocenter=0;
        fakemovefifoplc=0; inputfifo[0][0]=sync[0];
        fakedomovethings();
        if(myxvel!=vx || myyvel!=vy) {
            printf("FAIL water %d shrunk %d: predicted %d/%d actual %d/%d\n",
                   water,shrunk,myxvel,myyvel,vx,vy);
            return 1;
        }
    }
    puts("prediction: early friction matches gameplay");
    return 0;
}
