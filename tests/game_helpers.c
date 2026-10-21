/*
 * Checks game helper functions against the original DOS libraries'
 *  results (MACT386.LIB), and in 1.3D builds the episode and skill names
 *  the 1.3D executable has built in. Needs no game data.
 */
#define SDL_MAIN_HANDLED  /* game.c's main() is renamed; this one is ours. */
#include <sys/stat.h>
#include "duke3d.h"
#if PLATFORM_WIN32
#include <direct.h>
#define mkdir(d, m) _mkdir(d)
#define rmdir _rmdir
#endif

static int failures;

static void check(const char *what, int got, int want)
{
    if (got != want) {
        printf("FAIL %s: got %d, want %d\n", what, got, want);
        failures++;
    }
}

static void check_str(const char *what, const char *got, const char *want)
{
    if (strcmp(got, want) != 0) {
        printf("FAIL %s: got \"%s\", want \"%s\"\n", what, got, want);
        failures++;
    }
}

int main(void)
{
    /* On an axis the original returns the exact length, not the estimate. */
    check("FindDistance2D(1000,0)", FindDistance2D(1000, 0), 1000);
    check("FindDistance2D(0,-1000)", FindDistance2D(0, -1000), 1000);
    check("FindDistance2D(-7,0)", FindDistance2D(-7, 0), 7);
    check("FindDistance2D(0,0)", FindDistance2D(0, 0), 0);
    /* Off the axes it is the estimate: 1000 - 31 - 7 + 0 + 0 for (1000,1). */
    check("FindDistance2D(1000,1)", FindDistance2D(1000, 1), 962);
    check("FindDistance2D(3000,4000)", FindDistance2D(3000, 4000), 5039);

    /* Renderer numbers from duke3d.cfg or -renderer: 0 classic to 3 OpenGL. */
    check("clamprenderer(-1)", clamprenderer(-1), 0);
    check("clamprenderer(0)",  clamprenderer(0),  0);
    check("clamprenderer(2)",  clamprenderer(2),  2);
    check("clamprenderer(3)",  clamprenderer(3),  3);
    check("clamprenderer(7)",  clamprenderer(7),  3);

#if DUKE3D_GAMEVER < GAMEVER_14
    /* 1.3D CONs have no definevolumename or defineskillname. */
    check_str("volume_names[0]", volume_names[0], "L.A. MELTDOWN");
    check_str("volume_names[1]", volume_names[1], "LUNAR APOCALYPSE");
#if DUKE3D_GAMEVER < GAMEVER_13D
    check_str("volume_names[2]", volume_names[2], "SCHRAPNEL CITY"); /* the 1.1 EXE's spelling */
#else
    check_str("volume_names[2]", volume_names[2], "SHRAPNEL CITY");
#endif
    check_str("skill_names[0]", skill_names[0], "PIECE OF CAKE");
    check_str("skill_names[1]", skill_names[1], "LET'S ROCK");
    check_str("skill_names[2]", skill_names[2], "COME GET SOME");
    check_str("skill_names[3]", skill_names[3], "DAMN I'M GOOD");

    /* 1.3D saves and loads locally even in a network game: a negative slot
       (the menu's multiplayer request) must not queue a request that 1.3D
       has no handler for. A directory named game0.sav makes the save's fopen
       fail, and there is no game1.sav, so neither call touches game state. */
    mkdir("game0.sav", 0755);
    multiflag = 0;
    saveplayer(-1);
    check("multiflag after saveplayer(-1)", multiflag, 0);
    multiflag = 0;
    loadplayer(-2);
    check("multiflag after loadplayer(-2)", multiflag, 0);
    rmdir("game0.sav");
#endif

    /* Command-line switches must not leave an episode or player count the
       game can't use: /v0 and /v4 gave episode -1, /q16 gave 0 players. */
    {
        static char *v0[] = { "duke3d", "/v0" }, *v9[] = { "duke3d", "/v9" };
        static char *q16[] = { "duke3d", "/q16" }, *q0[] = { "duke3d", "/q0" };
#ifdef VOLUMEONE
        int vols = 1;
#elif defined(PLUTOPAK)
        int vols = 4;
#else
        int vols = 3;
#endif
        checkcommandline(2, v0);
        check("/v0 episode in range", ud.m_volume_number >= 0 && ud.m_volume_number < vols, 1);
        checkcommandline(2, v9);
        check("/v9 episode in range", ud.m_volume_number >= 0 && ud.m_volume_number < vols, 1);
        checkcommandline(2, q16);
#if DUKE3D_GAMEVER < GAMEVER_13D
        check("/q16 players", ud.multimode, 1); /* 1.1 reads one digit */
#else
        check("/q16 players", ud.multimode, MAXPLAYERS);
#endif
        checkcommandline(2, q0);
        check("/q0 players", ud.multimode, 1);
        ud.multimode = 1;
    }

    /* The GRP check: a known release GRP that isn't this build's release
       gets a warning; this build's own GRP, a modded one and an unknown one
       don't. CRC-32 is the standard (zlib) one. */
    check("grpcrc32(\"123456789\")", (int)grpcrc32(0, (const unsigned char *)"123456789", 9), (int)0xCBF43926);
    check("unknown GRP", grpmismatch(123456, 0x12345678) != NULL, 0);
    check("modded GRP the size of a release", grpmismatch(26524524, 0x12345678) != NULL, 0);
#if DUKE3D_GAMEVER < GAMEVER_14
#ifdef VOLUMEONE
    check("1.3D shareware GRP", grpmismatch(11035779, 0x983AD923) != NULL, 0);
    check("1.3D full GRP", grpmismatch(26524524, 0xBBC9CE44) != NULL, 1);
#else
    check("1.3D full GRP", grpmismatch(26524524, 0xBBC9CE44) != NULL, 0);
    check("1.3D shareware GRP", grpmismatch(11035779, 0x983AD923) != NULL, 1);
#endif
    check("Atomic GRP", grpmismatch(44356548, 0xFD3DCFF1) != NULL, 1);
    check("1.4 GRP", grpmismatch(44348015, 0xF514A6AC) != NULL, 1);
#elif DUKE3D_GAMEVER < GAMEVER_15
    check("1.4 GRP", grpmismatch(44348015, 0xF514A6AC) != NULL, 0);
    check("Atomic GRP", grpmismatch(44356548, 0xFD3DCFF1) != NULL, 1);
#else
    check("Atomic GRP", grpmismatch(44356548, 0xFD3DCFF1) != NULL, 0);
    check("1.4 GRP", grpmismatch(44348015, 0xF514A6AC) != NULL, 1);
    check("1.3D full GRP", grpmismatch(26524524, 0xBBC9CE44) != NULL, 1);
#endif
    check("1.1 shareware GRP", grpmismatch(10442980, 0x912E1E8D) != NULL, 1);

#if PLATFORM_UNIX
    /* duke3d.cfg values out of range, or in capitals, are read and written
       back as the game uses them. (Elsewhere the setup file is picked from a
       list of the folder's .cfg files.) Last, as it loads a whole setup. The
       file lives under a temporary HOME, never the working directory's. */
    {
        FILE *f;
        char text[16384], home[256], cfg[512], *slash;
        const char *tmp = getenv("TMPDIR");
        size_t n;

        snprintf(home, sizeof(home), "%s/duke3d_cfg_XXXXXX",
                 tmp != NULL && tmp[0] != '\0' ? tmp : "/tmp");
        if (mkdtemp(home) == NULL) {
            printf("FAIL cfg temp dir\n");
            return 1;
        }
        setenv("HOME", home, 1);
        if (setup_homedir() != 0) {
            printf("FAIL cfg setup_homedir\n");
            return 1;
        }
        GetPathFromEnvironment(cfg, sizeof(cfg), "duke3d.cfg");
        f = fopen(cfg, "w");

        fputs("[Screen Setup]\nRenderer = 7\nGLTextureMipLevel = 99\n"
              "ScreenshotFormat = \"JPG\"\n", f);
        fclose(f);
        CONFIG_GetSetupFilename();
        CONFIG_ReadSetup();
        check("cfg Renderer 7", RendererSetting, 3);
        check("cfg GLTextureMipLevel 99", GLTextureMipLevel, 15);
        check_str("cfg ScreenshotFormat JPG", ScreenshotFormat, "jpg");
        CONFIG_WriteSetup();
        f = fopen(cfg, "r");
        n = fread(text, 1, sizeof(text) - 1, f);
        fclose(f);
        text[n] = 0;
        check("cfg writes Renderer 3", strstr(text, "Renderer = 3") != NULL, 1);
        check("cfg writes GLTextureMipLevel 15", strstr(text, "GLTextureMipLevel = 15") != NULL, 1);
        check("cfg writes ScreenshotFormat jpg", strstr(text, "ScreenshotFormat = \"jpg\"") != NULL, 1);

        f = fopen(cfg, "w");
        fputs("[Screen Setup]\nRenderer = -2\nGLTextureMipLevel = -3\n", f);
        fclose(f);
        CONFIG_ReadSetup();
        check("cfg Renderer -2", RendererSetting, 0);
        check("cfg GLTextureMipLevel -3", GLTextureMipLevel, 0);
        remove(cfg);
        while ((slash = strrchr(cfg, '/')) != NULL) {
            *slash = 0;
            if (rmdir(cfg) != 0 || strcmp(cfg, home) == 0)
                break;
        }
    }
#endif

    if (failures) {
        printf("%d failures\n", failures);
        return 1;
    }
    printf("game_helpers: all match\n");
    return 0;
}
