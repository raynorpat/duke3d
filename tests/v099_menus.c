#include "profile_test_support.h"
/* Exercise beta menu transitions through the real input handler. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"
#include "config.h"
#include "platform.h"
#include "SDL.h"
static int failures;
static void check(const char *name,int got,int want)
{ if(got!=want) { printf("FAIL %s: %d want %d\n",name,got,want); failures++; } }
static void menu(int number,int row,int key)
{
    memset((void *)KB_KeyDown,0,sizeof(KB_KeyDown));
    ps[0].gm=MODE_MENU; cmenu(number); probey=row;
    KB_KeyDown[key]=1;
    if(key==sc_Space) {
        SDL_Event event; memset(&event,0,sizeof(event));
        event.type=SDL_KEYDOWN; event.key.state=SDL_PRESSED; event.key.keysym.sym=SDLK_SPACE;
        event.key.keysym.scancode=SDL_SCANCODE_SPACE; SDL_PushEvent(&event);
    }
    menus();
}
int main(int argc,char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    setvbuf(stdout,NULL,_IONBF,0);
    initengine(); _platform_init(argc,argv,"v099_menus","test");
    if(setgamemode(1,320,200)) return 1;
    numplayers=ud.multimode=1; myconnectindex=screenpeek=connecthead=0;
    connectpoint2[0]=-1; SoundToggle=MusicToggle=0;
    menu(0,5,sc_Enter); check("sixth main item is quit",current_menu,500);
    menu(0,6,sc_Enter); check("main menu wraps at six",current_menu,100);
    menu(501,0,sc_Y); check("quit to title without recording",current_menu,0);
    check("quit title mode",ps[0].gm,MODE_DEMO);
    menu(990,0,sc_Enter); check("credits next page",current_menu,991);
    menu(996,0,sc_Enter); check("credits end returns to main",current_menu,0);
    menu(403,0,sc_Space); check("orders end returns to main",current_menu,0);
    menu(15000,0,sc_Escape); check("escape restore dismisses",ps[0].gm&MODE_RESTART,0);
    menu(15001,0,sc_N); check("declining live restore dismisses",ps[0].gm&MODE_RESTART,0);
    menu(100,1,sc_Enter); check("beta second episode is shareware page",current_menu,20000);
    {
        int video_row=8;
#if CONTROLS_CONFIG_MENU
        video_row++;
#endif
        RendererSetting=2; ScreenWidth=1280; ScreenHeight=720;
        GLContrast=1.25; GLSaturation=0.75;
        menu(200,video_row,sc_Enter);
        check("beta options opens video",current_menu,710);
        menu(710,3,sc_Enter);
        check("video vsync preserves saved renderer",RendererSetting,2);
        check("video vsync preserves saved width",ScreenWidth,1280);
        check("video vsync preserves saved height",ScreenHeight,720);
        check("video vsync preserves saved color",GLContrast==1.25 && GLSaturation==0.75,1);
        menu(710,0,sc_Escape);
        RendererSetting=1; ScreenWidth=1600; ScreenHeight=900;
        menu(200,video_row,sc_Enter);
        menu(710,3,sc_Enter);
        check("video reopen refreshes saved renderer",RendererSetting,1);
        check("video reopen refreshes saved width",ScreenWidth,1600);
        check("video reopen refreshes saved height",ScreenHeight,900);
    }
    if(failures) return 1;
    puts("beta menu transitions: PASS"); return 0;
}
