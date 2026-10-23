/*
 * Checks game helper functions against the original DOS libraries'
 *  results (MACT386.LIB), and in 1.3D builds the episode and skill names
 *  the 1.3D executable has built in. Needs no game data.
 */
#define SDL_MAIN_HANDLED  /* game.c's main() is renamed; this one is ours. */
#include <sys/stat.h>
#include "duke3d.h"
#include "glext.h"
#include "displaymode.h"
#include "videoopts.h"
#if PLATFORM_WIN32
#include <direct.h>
#define mkdir(d, m) _mkdir(d)
#define rmdir _rmdir
#endif

static int failures;
void showtwoscreens(void);
extern int32_t qsetmode;

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
    /* Early CON failures can exit before any video mode or framebuffer exists. */
    qsetmode=0;
    showtwoscreens();
    check("quit ads before video setup",qsetmode,0);
#if DUKE3D_GAMEVER >= GAMEVER_10 && DUKE3D_GAMEVER < GAMEVER_11
    {
        /* DOS 1.0's Watcom rand, initial seed 1 (sw10 0x97f61, 0xd0a8c). */
        static const int expected[] = {
            16838, 5758, 10113, 17515, 31051, 5627, 23010, 7419,
            16212, 4086, 2749, 12767, 9084, 12060, 32225, 17543
        };
        unsigned i;
        for(i = 0; i < sizeof(expected)/sizeof(expected[0]); i++)
        {
            /* Native networking's host RNG must not reset the DOS game stream. */
            if(i == 8) { srand(123); (void)(rand)(); }
            check("1.0 Watcom rand", i&1 ? game_rand() : rand(), expected[i]);
        }
        ud.multimode=1;
        ps[0].auto_run=1;
        newgame(0,0,2);
        check("1.0 new game preserves local auto-run",ps[0].auto_run,1);
    }
#endif
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
#if DUKE3D_GAMEVER < GAMEVER_10
    check_str("volume_names[1]", volume_names[1], "COSMIC WASTELAND");
    check_str("volume_names[2]", volume_names[2], "SHUTDOWN AT GROUND ZERO");
    check_str("skill_names[0]", skill_names[0], "EASY");
    check_str("skill_names[1]", skill_names[1], "MEDIUM");
    check_str("skill_names[2]", skill_names[2], "NORMAL");
    check_str("skill_names[3]", skill_names[3], "HARD");
    check_str("skill_names[4]", skill_names[4], "HEADACHE");
#else
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
#endif

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

    {
        char *args[] = { "duke3d", "-demodump", "/tmp/map-state.bin",
            "-loadverify", "/tmp/save.sav", "-saveverify", "/tmp/out.sav" };
        boardfilename[0]=0; checkcommandline(7,args);
        check("diagnostic paths don't enable respawn",ud.m_respawn_monsters,0);
        check("diagnostic paths don't select a map",boardfilename[0],0);
    }

    /* The GRP check: a known release GRP that isn't this build's release
       gets a warning; this build's own GRP, a modded one and an unknown one
       don't. CRC-32 is the standard (zlib) one. */
    check("grpcrc32(\"123456789\")", (int)grpcrc32(0, (const unsigned char *)"123456789", 9), (int)0xCBF43926);
    check("unknown GRP", grpmismatch(123456, 0x12345678) != NULL, 0);
    check("modded GRP the size of a release", grpmismatch(26524524, 0x12345678) != NULL, 0);
#if DUKE3D_GAMEVER < GAMEVER_10
    check("0.99 shareware GRP", grpmismatch(9690241, 0x02F18900) != NULL, 0);
    check("1.0 shareware GRP", grpmismatch(10429258, 0xA28AA589) != NULL, 1);
    check("1.1 shareware GRP", grpmismatch(10442980, 0x912E1E8D) != NULL, 1);
    check("1.3D shareware GRP", grpmismatch(11035779, 0x983AD923) != NULL, 1);
#elif DUKE3D_GAMEVER < GAMEVER_11
    check("1.0 shareware GRP", grpmismatch(10429258, 0xA28AA589) != NULL, 0);
    check("1.1 shareware GRP", grpmismatch(10442980, 0x912E1E8D) != NULL, 1);
    check("1.3D shareware GRP", grpmismatch(11035779, 0x983AD923) != NULL, 1);
#elif DUKE3D_GAMEVER < GAMEVER_13D
    check("1.1 shareware GRP", grpmismatch(10442980, 0x912E1E8D) != NULL, 0);
    check("1.0 shareware GRP", grpmismatch(10429258, 0xA28AA589) != NULL, 1);
    check("1.3D shareware GRP", grpmismatch(11035779, 0x983AD923) != NULL, 1);
#elif DUKE3D_GAMEVER < GAMEVER_14
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
#if DUKE3D_GAMEVER >= GAMEVER_10
    check("0.99 shareware GRP", grpmismatch(9690241, 0x02F18900) != NULL, 1);
#endif
#if DUKE3D_GAMEVER >= GAMEVER_13D
    check("1.1 shareware GRP", grpmismatch(10442980, 0x912E1E8D) != NULL, 1);
