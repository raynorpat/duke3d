/* Real filesystem fixtures catch wrong sources, truncation, and duplicate scans. */
#include "usermaps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

static int failures;
static void check(const char *name, int ok)
{
    if (!ok) { printf("FAIL %s\n", name); failures++; }
}
static void writefile(const char *path)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) { perror(path); exit(2); }
    fputs("fixture", fp);
    fclose(fp);
}

int main(int argc, char **argv)
{
    usermap_list list = {0};
    char before[4096], after[4096];
    size_t i;
    if (argc != 3 || !getcwd(before, sizeof(before))) return 2;
    check("scan succeeds", usermaps_scan(&list, ".", argv[1]) == 0);
    if (!strcmp(argv[2], "bulk")) {
        check("all 300 files survive byte count boundary", list.count == 307);
        for (i = 0; i < 300 && list.count == 307; i++) {
            char want[32];
            snprintf(want, sizeof(want), "bulk%03d.map", (int)i);
            check("bulk list sorted and complete", !strcmp(list.entries[i+1].name, want));
        }
    } else {
        const char *names[] = {"alpha.MAP", "duplicate.map", "duplicate.map",
            "duplicate.map", "duplicate.map", "rooftop_rampage_extended.map", "zeta.map"};
        const int sources[] = {0, 0, 1, 2, 3, 1, 3};
        check("only regular maps in four nonrecursive locations", list.count == 7);
        for (i = 0; i < 7 && list.count == 7; i++) {
            check("alphabetical order preserves case", !strcmp(list.entries[i].name, names[i]));
            check("duplicate name retains source", list.entries[i].source == sources[i]);
            check("discovered path opens", usermap_check(&list.entries[i], 4096) == USERMAP_READY);
        }
        if (list.count == 7) {
            char want[4096];
            check("current path stays relative", !strcmp(list.entries[1].path, "duplicate.map"));
            check("current maps path stays relative", !strcmp(list.entries[2].path, "maps/duplicate.map"));
            snprintf(want, sizeof(want), "%s/duplicate.map", argv[1]);
            check("profile duplicate has exact load path", !strcmp(list.entries[3].path, want));
            snprintf(want, sizeof(want), "%s/maps/duplicate.map", argv[1]);
            check("profile maps duplicate has exact load path", !strcmp(list.entries[4].path, want));
            remove("alpha.MAP");
            check("disappearing file is unavailable", usermap_check(&list.entries[0], 128) == USERMAP_UNREADABLE);
            writefile("alpha.MAP");
        }
        {
            char path[129];
            usermap_entry entry = {path, path, 0};
            memset(path, 'a', 123); strcpy(path+123, ".map"); writefile(path);
            check("127 byte path fits serialized field", usermap_check(&entry, 128) == USERMAP_READY);
            remove(path);
            memset(path, 'a', 124); strcpy(path+124, ".map"); writefile(path);
            check("128 byte path rejected without truncation", usermap_check(&entry, 128) == USERMAP_PATH_TOO_LONG);
            remove(path);
            entry.path = "directory.map";
            check("map named directory cannot be selected", usermap_check(&entry, 128) == USERMAP_UNREADABLE);
        }
        check("replace list when roots coincide", usermaps_scan(&list, ".", ".") == 0 && list.count == 4);
        check("normalize equivalent roots", usermaps_scan(&list, ".", "maps/..") == 0 && list.count == 4);
        if (strcmp(argv[2], "no-symlink"))
            check("alias directory scanned once", usermaps_scan(&list, ".", "../alias") == 0 && list.count == 4);
        check("missing profile is skipped", usermaps_scan(&list, ".", "not-present") == 0 && list.count == 4);
        check("missing roots give empty list", usermaps_scan(&list, "not-present", "also-absent") == 0 && list.count == 0);
        check("repeated scan repopulates", usermaps_scan(&list, ".", argv[1]) == 0 && list.count == 7);
    }
    check("scan never changes working directory", getcwd(after, sizeof(after)) && !strcmp(before, after));
    usermaps_free(&list);
    check("free clears ownership", list.entries == NULL && list.count == 0);
    usermaps_free(&list);
    if (failures) return 1;
    puts("user map discovery: PASS");
    return 0;
}
