/* Fingerprints select behavior, and explicit overrides never unlock shareware. */
#include <stdio.h>
#include <string.h>
#include "gameprofile.h"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); failures++; } } while (0)
int main(int argc, char **argv)
{
    static const struct {
        const char *token; int game, engine, byte, sw, pluto;
        uint64_t size; uint32_t crc;
    } cases[] = {
        {"0.99",199601040,19960104,9,1,0,9690241,0x02f18900},
        {"1.0",199601290,19960129,10,1,0,10429258,0xa28aa589},
        {"1.1",199602200,19960220,12,1,0,10442980,0x912e1e8d},
        {"1.3d-shareware",199604190,19960419,14,1,0,11035779,0x983ad923},
        {"1.3d",199604190,19960419,103,0,0,26524524,0xbbc9ce44},
        {"1.4",199610210,19960820,116,0,1,44348015,0xf514a6ac},
        {"1.5",199610320,20000614,117,0,1,44356548,0xfd3dcff1}
    };
    int failures=0, i; const GameProfile *p, *selected;
    const GameGrp *g, *mac; char error[160]; uint64_t size; uint32_t crc;
    unsigned char header[32]="KenSilverman"; FILE *fp;
    if (argc==3 && !strcmp(argv[1],"--identify")) {
        if (!game_grp_inspect(argv[2],&size,&crc,error,sizeof(error))) {
            fprintf(stderr,"%s\n",error); return 1;
        }
        g=game_grp_find(size,crc);
        puts(!g?"unknown":!g->profile?"unsupported":g->profile->token); return 0;
    }
    CHECK(argc==2); if(argc!=2) return 1;
    for(i=0;i<7;i++) {
        p=game_profile_find(cases[i].token); g=game_grp_find(cases[i].size,cases[i].crc);
        CHECK(p && g && g->profile==p); if(!p||!g) continue;
        CHECK(p->gamever==cases[i].game && p->enginerev==cases[i].engine);
        CHECK(p->byteversion==cases[i].byte && p->shareware==cases[i].sw && p->plutopak==cases[i].pluto);
        CHECK(game_profile_resolve(g,NULL,&selected)==GAME_RESOLVE_OK && selected==p);
        CHECK(game_profile_resolve(NULL,p,&selected)==GAME_RESOLVE_OK && selected==p);
        CHECK(!game_grp_find(cases[i].size,cases[i].crc^1));
        if(p->shareware) CHECK(game_profile_resolve(g,game_profile_find("1.5"),&selected)==GAME_RESOLVE_SHAREWARE_CONFLICT);
    }
    CHECK(game_profile_find("1.3D-SHAREWARE")==game_profile_get(GAME_13DSW));
    CHECK(!game_profile_find("1.3") && !game_profile_find(""));
    CHECK(game_profile_resolve(NULL,NULL,&selected)==GAME_RESOLVE_NEEDS_OVERRIDE);
    mac=game_grp_find(10444391,0xc5f71561); CHECK(mac && mac->shareware && !mac->profile);
    CHECK(game_profile_resolve(mac,NULL,&selected)==GAME_RESOLVE_UNSUPPORTED);
    CHECK(game_profile_resolve(mac,game_profile_get(GAME_13DSW),&selected)==GAME_RESOLVE_OK);
    CHECK(game_profile_resolve(mac,game_profile_get(GAME_15),&selected)==GAME_RESOLVE_SHAREWARE_CONFLICT);
    CHECK(grpcrc32(0,(const unsigned char *)"123456789",9)==0xcbf43926);
    CHECK(grpcrc32(grpcrc32(0,(const unsigned char *)"1234",4),(const unsigned char *)"56789",5)==0xcbf43926);
    fp=fopen(argv[1],"wb"); CHECK(fp!=NULL); if(!fp) return 1;
    fwrite(header,1,16,fp); fclose(fp);
    CHECK(game_grp_inspect(argv[1],&size,&crc,error,sizeof(error)) && size==16);
    CHECK(!game_grp_find(size,crc));
    /* The directory claims a missing member; forcing a version cannot repair it. */
    header[12]=1; header[28]=1;
    fp=fopen(argv[1],"wb"); fwrite(header,1,32,fp); fclose(fp);
    CHECK(!game_grp_inspect(argv[1],&size,&crc,error,sizeof(error)));
    fp=fopen(argv[1],"wb"); fwrite(header,1,8,fp); fclose(fp);
    CHECK(!game_grp_inspect(argv[1],&size,&crc,error,sizeof(error)));
    header[0]='X'; fp=fopen(argv[1],"wb"); fwrite(header,1,16,fp); fclose(fp);
    CHECK(!game_grp_inspect(argv[1],&size,&crc,error,sizeof(error)));
    remove(argv[1]); CHECK(!game_grp_inspect(argv[1],&size,&crc,error,sizeof(error)));
    CHECK(game_profile==NULL);
    CHECK(game_profile_select(game_profile_get(GAME_10SW)));
    CHECK(game_profile_select(game_profile_get(GAME_10SW)));
    CHECK(!game_profile_select(game_profile_get(GAME_15)));
    CHECK(game_profile->id==GAME_10SW);
    return failures!=0;
}
