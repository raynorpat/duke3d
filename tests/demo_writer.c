#include "profile_test_support.h"
/* Recording must finalize even when its last batch filled the buffer. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"
#include "build/cache1d.h"
static int failures;
extern int32_t recfilep;
int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    game_init_release_defaults();
    int sizes[]={1,RECSYNCBUFSIZ,RECSYNCBUFSIZ+1},n,i;
    static unsigned char cache[1024*1024];
    initengine(); initcache((intptr_t)cache,sizeof(cache)); connecthead=0; connectpoint2[0]=-1; ud.multimode=1;
    for(n=0;n<3;n++) {
        unsigned char header[4]; FILE *f;
        ud.recstat=ud.m_recstat=1; opendemowrite();
        memset(sync,0,sizeof(sync)); sync[0].fvel=123; sync[0].avel=-100;
        for(i=0;i<sizes[n];i++) record();
        closedemowrite();
        f=fopen("demo1.dmo","rb");
        if(!f || fread(header,1,4,f)!=4) return 1;
        fclose(f);
        i=header[0]|header[1]<<8|header[2]<<16|header[3]<<24;
        if(i!=sizes[n]) { printf("FAIL finalized count %d want %d\n",i,sizes[n]); failures++; }
        if(ud.recstat || ud.m_recstat) { puts("FAIL recording state not cleared"); failures++; }
        /* Do not call close twice on the broken baseline's dangling stream. */
    }
    remove("demo1.dmo");
#if DUKE3D_GAMEVER == GAMEVER_13D
    /* A valid 1.3D prefix must not make an incomplete 24-byte header playable. */
    for(n=0;n<24;n++) {
        unsigned char header[24]={0}; FILE *f;
        header[8]=1; /* one player */
        f=fopen("demo1.dmo","wb");
        if(!f || fwrite(header,1,n,f)!=(size_t)n) return 1;
        fclose(f);
        if(opendemoread(1)) {
            printf("FAIL accepted %d-byte truncated header\n",n); failures++;
            kclose(recfilep);
        }
    }
    remove("demo1.dmo");
#endif
    puts("demo writer checks reached completion"); return failures!=0;
}
