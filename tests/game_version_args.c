/* Early arguments must not leak into legacy bare-map/net parsing. */
#include <stdio.h>
#include <string.h>
#include "gameversion.h"
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); failures++; } } while (0)
int main(void)
{
    int failures=0,n,help; const GameProfile *p; char error[180];
    char *a[]={"duke3d","/gaddon.grp","-gameversion","1.3D-shareware","/l1","-map","custom.map","-net","peer",NULL};
    char *bad[]={"duke3d","-gameversion",NULL};
    char *unknown[]={"duke3d","-gameversion","1.3",NULL};
    char *dup[]={"duke3d","-gameversion","1.5","-gameversion","1.5",NULL};
    char *h[]={"duke3d","/?",NULL};
    n=9; CHECK(gameversion_parse_args(&n,a,&p,&help,error,sizeof(error)));
    CHECK(n==7 && p==game_profile_get(GAME_13DSW) && !help);
    CHECK(!strcmp(a[1],"/gaddon.grp") && !strcmp(a[2],"/l1") && !strcmp(a[3],"-map"));
    CHECK(!strcmp(a[4],"custom.map") && !strcmp(a[5],"-net") && !strcmp(a[6],"peer") && !a[7]);
    n=2; CHECK(!gameversion_parse_args(&n,bad,&p,&help,error,sizeof(error)));
    n=3; CHECK(!gameversion_parse_args(&n,unknown,&p,&help,error,sizeof(error)));
    n=5; CHECK(!gameversion_parse_args(&n,dup,&p,&help,error,sizeof(error)));
    n=2; CHECK(gameversion_parse_args(&n,h,&p,&help,error,sizeof(error)) && help && n==1 && !p);
    return failures!=0;
}
