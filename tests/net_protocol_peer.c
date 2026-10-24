#include "profile_test_support.h"
/* Data-free protocol/custom-map refusal peer using the real Build UDP transport.
 * Includes Atomic 1.5 (117) and its shareware protocol number (27), whose
 * game build is unsupported. This fixture does not run those game versions. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "platform.h"
#include "build.h"
#include "display.h"
#include "engine.h"
#include <SDL.h>

extern short myconnectindex, numplayers, connecthead, connectpoint2[MAXPLAYERS];
void timerhandler(void) { }
void keyhandler(void) { }
void faketimerhandler(void) { }
int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    char packet[2048], version[3];
    short other;
    int i, custom_map=0, byteversion=atoi(argv[argc-1]);
    for(i=1;i<argc;i++) if(!strcmp(argv[i],"-test-custom-map")) custom_map=1;
    Uint32 start;
    if(byteversion < 1 || byteversion > 255) return 2;
    setvbuf(stdout,NULL,_IONBF,0);
    _platform_init(argc,argv,"Protocol refusal peer","test");
    initmultiplayers(0,0,0);
    if(numplayers != 2) return 2;
    version[0]=6; version[1]=byteversion; version[2]=0;
    for(i=connecthead;i>=0;i=connectpoint2[i])
        if(i != myconnectindex) sendpacket(i,version,3);
    start=SDL_GetTicks();
    while(SDL_GetTicks()-start < 15000) {
        totalclock=(SDL_GetTicks()-start)*120/1000;
        if(getpacket(&other,packet) >= 3 && packet[0] == 6) {
            if(custom_map && packet[1] == version[1]) {
                char request[21]={8,7,0,1};
                memcpy(request+10,"custom.map",11);
                sendpacket(other,request,sizeof(request));
                puts("Custom map request sent.");
                for(i=0;i<100;i++) { totalclock++; getpacket(&other,packet); SDL_Delay(10); }
                uninitmultiplayers(); return 0;
            }
            if(packet[1] != version[1]) {
            sendpacket(other,version,3);
            printf("Protocol peer exchanged BYTEVERSION %d with %u.\n",byteversion,(unsigned char)packet[1]);
            puts("You cannot play Duke with different versions.");
            fflush(stdout);
            /* Poll briefly so the reliable transport can deliver our version. */
            for(i=0;i<100;i++) { totalclock++; getpacket(&other,packet); SDL_Delay(10); }
            uninitmultiplayers(); return 0;
        }
        }
        SDL_Delay(10);
    }
    return 1;
}
