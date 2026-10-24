#include "profile_test_support.h"
/* Native storage must hold every release without altering packed transport. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"
#include <stdio.h>
int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,"1.5")) return 1;
    int bad=0;
    if(sizeof(input)!=10 || sizeof(sectortype)!=40 || sizeof(walltype)!=32 || sizeof(spritetype)!=44) bad=1;
    if(sizeof(((struct player_struct *)0)->weaprecs)/sizeof(short)<256) {
        puts("FAIL native weapon record capacity cannot hold 1.3D"); bad=1;
    }
    if(sizeof(script)/sizeof(script[0])<20460) {
        puts("FAIL native script capacity cannot hold Atomic"); bad=1;
    }
    return bad;
}
