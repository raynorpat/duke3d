/* Exercise the real input handler; wrong-source loading fails these fixtures. */
#define SDL_MAIN_HANDLED
#include "duke3d.h"
#include "config.h"
#include "platform.h"
#include "build/cache1d.h"

extern int setup_homedir(void);
static int failures;
static void check(const char *name, int ok)
{
    if (!ok) { printf("FAIL %s (menu=%d row=%d)\n", name, current_menu, probey); failures++; }
}
static void press(int key)
{
    memset((void *)KB_KeyDown, 0, sizeof(KB_KeyDown));
    if (key) KB_KeyDown[key] = 1;
    clearview(0L);
    menus();
}
static void writefile(const char *path)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) { perror(path); exit(2); }
    fputs("fixture", fp); fclose(fp);
}
static void openmaps(void)
{
    cmenu(100);
#ifdef PLUTOPAK
    probey = 4;
#else
    probey = 3;
#endif
    press(sc_Enter);
    check("permanent entry opens picker", current_menu == 120);
}

int main(int argc, char **argv)
{
    int i, source, launch = argc == 3;
    char paths[4][MAX_PATH];
    setvbuf(stdout, NULL, _IONBF, 0);
    if ((argc != 2 && !launch) || setup_homedir()) return 2;
    if (launch) {
        if (initgroupfile(argv[2]) < 0) return 2;
        CONFIG_SetDefaults(); compilecons();
    }
    initengine(); _platform_init(argc, argv, "usermaps_menu", "test");
    if (launch) {
        if (loadpics("tiles000.art")) return 2;
        genspriteremaps();
    }
    if (setgamemode(1, 320, 200)) return 2;
    numplayers = ud.multimode = 1; myconnectindex = screenpeek = connecthead = 0;
    connectpoint2[0] = -1; SoundToggle = MusicToggle = 0; ps[0].gm = MODE_MENU;
#ifdef VOLUMEONE
    cmenu(100); probey = 1; press(sc_Enter);
    check("shareware still shows episode restriction", current_menu == 20000);
    boardfilename[0] = 0;
    cmenu(100); probey = 3; press(sc_Enter);
    check("shareware has no selectable user maps entry", current_menu != 120);
#else
    strcpy(paths[0], "duplicate.map"); strcpy(paths[1], "maps/duplicate.map");
    GetPathFromEnvironment(paths[2], sizeof(paths[2]), "duplicate.map");
    GetPathFromEnvironment(paths[3], sizeof(paths[3]), "maps/duplicate.map");
    if (launch) {
        int32_t px, py, pz;
        short angle, sect;
        if (loadboard("E1L1.MAP", &px, &py, &pz, &angle, &sect) < 0) return 2;
        for (source = 0; source < 4; source++) {
            wall[0].shade = (source+1)*3;
            if (saveboard(paths[source], &px, &py, &pz, &angle, &sect)) return 2;
        }
        wall[0].shade = 0;
        if (saveboard("rooftop_rampage_extra_long_custom_map_name.map", &px, &py, &pz, &angle, &sect)) return 2;
        ud.last_level = -1;
    } else if (!strcmp(argv[1], "main")) {
        for (source = 0; source < 4; source++) writefile(paths[source]);
    } else if (!strcmp(argv[1], "bulk")) {
        for (i = 0; i < 300; i++) {
            char path[32]; snprintf(path, sizeof(path), "maps/z%03d.map", i); writefile(path);
        }
    } else if (!strcmp(argv[1], "long")) {
        memset(paths[0], 'a', 124); strcpy(paths[0]+124, ".map"); writefile(paths[0]);
    }
    openmaps();
    if (failures) return 1; /* Do not activate the old menu's unrelated skill page. */
    if (launch) {
        for (source = 0; source < 4; source++) {
            ps[0].gm = MODE_MENU; openmaps();
            for (i = 0; i < source; i++) press(sc_DownArrow);
            press(0); nextpage();
            if (!source) {
                int band, col, ink;
                for (i = 40; i < 80; i++)
                    check("picker cursor stays inside screen", *((unsigned char *)(frameplace+ylookup[i])) == 0);
                for (band = 0; band < 3; band++) {
                    int y = band == 0 ? 168 : (band == 1 ? 178 : 192);
                    ink = 0;
                    for (i = y; i < y+7; i++)
                        for (col = 0; col < 320; col++)
                            ink += *((unsigned char *)(frameplace+ylookup[i]+col)) != 0;
                    check("picker path, count, and navigation hints render", ink != 0);
                }
                screencapture("maps0000.png", 0);
            }
            press(sc_Enter);
            check("real map reaches difficulty page", current_menu == 110);
            press(sc_Enter);
            check("skill confirmation starts a game", (ps[0].gm & MODE_GAME) && !(ps[0].gm & MODE_MENU));
            check("engine loaded exact chosen source", wall[0].shade == (source+1)*3);
            check("selected difficulty applies", ud.player_skill == 2);
            printf("LAUNCHED source=%d path=%s sectors=%d skill=%d\n", source, boardfilename, numsectors, ud.player_skill);
        }
    } else if (!strcmp(argv[1], "empty") || !strcmp(argv[1], "long")) {
        press(sc_Enter);
        check("empty or overlong selection stays in picker", current_menu == 120);
        check("invalid choice does not alter map", !boardfilename[0]);
        press(sc_Escape); check("picker returns to episodes", current_menu == 100);
    } else if (!strcmp(argv[1], "bulk")) {
        for (i = 0; i < 299; i++) press(sc_DownArrow);
        press(sc_Enter); check("large list reaches skill page", current_menu == 110);
        remove("maps/z299.map"); press(sc_Enter);
        check("last large-list file remains selected", current_menu == 120);
        writefile("maps/z299.map"); press(sc_DownArrow); press(sc_Enter);
        check("last entry wraps to first", current_menu == 110);
        remove("maps/z000.map"); press(sc_Enter);
        check("wrapped choice is first file", current_menu == 120);
    } else {
        press(sc_Escape); check("back restores user maps episode row",
#ifdef PLUTOPAK
            current_menu == 100 && probey == 4);
#else
            current_menu == 100 && probey == 3);
#endif
        for (source = 0; source < 4; source++) {
            strcpy(boardfilename, "previous.map"); openmaps();
            for (i = 0; i < source; i++) press(sc_DownArrow);
            press(sc_Enter); check("map selection opens skill", current_menu == 110);
            check("custom map uses existing level slot", ud.m_volume_number == 0 && ud.m_level_number == 7);
            check("choice is pending until skill confirmation", !strcmp(boardfilename, "previous.map"));
            press(sc_Escape); check("skill back returns to picker", current_menu == 120);
            press(sc_Enter); check("back preserves selected map", current_menu == 110);
            remove(paths[source]); press(sc_Enter);
            check("confirmation rechecks exact selected source", current_menu == 120);
            check("unavailable map does not replace active map", !strcmp(boardfilename, "previous.map"));
            writefile(paths[source]); press(sc_Escape);
        }
        openmaps();
        remove(paths[0]); press(sc_Enter);
        check("disappearing choice remains in picker", current_menu == 120);
        writefile(paths[0]); press(sc_Escape);
        check("cancel preserves active map", !strcmp(boardfilename, "previous.map"));
        cmenu(100); probey = 0; press(sc_Enter);
        check("stock episode selects stock skill page", current_menu == 110 && ud.m_level_number == 0);
        check("stock episode clears custom map", !boardfilename[0]);
        press(sc_Escape); check("stock skill back returns to episodes", current_menu == 100);
    }
#endif
    uninitengine();
    if (failures) return 1;
    puts("user maps menu: PASS"); return 0;
}
