/* Release selection changes compiler vocabulary, limits and defaults together. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"
#include "profile_test_support.h"
extern char *keyw[];
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%d: %s\n",__LINE__,#c); bad++; } } while (0)
int main(int argc,char **argv)
{
    int bad=0, old;
    if(!test_select_profile(&argc,argv,NULL)) return 1;
    game_init_release_defaults();
    old=game_profile->gamever<GAMEVER_13D;
    CHECK(game_freeze_weapon()==(old?10:9));
    CHECK(game_remote_weapon()==(old?9:10));
    CHECK(game_weapon_case(9)==(old?HANDREMOTE_WEAPON:FREEZE_WEAPON));
    CHECK(game_weapon_case(10)==(old?FREEZE_WEAPON:HANDREMOTE_WEAPON));
    memset(&ps[0],0,sizeof(ps[0]));
    ps[0].curr_weapon=PISTOL_WEAPON;
    ps[0].wantweaponfire=-1;
    ps[0].gotweapon[DEVISTATOR_WEAPON]=1;
    ps[0].ammo_amount[DEVISTATOR_WEAPON]=10;
    ud.wchoice[0][0]=DEVISTATOR_WEAPON+1;
    checkavailweapon(&ps[0]);
    CHECK(ps[0].curr_weapon==(!game_profile->shareware || game_profile->id==GAME_099SW ? DEVISTATOR_WEAPON : KNEE_WEAPON));
    CHECK(game_script_limit()==(game_profile->gamever<GAMEVER_14?20000:20460));
    CHECK(game_sound_limit()==(old?255:game_profile->gamever<GAMEVER_14?348:450));
    CHECK(game_weaprecs_limit()==(game_profile->gamever<GAMEVER_14?256:16));
    CHECK(!strcmp(keyw[65],old?"shadeto":"ifmultiplayer"));
    CHECK(!strcmp(keyw[68],old?"ifplayersl":"debug"));
    CHECK(!strcmp(keyw[69],old?(game_profile->id==GAME_099SW?"activate":"endoflevel"):"endofgame"));
    CHECK(!strcmp(keyw[90],game_profile->gamever<GAMEVER_11?"debugprint":"ifawayfromwall"));
    CHECK(dukefriction==(game_profile->gamever<GAMEVER_11?0xcb00:0xcc00));
    if(game_profile->id==GAME_099SW) {
        CHECK(!strcmp(volume_names[1],"COSMIC WASTELAND"));
        CHECK(!strcmp(skill_names[4],"HEADACHE"));
    } else if(game_profile->gamever<GAMEVER_14) {
        CHECK(!strcmp(volume_names[2],old?"SCHRAPNEL CITY":"SHRAPNEL CITY"));
        CHECK(!strcmp(skill_names[0],"PIECE OF CAKE"));
    } else CHECK(!volume_names[0][0] && !skill_names[0][0]);
    if(game_profile->id==GAME_10SW) {
        CHECK(game_rand()==16838); CHECK(game_rand()==5758); CHECK(game_rand()==10113);
    }
    return bad!=0;
}
