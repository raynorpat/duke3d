/* Map lookup policy must keep the base campaign while retaining other overrides. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"
#include "profile_test_support.h"

static int bad;
#define CHECK(c) do { if(!(c)) { printf("FAIL line %d: %s\n",__LINE__,#c); bad++; } } while(0)
static void put16(unsigned char *b,int n) { b[0]=n; b[1]=n>>8; }
static void put32(unsigned char *b,int n) { put16(b,n); put16(b+2,n>>16); }
static void map(unsigned char b[194],int x)
{
    static const int xs[]={0,4096,4096,0},ys[]={0,0,4096,4096};
    int i;
    memset(b,0,194); put32(b,7); put32(b+4,x); put32(b+8,512);
    put16(b+20,1); put16(b+24,4); put32(b+26,-8192); put32(b+30,8192);
    put16(b+62,4);
    for(i=0;i<4;i++) {
        unsigned char *w=b+64+32*i;
        put32(w,xs[i]); put32(w+4,ys[i]); put16(w+8,(i+1)&3);
        put16(w+10,-1); put16(w+12,-1);
    }
}
static void group(const char *path,int x,int asset)
{
    unsigned char b[194],d[16]={0}; FILE *f=fopen(path,"wb");
    map(b,x); fwrite("KenSilverman",12,1,f); put32(d,2); fwrite(d,4,1,f);
    memset(d,0,16); memcpy(d,"E1L1.MAP",8); put32(d+12,194); fwrite(d,16,1,f);
    memset(d,0,16); memcpy(d,"TEXT.TXT",8); put32(d+12,1); fwrite(d,16,1,f);
    fwrite(b,194,1,f); fputc(asset,f); fclose(f);
}
static int load(char *name)
{
    int32_t x=0,y,z; short ang,sect;
    int r=game_load_campaign_map(name,&x,&y,&z,&ang,&sect);
    CHECK(r==0); return x;
}
int main(int argc,char **argv)
{
    unsigned char b[194]; char name[]="E1L1.MAP",text=0; int i,fil,sw;
    FILE *f;
    if(!test_select_profile(&argc,argv,"1.5")) return 1;
    game_init_release_defaults(); sw=game_profile->shareware;
    for(i=0;i<6;i++) {
        char campaign[]="E1L1.MAP"; campaign[3]+=(char)i;
        CHECK(game_map_allowed(0,i,campaign,0));
    }
    CHECK(game_map_allowed(0,0,"e1l1.map",0));
    CHECK(game_map_allowed(0,-1,"E1L1.MAP",0)==!sw);
    CHECK(game_map_allowed(1,0,"E2L1.MAP",0)==!sw);
    CHECK(game_map_allowed(0,7,"custom.map",1)==!sw);
    CHECK(game_map_allowed(0,0,"E1L2.MAP",0)==!sw);
    CHECK(game_map_allowed(0,0,"custom.map",0)==!sw);
    CHECK(game_map_allowed(0,0,"E1L1.MAP",1)==!sw);
    initengine(); group("base.grp",111,'B'); group("addon.grp",333,'A');
    CHECK(initgroupfile("base.grp")>=0); CHECK(initgroupfile("addon.grp")>=0);
    map(b,222); f=fopen("E1L1.MAP","wb"); fwrite(b,194,1,f); fclose(f);
    f=fopen("TEXT.TXT","wb"); fputc('L',f); fclose(f);
    ud.volume_number=ud.level_number=0; boardfilename[0]=0;
    CHECK(load(name)==(sw?111:222)); CHECK(!strcmp(name,"E1L1.MAP"));
    remove("E1L1.MAP"); CHECK(load(name)==(sw?111:333));
    fil=kopen4load("TEXT.TXT",0); kread(fil,&text,1); kclose(fil); CHECK(text=='L');
    remove("TEXT.TXT"); fil=kopen4load("TEXT.TXT",0); kread(fil,&text,1); kclose(fil); CHECK(text=='A');
    if(sw) {
        int32_t x=123,y=0,z=0; short ang=0,sect=0;
        strcpy(boardfilename,"E1L1.MAP");
        CHECK(game_load_campaign_map(name,&x,&y,&z,&ang,&sect)==-1); CHECK(x==123);
        boardfilename[0]=0; ud.level_number=7;
        CHECK(game_load_campaign_map(name,&x,&y,&z,&ang,&sect)==-1);
    }
    if(sw) {
        unsigned char header[24]={0};
        current_menu=602; menus(); CHECK(current_menu==600);
        current_menu=603; menus(); CHECK(current_menu==600);
        int early=game_profile->gamever<GAMEVER_13D;
        put32(header,1); header[5]=7; header[6]=1; header[early?7:8]=1;
        f=fopen("demo1.dmo","wb"); fwrite(header,early?9:24,1,f); fclose(f);
        CHECK(opendemoread(1)==0);
    }
    uninitgroupfile(); uninitengine(); return bad!=0;
}
