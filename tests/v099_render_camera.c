/* Beta camera inputs reaching the real classic renderer (EXE 0x21950). */
#define SDL_MAIN_HANDLED
#include "duke3d.h"
#include "platform.h"
#include "SDL.h"

extern int32_t globalposx,globalposy,globalposz,globalhoriz;
extern short globalang;
extern SDL_Surface *surface;
static int failures,player,other;

static void check(const char *name,int got,int want)
{
    if(got != want) { printf("FAIL %s: %d want %d\n",name,got,want); failures++; }
}

static short solidtile(void)
{
    short t;
    int32_t i,n;
    for(t=0;t<MAXTILES;t++)
    {
        if(tilesizx[t]<64 || tilesizy[t]<64 || (picanm[t]&192)) continue;
        loadtile(t);
        if(!waloff[t]) continue;
        n=tilesizx[t]*tilesizy[t];
        for(i=0;i<n;i++) if(((unsigned char *)waloff[t])[i]==255) break;
        if(i==n) return t;
    }
    return -1;
}

static void room(short tile)
{
    int i;
    static const int xy[4][2]={{-8192,-8192},{8192,-8192},{8192,8192},{-8192,8192}};
    numwalls=4; numsectors=1;
    memset(sector,0,sizeof(sector[0]));
    sector[0].wallnum=4;
    sector[0].ceilingz=-16384; sector[0].floorz=16384;
    sector[0].ceilingpicnum=sector[0].floorpicnum=tile;
    for(i=0;i<4;i++)
    {
        memset(&wall[i],0,sizeof(wall[i]));
        wall[i].x=xy[i][0]; wall[i].y=xy[i][1]; wall[i].point2=(i+1)&3;
        wall[i].nextwall=wall[i].nextsector=-1;
        wall[i].picnum=tile; wall[i].xrepeat=wall[i].yrepeat=8;
    }
    initspritelists();
    player=insertsprite(0,10); other=insertsprite(0,0);
}

static struct player_struct *reset(void)
{
    struct player_struct *p=&ps[0];
    memset(p,0,sizeof(*p));
    memset(&sprite[player],0,sizeof(sprite[player]));
    memset(&sprite[other],0,sizeof(sprite[other]));
    sprite[player].sectnum=sprite[other].sectnum=0;
    sprite[player].statnum=10; sprite[other].statnum=0;
    sprite[player].cstat=sprite[other].cstat=32768;
    sprite[player].picnum=APLAYER; sprite[other].picnum=CAMERA1;
    sprite[player].xrepeat=sprite[player].yrepeat=64;
    sprite[other].xrepeat=sprite[other].yrepeat=64;
    sprite[other].x=1024; sprite[other].z=8192; sprite[other].shade=20;
    sprite[other].yvel=100;
    p->i=player; p->cursectnum=0; p->newowner=p->on_crane=-1;
    p->posz=p->oposz=8192; p->pyoff=1024; p->opyoff=-2048;
    p->horiz=p->ohoriz=100; p->visibility=512;
    p->truecz=-16384; p->truefz=16384;
    hittype[player].ceilingz=-16384; hittype[player].floorz=16384;
    numplayers=ud.multimode=1; myconnectindex=screenpeek=connecthead=0;
    connectpoint2[0]=-1; ud.camerasprite=camsprite=-1;
    ud.overhead_on=ud.show_help=ud.pause_on=ud.screen_size=ud.screen_tilting=0;
    ud.detail=1; ud.const_visibility=512; widescreen=0;
    numinterpolations=mirrorcnt=pub=0; earthquaketime=0;
    memset(gotpic,0,sizeof(gotpic));
    return p;
}

static uint32_t frame(void)
{
    uint32_t hash=2166136261u;
    int x,y;
    nextpage();
    for(y=0;y<ydim;y++) for(x=0;x<xdim;x++)
    {
        hash^=((unsigned char *)surface->pixels)[y*surface->pitch+x];
        hash*=16777619u;
    }
    return hash;
}

static void camera(const char *name,int x,int z,int angle,int horizon)
{
    clearallviews(0);
    displayrooms(0,32768);
    printf("%s: camera %d,%d,%d angle %d horizon %d\n",name,
        globalposx,globalposy,globalposz,globalang,globalhoriz);
    check(name,globalposx,x); check(name,globalposy,0);
    check(name,globalposz,z); check(name,globalang,angle);
    check(name,globalhoriz,horizon);
}

int main(int argc,char **argv)
{
    struct player_struct *p;
    short tile;
    uint32_t actual,reference;
    setvbuf(stdout,NULL,_IONBF,0);
    if(argc!=2 || initgroupfile(argv[1])==-1) return 2;
    initengine();
    if(loadpics("tiles000.art")!=0 || (tile=solidtile())<0) return 1;
    _platform_init(argc,argv,"v099_render_camera","test");
    if(setgamemode(1,320,200)) return 1;
    setbrightness(0,palette); room(tile);

    p=reset(); camera("direct current bob",0,9216,0,100);
    actual=frame();
    clearallviews(0);
    /* Fixed camera from beta 0x22195, not a second camera implementation. */
    drawrooms(0,0,9216,0,100,0); drawmasks(); reference=frame();
    check("rendered frame at beta bob height",actual==reference,1);
    p->opyoff=777777; p->truecz=999999; p->truefz=-999999;
    memset(gotpic,0,sizeof(gotpic));
    camera("absent native fields ignored",0,9216,0,100);
    check("absent fields leave rendered frame unchanged",frame()==reference,1);

    p=reset(); p->pyoff=10000; camera("beta floor cap",0,14336,0,100);
    p=reset(); p->posz=p->oposz=-16000; p->pyoff=0;
    camera("no later ceiling margin",0,-16000,0,100);
    p=reset(); p->on_ground=1; earthquaketime=1;
    camera("no second earthquake offset",0,9216,0,100);
    p=reset(); sprite[player].pal=1;
    camera("no later shrink palette offset",0,9216,0,100);

    p=reset(); p->over_shoulder_on=1;
    camera("third person height",-2048,5120,0,100);
    p=reset(); p->over_shoulder_on=1; p->posz=p->oposz=-16000;
    camera("third person ceiling clearance",-2048,-11264,0,100);
    p=reset(); p->over_shoulder_on=1; p->posz=p->oposz=19000;
    camera("third person floor clearance",-2048,13312,0,100);

    p=reset(); p->newowner=other; p->posz=-16000; p->oposz=8192;
    camera("remote camera ceiling and shade",0,-12288,0,120);
    p=reset(); ud.camerasprite=other;
    camera("sprite camera direct z",1024,8192,0,100);

    uninitengine();
    if(failures) return 1;
    puts("beta real-renderer camera: PASS"); return 0;
}