#endif

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
              "ScreenshotFormat = \"JPG\"\nGLDrawPath = \"Shader\"\n"
              "GLInternalWidth = -5\nGLInternalHeight = 99999\n"
              "GLContrast = \"9.5\"\nGLSaturation = \"-1\"\n"
              "DisplayMode = \"Exclusive\"\nVSync = 0\n"
              "Supersampling = \"8K\"\nGLTextureFilter = \"Trilinear\"\n", f);
        fclose(f);
        CONFIG_GetSetupFilename();
        CONFIG_ReadSetup();
        check("cfg Renderer 7", RendererSetting, 3);
        check("cfg GLTextureMipLevel 99", GLTextureMipLevel, 15);
        check_str("cfg ScreenshotFormat JPG", ScreenshotFormat, "jpg");
        check("cfg GLDrawPath Shader", GLDrawPath, GLPATH_SHADER);
        check("cfg GLInternalWidth -5", GLInternalWidth, 0);
        check("cfg GLInternalHeight 99999", GLInternalHeight, 4320);
        check("cfg GLContrast 9.5", GLContrast == GL_CONTRAST_MAX, 1);
        check("cfg GLSaturation -1", GLSaturation == GL_SATURATION_MIN, 1);
        check("cfg DisplayMode Exclusive", DisplayMode, DISPLAY_EXCLUSIVE);
        check("cfg VSync 0", VSync, 0);
        check("cfg Supersampling 8K", GLSupersampling, SS_8K);
        check("cfg GLTextureFilter Trilinear", GLTextureFilter, TEXFILTER_TRILINEAR);
        CONFIG_WriteSetup();
        f = fopen(cfg, "r");
        n = fread(text, 1, sizeof(text) - 1, f);
        fclose(f);
        text[n] = 0;
        check("cfg writes Renderer 3", strstr(text, "Renderer = 3") != NULL, 1);
        check("cfg writes GLTextureMipLevel 15", strstr(text, "GLTextureMipLevel = 15") != NULL, 1);
        check("cfg writes ScreenshotFormat jpg", strstr(text, "ScreenshotFormat = \"jpg\"") != NULL, 1);
        check("cfg writes GLDrawPath shader", strstr(text, "GLDrawPath = \"shader\"") != NULL, 1);
        check("cfg writes GLInternalWidth 0", strstr(text, "GLInternalWidth = 0") != NULL, 1);
        check("cfg writes GLInternalHeight 4320", strstr(text, "GLInternalHeight = 4320") != NULL, 1);
        check("cfg writes GLContrast 1.5", strstr(text, "GLContrast = \"1.5\"") != NULL, 1);
        check("cfg writes GLSaturation 0", strstr(text, "GLSaturation = \"0\"") != NULL, 1);
        check("cfg writes DisplayMode exclusive", strstr(text, "DisplayMode = \"exclusive\"") != NULL, 1);
        check("cfg writes VSync 0", strstr(text, "VSync = 0") != NULL, 1);
        check("cfg writes Supersampling 8k", strstr(text, "Supersampling = \"8k\"") != NULL, 1);
        check("cfg writes GLTextureFilter trilinear", strstr(text, "GLTextureFilter = \"trilinear\"") != NULL, 1);

        f = fopen(cfg, "w");
        fputs("[Screen Setup]\nRenderer = -2\nGLTextureMipLevel = -3\n"
              "GLDrawPath = \"bogus\"\nGLInternalWidth = 320\n"
              "GLContrast = \"0.75\"\nGLSaturation = \"1.25x\"\n"
              "DisplayMode = \"bogus\"\nVSync = 7\n"
              "Supersampling = \"16k\"\nGLTextureFilter = \"bogus\"\n", f);
        fclose(f);
        CONFIG_ReadSetup();
        check("cfg Renderer -2", RendererSetting, 0);
        check("cfg GLTextureMipLevel -3", GLTextureMipLevel, 0);
        check("cfg GLDrawPath bogus", GLDrawPath, GLPATH_AUTO);
        check("cfg GLInternalWidth 320", GLInternalWidth, 320);
        check("cfg GLContrast 0.75", GLContrast == 0.75, 1);
        check("cfg GLSaturation 1.25x (not a number)", GLSaturation == 1.0, 1);
        check("cfg DisplayMode bogus", DisplayMode, DISPLAY_BORDERLESS);
        check("cfg VSync 7", VSync, 1);
        check("cfg Supersampling 16k", GLSupersampling, SS_OFF);
        check("cfg GLTextureFilter bogus", GLTextureFilter, TEXFILTER_NEAREST);
        CONFIG_WriteSetup();
        CONFIG_ReadSetup();
        check("cfg GLContrast 0.75 round trip", GLContrast == 0.75, 1);
        check("cfg GLSaturation 1 round trip", GLSaturation == 1.0, 1);

        /* Unquoted decimals read as whole numbers in scriplib: ignored, and
         * written back quoted over the number. */
        f = fopen(cfg, "w");
        fputs("[Screen Setup]\nGLContrast = 1.25\nGLSaturation = 2\n", f);
        fclose(f);
        CONFIG_ReadSetup();
        check("cfg GLContrast 1.25 unquoted", GLContrast == 1.0, 1);
        check("cfg DisplayMode unset: borderless", DisplayMode, DISPLAY_BORDERLESS);
        check("cfg VSync unset: on", VSync, 1);
        check("cfg GLSaturation 2 unquoted", GLSaturation == 1.0, 1);
        CONFIG_WriteSetup();
        f = fopen(cfg, "r");
        n = fread(text, 1, sizeof(text) - 1, f);
        fclose(f);
        text[n] = 0;
        check("cfg writes GLContrast quoted over a number", strstr(text, "GLContrast = \"1\"") != NULL, 1);
        check("cfg writes GLSaturation quoted over a number", strstr(text, "GLSaturation = \"1\"") != NULL, 1);
        check("cfg writes DisplayMode borderless", strstr(text, "DisplayMode = \"borderless\"") != NULL, 1);
        check("cfg writes VSync 1", strstr(text, "VSync = 1") != NULL, 1);
        CONFIG_ReadSetup();
        check("cfg GLContrast after", GLContrast == 1.0, 1);

        /* The VIDEO page saves only the rows the player changed. The run
           here is renderer 0 at 320x200 borderless (nothing set), the file
           renderer 3 at 1024x768 windowed: a -renderer 0 run, say. */
        RendererSetting = 3;
        DisplayMode = DISPLAY_WINDOWED;
        ScreenWidth = 1024;
        ScreenHeight = 768;
        VSync = !getvideovsync();
        video_pagebegin();
        video_pagetouch(VROW_VSYNC);
        check("page untouched: Renderer kept", RendererSetting, 3);
        check("page untouched: DisplayMode kept", DisplayMode, DISPLAY_WINDOWED);
        check("page untouched: ScreenWidth kept", ScreenWidth, 1024);
        check("page touched: VSync in effect", VSync, getvideovsync());
        f = fopen(cfg, "r");
        n = fread(text, 1, sizeof(text) - 1, f);
        fclose(f);
        text[n] = 0;
        check("page untouched writes Renderer 3", strstr(text, "Renderer = 3") != NULL, 1);
        check("page untouched writes windowed", strstr(text, "DisplayMode = \"windowed\"") != NULL, 1);
        check("page untouched writes ScreenWidth 1024", strstr(text, "ScreenWidth = 1024") != NULL, 1);

        video_pagebegin();
        video_pagetouch(VROW_RENDERER);
        check("page touched: Renderer in effect", RendererSetting, 0);
        check("page touched others: DisplayMode kept", DisplayMode, DISPLAY_WINDOWED);
        f = fopen(cfg, "r");
        n = fread(text, 1, sizeof(text) - 1, f);
        fclose(f);
        text[n] = 0;
        check("page touched writes Renderer 0", strstr(text, "Renderer = 0") != NULL, 1);

