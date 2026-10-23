#include "profile_test_support.h"
/*
 * Compiles the in-repo CON scripts with the game's real compiler and checks
 *  that every script address it stores is a byte offset into script[].
 *
 * Address slots are found by compiling twice: once from game.con, and once
 *  from prefix.con, which is game.con with one action (5 slots) defined in
 *  front. Everything game.con emits moves up by 5 slots. A slot holding
 *  a value compiles the same both times; a slot holding a script address
 *  (jump target, label, actor or ai entry) grows by exactly 20 bytes. Every
 *  such slot, every non-define label and every actor entry must then lie in
 *  [0, scriptsize * 4). An absolute address doesn't.
 *
 * Run it from a directory holding game.con, DEFS.CON, USER.CON and
 *  prefix.con, or name that directory as the first argument.
 *
 * With a second argument, it instead compiles game.con once and compares
 *  script[] and actorscrptr[] word for word with a reference dump, as
 *  tests/v13d_save_decode.py writes from a DOS save: "script <i> <value>"
 *  and "actor <i> <value>" lines, addresses as byte offsets.
 */
#define SDL_MAIN_HANDLED  /* game.c's main() is renamed; this one is ours. */
#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif
#include "duke3d.h"

#define PREFIX_SLOTS 5  /* one action: five values, its keyword dropped */
#define SHIFT (PREFIX_SLOTS * 4)

static int32_t a_script[MAXSCRIPTSIZE];
static int32_t a_labelcode[MAXSCRIPTSIZE];
static int32_t a_actorscrptr[MAXTILES];
static int32_t a_size, a_labelcnt;

static int failures = 0;

static void fail(const char *what, int32_t index, int32_t value)
{
    if (failures++ < 20)
        printf("FAIL: %s [%d] = %d (0x%x)\n", what, index, value, value);
}

/* A stored script address must be a 4-aligned byte offset in the script. */
static int is_offset(int32_t v)
{
    return v >= 0 && v < a_size * 4 && (v & 3) == 0;
}

static int compile(char *filename)
{
    /* The compiler skips some slots (unset actor header fields), which the
       game only ever compiles into a zeroed script[]. */
    memset(script, 0, sizeof(script));

    /* The buffers compilecons() hands the compiler. */
    mymembuf = (char *)&hittype[0];
    labelcode = (int32_t *)&sector[0];
    label = (char *)&sprite[0];

    loadefs(filename, mymembuf);
    if (error || loadfromgrouponly)
    {
        printf("FAIL: compiling %s (errors: %d)\n", filename, error);
        return 0;
    }
    if (script[0] != SCRIPT_OFF(scriptptr))
    {
        printf("FAIL: script[0] = %d, not the script size in bytes\n", script[0]);
        return 0;
    }
    return 1;
}

/* No level name may keep the CR of a CRLF line. */
static void check_level_names(void)
{
    int32_t i;

    for (i = 0; i < 44; i++)
        if (strchr(level_names[i], '\r') != NULL)
            fail("level name keeps a CR", i, (int32_t)strlen(level_names[i]));
}

/* Compares the compiled script with a DOS reference dump. */
static int compare_reference(const char *reffile)
{
    static int32_t want_script[MAXSCRIPTSIZE], want_actor[MAXTILES];
    char name[16];
    int32_t i, v, words = 0, actors = 0;
    FILE *f = fopen(reffile, "r");

    if (f == NULL)
    {
        printf("FAIL: can't open %s\n", reffile);
        return 1;
    }
    while (fscanf(f, "%15s %d %d", name, &i, &v) == 3)
    {
        if (strcmp(name, "script") == 0 && i >= 0 && i < MAXSCRIPTSIZE)
            { want_script[i] = v; if (i >= words) words = i + 1; }
        else if (strcmp(name, "actor") == 0 && i >= 0 && i < MAXTILES)
            { want_actor[i] = v; actors++; }
    }
    fclose(f);

    if (!compile("game.con"))
        return 1;
    if (warning)
    {
        printf("FAIL: %d warning(s)\n", warning);
        return 1;
    }
    check_level_names();
    for (i = 0; i < MAXSCRIPTSIZE; i++)
        if (script[i] != (i < words ? want_script[i] : 0))
            fail("script differs from the reference, ours", i, script[i]);
    for (i = 0; i < MAXTILES; i++)
        if (actorscrptr[i] != want_actor[i])
            fail("actorscrptr differs from the reference, ours", i, actorscrptr[i]);

    printf("script: %d bytes, %d actors; reference: %d words, %d actors\n",
           script[0], actors, words, actors);
    if (words < 100 || actors == 0)
    {
        printf("FAIL: the reference is too small\n");
        failures++;
    }
    if (failures)
    {
        printf("FAIL: %d problem(s)\n", failures);
        return 1;
    }
    printf("PASS\n");
    return 0;
}

