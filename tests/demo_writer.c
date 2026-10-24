#include "profile_test_support.h"
/* Recording must finalize even when its last batch filled the buffer. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"
#include "build/cache1d.h"
static int failures;
extern int32_t recfilep;
int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,"1.5")) return 1;
    game_init_release_defaults();
    int sizes[]={1,RECSYNCBUFSIZ,RECSYNCBUFSIZ+1},n,i;
    static const int protocol[]={9,10,12,14,103,116,117};
    int old=game_profile->gamever<GAMEVER_11;
    int early=game_profile->gamever<GAMEVER_13D;
    int atomic=game_profile->gamever>=GAMEVER_14;
    int header_size=atomic?675:early?9:24;
    if(game_profile->byteversion!=protocol[game_profile->id]) { puts("FAIL protocol byte"); failures++; }
    static unsigned char cache[1024*1024];
    initengine(); initcache((intptr_t)cache,sizeof(cache)); connecthead=0; connectpoint2[0]=-1; ud.multimode=1;
    for(n=0;n<3;n++) {
        unsigned char header[32]; FILE *f;
        ud.volume_number=0; ud.level_number=1; ud.player_skill=2; ud.recstat=ud.m_recstat=1; opendemowrite();
        memset(sync,0,sizeof(sync)); sync[0].fvel=123; sync[0].avel=-100; sync[0].horz=37; sync[0].bits=0x12345678;
        for(i=0;i<sizes[n];i++) record();
        closedemowrite();
        f=fopen("demo1.dmo","rb");
        if(!f || fread(header,1,12,f)!=12) return 1;
        fclose(f);
        i=header[0]|header[1]<<8|header[2]<<16|header[3]<<24;
        if(i!=sizes[n]) { printf("FAIL finalized count %d want %d\n",i,sizes[n]); failures++; }
        if(ud.recstat || ud.m_recstat) { puts("FAIL recording state not cleared"); failures++; }
        if(atomic) {
            if(header[4]!=protocol[game_profile->id] || header[5]!=0 || header[6]!=1 || header[7]!=2 || header[10]!=1 || header[11]!=0) {
                puts("FAIL Atomic header fields"); failures++;
            }
        } else if(header[4]!=0 || header[5]!=1 || header[6]!=2 || header[early?7:8]!=1 || header[early?8:9]!=0) {
            puts("FAIL historical header fields"); failures++;
        }
        {
            unsigned char input_bytes[10];
            int fil=kopen4load("demo1.dmo",0);
            klseek(fil,header_size,SEEK_SET);
            kdfread(input_bytes,10,1,fil); kclose(fil);
            static const unsigned char expected[2][10] = {
                {156,37,123,0,0,0,0x78,0x56,0x34,0x12}, /* 1.1 and later */
                {123,0,0,0,156,255,0x78,0x56,0x34,0x12} /* 0.99/1.0 */
            };
            if(memcmp(input_bytes,expected[old],10)) {
                puts("FAIL packed demo input"); failures++;
            }
        }
        /* Do not call close twice on the broken baseline's dangling stream. */
    }
    remove("demo1.dmo");
    /* Valid prefixes must not make incomplete release headers playable. */
    for(n=0;n<header_size;n++) {
        unsigned char header[675]={0}; FILE *f;
        if(atomic) { header[4]=protocol[game_profile->id]; header[10]=1; }
        else header[early?7:8]=1;
        f=fopen("demo1.dmo","wb");
        if(!f || fwrite(header,1,n,f)!=(size_t)n) return 1;
        fclose(f);
        if(opendemoread(1)) {
            printf("FAIL accepted %d-byte truncated header\n",n); failures++;
            kclose(recfilep);
        }
    }
    remove("demo1.dmo");

    puts("demo writer checks reached completion"); return failures!=0;
}
