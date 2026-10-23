/* Runs the real game: a boss replay must reach the actual ending, remain
   responsive for 60 seconds, draw every screen, and finish normal shutdown.
   Only the private game copy promotes the completed replay into gameplay EOL;
   normal attract playback intentionally returns to title instead. Score uses
   queued key skips, not its 10360-tick timeout. Color PPMs are render evidence,
   not DOS pixel goldens; the original-executable comparison is a separate audit.
   Beta quit skips later-version ad waits because their art is absent; entering
   either wait fails this test before normal shutdown. */
#define SDL_MAIN_HANDLED
#include <stdint.h>
#include <SDL.h>
#include "duke3d.h"

int duke3d_main(int argc, char **argv);
extern int32_t bytesperline;
static const char *const names[] = {
    "victory", "score", "ordering0", "ordering1", "ordering2", "ordering3",
    "title"
};
static int phase = -1, scorekeys;
static uint32_t entered, held, victory_frames, last_frames;
static int last_clock;
static unsigned victory_mask;
static uint32_t pictures[6];

void episode_test_fail(const char *reason)
{
    fprintf(stderr,"EPISODE FAIL: %s\n",reason);
    fflush(NULL);
    exit(2);
}

static void key(SDL_Keycode symbol)
{
    SDL_Event e;
    memset(&e,0,sizeof(e));
    e.type=SDL_KEYDOWN; e.key.state=SDL_PRESSED;
    e.key.keysym.sym=symbol; e.key.keysym.scancode=SDL_GetScancodeFromKey(symbol);
    if(SDL_PushEvent(&e)<0) episode_test_fail("key down injection");
    e.type=SDL_KEYUP; e.key.state=SDL_RELEASED;
    if(SDL_PushEvent(&e)<0) episode_test_fail("key up injection");
}

static uint32_t picture(int stage, int frame)
{
    uint32_t h=2166136261u;
    unsigned char *pixels=(unsigned char *)frameplace;
    unsigned char first=pixels[0];
    int x,y,changed=0;
    char path[64];
    FILE *f;
    if(xdim!=320 || ydim!=200) episode_test_fail("expected classic 320x200 framebuffer");
    snprintf(path,sizeof(path),"episode_%d_%d.ppm",stage,frame);
    f=fopen(path,"wb");
    if(!f) episode_test_fail("frame evidence file");
    fprintf(f,"P6\n320 200\n255\n");
    for(y=0;y<ydim;y++) {
        unsigned char *row=pixels+y*bytesperline;
        for(x=0;x<xdim;x++) {
            unsigned char *pal=(unsigned char *)ps[myconnectindex].palette+row[x]*3;
            unsigned char rgb[3]={pal[0]<<2,pal[1]<<2,pal[2]<<2};
            if(fwrite(rgb,1,3,f)!=3) episode_test_fail("frame evidence write");
            h=(h^row[x])*16777619u; changed|=row[x]!=first;
        }
    }
    fclose(f);
    if(!changed) episode_test_fail("blank rendered screen");
    printf("EPISODE pixels %s frame %d hash %08x\n",names[stage],frame,h);
    fflush(stdout);
    return h;
}

void episode_test_replay(void)
{
    if(!(ps[myconnectindex].gm&MODE_EOL) || ud.level_number!=5 || ud.from_bonus!=0)
        episode_test_fail("boss replay did not reach episode EOL");
    printf("EPISODE replay EOL level 5\n");
    fflush(stdout);
    video_requestmode(0,320,200);
}

void episode_test_frame(int stage, int frame)
{
    uint32_t now=SDL_GetTicks();
    if(stage>=7) episode_test_fail("beta quit must not wait on missing later-version ads");
    if(stage!=phase) {
        if(video_pending()) episode_test_fail("video change pending on an ending screen");
        if(stage!=phase+1) episode_test_fail("screen order");
        phase=stage; entered=now;
        printf("EPISODE phase %s\n",names[stage]);
        fflush(stdout);
        picture(stage,0);
    }
    if(stage==0) {
        victory_frames++;
        if(frame>0 && frame<=5 && !(victory_mask&(1u<<frame))) {
            pictures[frame]=picture(stage,frame);
            victory_mask|=1u<<frame;
        }
        if(now-entered>=held+10000) {
            if(victory_frames<=last_frames+1 || totalclock<=last_clock)
                episode_test_fail("victory rendering/timer stopped");
            held=now-entered; last_frames=victory_frames; last_clock=totalclock;
            printf("EPISODE alive %u ms frames %u clock %d\n",held,victory_frames,totalclock);
            fflush(stdout);
        }
        if(now-entered>=60000) {
            if(victory_mask!=62 || pictures[1]==pictures[2] || pictures[3]==pictures[4] || pictures[4]==pictures[5])
                episode_test_fail("victory animation was not rendered");
            printf("EPISODE victory held %u ms\n",now-entered); fflush(stdout);
            key(SDLK_SPACE);
        }
    } else if(stage==1) {
        if(!scorekeys && totalclock>720) {
            picture(stage,1); key(SDLK_SPACE); scorekeys=1;
        } else if(scorekeys==1 && totalclock>800 && totalclock<1000000000) {
            key(SDLK_SPACE); scorekeys=2;
        }
    } else if(stage>=2 && stage<=5 && now-entered>=500) key(SDLK_SPACE);
    SDL_Delay(1);
}

void episode_test_title(void)
{
    SDL_Event e;
    if(ps[myconnectindex].gm!=0 || phase!=5 || scorekeys!=2 || held<50000)
        episode_test_fail("episode did not return to title");
    episode_test_frame(6,0);
    /* Use the real quit confirmation and gameexit, not SDL_QUIT's direct exit. */
    cmenu(500); ps[myconnectindex].gm=MODE_MENU;
    memset(&e,0,sizeof(e)); e.type=SDL_KEYDOWN; e.key.state=SDL_PRESSED;
    e.key.keysym.sym=SDLK_y; e.key.keysym.scancode=SDL_SCANCODE_Y;
    if(SDL_PushEvent(&e)<0) episode_test_fail("quit key injection");
    KB_KeyWaiting();
    menus();
    episode_test_fail("quit confirmation returned");
}

void episode_test_shutdown(void)
{
    if(phase!=6) episode_test_fail("shutdown before ending screens completed");
    printf("EPISODE shutdown complete\n");
    fflush(stdout);
}

int main(int argc, char **argv)
{
    return duke3d_main(argc,argv);
}
