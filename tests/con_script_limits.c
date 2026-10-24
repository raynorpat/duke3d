/* Compile a generated program through the real compiler at each release limit. */
#include "profile_test_support.h"
#include "duke3d.h"
int main(int argc,char **argv)
{
    FILE *f;
    int i, words;
    if(!test_select_profile(&argc,argv,"1.5") || argc!=3) return 2;
    game_init_release_defaults();
    words=atoi(argv[2]);
    f=fopen(argv[1],"wb");
    if(!f) return 2;
    fputs("state limit_test\n",f);
    for(i=0;i<words;i++) fputs("killit\n",f);
    fputs("ends\n\n",f);
    fclose(f);
    groupfile=-1;
    loadfromgrouponly=0;
    mymembuf=(char *)&hittype[0];
    labelcode=(int32_t *)&sector[0];
    label=(char *)&sprite[0];
    loadefs(argv[1],mymembuf);
    if(error) return 1;
    printf("Compiled limit probe: %d words.\n",script[0]/4);
    return 0;
}