#ifdef USE_OPENGL
        /* A change waiting for its fade is saved once it's made. */
        GLSupersampling = SS_OFF;
        video_pagebegin();
        video_requestsupersample(SS_4K);
        video_pagetouch(VROW_SUPERSAMPLING);
        check("page pending: not saved yet", GLSupersampling, SS_OFF);
        video_flushpending();
        check("page pending: saved once made", GLSupersampling, SS_4K);
        f = fopen(cfg, "r");
        n = fread(text, 1, sizeof(text) - 1, f);
        fclose(f);
        text[n] = 0;
        check("page pending writes Supersampling 4k", strstr(text, "Supersampling = \"4k\"") != NULL, 1);

        /* The page opened again during that fade keeps the change to save. */
        video_pagebegin();
        video_requestsupersample(SS_2K);
        video_pagetouch(VROW_SUPERSAMPLING);
        video_pagebegin();
        video_flushpending();
        check("page reopened while pending: saved", GLSupersampling, SS_2K);

        /* Leaving the game during a change's fade saves the change asked for. */
        video_pagebegin();
        video_requestsupersample(SS_4K);
        video_pagetouch(VROW_SUPERSAMPLING);
        video_pagesettle(1);
        check("page exit while pending: the change saved", GLSupersampling, SS_4K);
        video_flushpending();
#endif

        /* DISPLAY saves the choice, not the mode in effect (a fallback from
           fullscreen, or BUILD_WINDOWED): here no window, so borderless. */
        DisplayMode = DISPLAY_WINDOWED;
        video_pagebegin();
        video_pagesetdisplay(DISPLAY_EXCLUSIVE);
        check("page DISPLAY in effect differs", getvideodisplaymode() != DISPLAY_EXCLUSIVE, 1);
        check("page DISPLAY saves the choice", DisplayMode, DISPLAY_EXCLUSIVE);
        check("page DISPLAY steps from the choice", video_pagegetdisplay(), DISPLAY_EXCLUSIVE);
        video_pagebegin();
        check("page DISPLAY reopened: in effect", video_pagegetdisplay(), getvideodisplaymode());
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