int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    int32_t i, a, b, addrslots = 0, addrlabels = 0, actors = 0;

    groupfile = -1;          /* no GRP: never offer the internal defaults */
    loadfromgrouponly = 0;

    if (argc > 1 && chdir(argv[1]) != 0)
    {
        printf("FAIL: can't change to %s\n", argv[1]);
        return 1;
    }
    if (argc > 2)
        return compare_reference(argv[2]);

    if (!compile("game.con"))
        return 1;
    /* The in-repo CONs have LF line ends; no letter may be cut off. */
    check_level_names();
    if (strcmp(level_names[0], "HOLLYWOOD HOLOCAUST") != 0)
        printf("FAIL: E1L1 is named \"%s\"\n", level_names[0]), failures++;
    a_size = script[0] / 4;
    a_labelcnt = labelcnt;
    memcpy(a_script, script, sizeof(script));
    memcpy(a_labelcode, labelcode, labelcnt * sizeof(int32_t));
    memcpy(a_actorscrptr, actorscrptr, sizeof(actorscrptr));

    if (!compile("prefix.con"))
        return 1;
    if (script[0] != a_script[0] + SHIFT || labelcnt != a_labelcnt + 1)
    {
        printf("FAIL: prefix.con compiled to %d bytes and %d labels, "
               "expected %d and %d\n",
               script[0], labelcnt, a_script[0] + SHIFT, a_labelcnt + 1);
        return 1;
    }

    /* script[]: jump targets and label operands. Slot 0 is the size. */
    for (i = 1; i < a_size; i++)
    {
        a = a_script[i];
        b = script[i + PREFIX_SLOTS];
        if (a == b)
            continue;
        addrslots++;
        if (b - a != SHIFT)
            fail("script slot moved by an odd amount", i, b - a);
        else if (!is_offset(a))
            fail("script slot address", i, a);
    }

    /* labelcode[]: defines hold values; actions, moves, ais and states
       hold addresses. prefix.con's action is label 0. */
    for (i = 0; i < a_labelcnt; i++)
    {
        a = a_labelcode[i];
        b = labelcode[i + 1];
        if (a == b)
            continue;
        addrlabels++;
        if (b - a != SHIFT)
            fail("label moved by an odd amount", i, b - a);
        else if (!is_offset(a))
            fail("labelcode", i, a);
    }

    /* actorscrptr[]: 0 means no script. */
    for (i = 0; i < MAXTILES; i++)
    {
        a = a_actorscrptr[i];
        if (a == 0)
            continue;
        actors++;
        if (!is_offset(a))
            fail("actorscrptr", i, a);
        else if (actorscrptr[i] != a + SHIFT)
            fail("actorscrptr didn't move with the script", i, actorscrptr[i]);
    }

    printf("script: %d bytes, %d address slots, %d address labels "
           "of %d, %d actors\n",
           a_size * 4, addrslots, addrlabels, a_labelcnt, actors);

    /* Guard against a vacuous pass. */
    if (actors == 0 || addrslots < 100 || addrlabels < 100)
    {
        printf("FAIL: too few script addresses found\n");
        failures++;
    }

    if (failures)
    {
        printf("FAIL: %d problem(s)\n", failures);
        return 1;
    }
    printf("PASS\n");
    return 0;
}
