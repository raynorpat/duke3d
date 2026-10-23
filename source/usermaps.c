/* Loose single-player maps; scanning never changes the process directory. */
#include "usermaps.h"
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <direct.h>
#define stat _stat
#define S_ISREG(mode) (((mode) & _S_IFMT) == _S_IFREG)
#else
#include <dirent.h>
#endif

typedef struct { uint64_t device, file; } directory_id;

static int directory_identity(const char *path, directory_id *id)
{
#ifdef _WIN32
    BY_HANDLE_FILE_INFORMATION info;
    HANDLE handle = CreateFileA(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE |
        FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    int ok;
    if (handle == INVALID_HANDLE_VALUE) return 0;
    ok = GetFileInformationByHandle(handle, &info) && (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    CloseHandle(handle);
    if (!ok) return 0;
    id->device = info.dwVolumeSerialNumber;
    id->file = ((uint64_t)info.nFileIndexHigh << 32) | info.nFileIndexLow;
#else
    struct stat info;
    if (stat(path, &info) || !S_ISDIR(info.st_mode)) return 0;
    id->device = info.st_dev;
    id->file = info.st_ino;
#endif
    return 1;
}

static char *joinpath(const char *dir, const char *name)
{
    size_t len;
    char *path;
    if (!strcmp(dir, ".")) dir = "";
    len = strlen(dir);
    path = malloc(len + strlen(name) + 2);
    if (path) snprintf(path, len + strlen(name) + 2, "%s%s%s", dir,
        len && dir[len-1] != '/' && dir[len-1] != '\\' ? "/" : "", name);
    return path;
}

static int namecompare(const char *a, const char *b)
{
    while (*a && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { a++; b++; }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static int entrycompare(const void *va, const void *vb)
{
    const usermap_entry *a = va, *b = vb;
    int order = namecompare(a->name, b->name);
    if (!order) order = a->source - b->source;
    return order ? order : strcmp(a->name, b->name);
}

static int addmap(usermap_list *list, const char *dir, const char *name, int source)
{
    size_t len = strlen(name);
    struct stat info;
    char *path, *copy;
    usermap_entry *entries;
    if (len < 5 || namecompare(name + len - 4, ".map")) return 0;
    path = joinpath(dir, name);
    if (!path) return -1;
    if (stat(path, &info) || !S_ISREG(info.st_mode)) { free(path); return 0; }
    copy = malloc(len + 1);
    if (!copy) { free(path); return -1; }
    memcpy(copy, name, len + 1);
    entries = realloc(list->entries, (list->count + 1) * sizeof(*entries));
    if (!entries) { free(path); free(copy); return -1; }
    list->entries = entries;
    entries[list->count].name = copy;
    entries[list->count].path = path;
    entries[list->count].source = source;
    list->count++;
    return 0;
}

static int scandirectory(usermap_list *list, const char *path, int source)
{
    int result = 0;
#ifdef _WIN32
    struct _finddata_t entry;
    intptr_t handle;
    char *pattern = joinpath(path, "*");
    if (!pattern) return -1;
    handle = _findfirst(pattern, &entry);
    free(pattern);
    if (handle == -1) return 0;
    do {
        result = addmap(list, path, entry.name, source);
    } while (!result && !_findnext(handle, &entry));
    _findclose(handle);
#else
    struct dirent *entry;
    DIR *dir = opendir(path);
    if (!dir) return 0;
    while ((entry = readdir(dir)) != NULL)
        if ((result = addmap(list, path, entry->d_name, source)) != 0) break;
    closedir(dir);
#endif
    return result;
}

void usermaps_free(usermap_list *list)
{
    size_t i;
    for (i = 0; i < list->count; i++) {
        free(list->entries[i].name);
        free(list->entries[i].path);
    }
    free(list->entries);
    list->entries = NULL;
    list->count = 0;
}

int usermaps_scan(usermap_list *list, const char *current_dir, const char *profile_dir)
{
    char *roots[4] = {NULL, NULL, NULL, NULL};
    directory_id seen[4], id;
    int i, j, count = 0, result = 0;
    usermaps_free(list);
    roots[0] = joinpath(current_dir, "");
    roots[1] = joinpath(current_dir, "maps");
    errno = 0;
#ifdef _WIN32
    roots[2] = _fullpath(NULL, profile_dir, 0);
    if (roots[2]) {
        char *p;
        for (p = roots[2]; *p; p++) if (*p == '\\') *p = '/';
    }
#else
    roots[2] = realpath(profile_dir, NULL);
#endif
    if (!roots[2] && errno == ENOMEM) result = -1;
    /* A missing profile is allowed, just like a missing maps folder. */
    if (roots[2]) roots[3] = joinpath(roots[2], "maps");
    if (!roots[0] || !roots[1] || (roots[2] && !roots[3])) result = -1;
    for (i = 0; i < 4 && !result; i++) {
        const char *root = roots[i];
        if (!root) continue;
        if (!root[0]) root = ".";
        if (!directory_identity(root, &id)) continue;
        for (j = 0; j < count; j++)
            if (seen[j].device == id.device && seen[j].file == id.file) break;
        if (j != count) continue;
        seen[count++] = id;
        result = scandirectory(list, root, i);
    }
    for (i = 0; i < 4; i++) free(roots[i]);
    if (result) usermaps_free(list);
    else if (list->count > 1) qsort(list->entries, list->count, sizeof(*list->entries), entrycompare);
    return result;
}

const char *usermap_source(int source)
{
    static const char *labels[] = {"CURRENT", "CURRENT / MAPS", "PROFILE", "PROFILE / MAPS"};
    return labels[source];
}

int usermap_check(const usermap_entry *entry, size_t path_capacity)
{
    struct stat info;
    FILE *file;
    if (strlen(entry->path) >= path_capacity) return USERMAP_PATH_TOO_LONG;
    if (stat(entry->path, &info) || !S_ISREG(info.st_mode)) return USERMAP_UNREADABLE;
    file = fopen(entry->path, "rb");
    if (!file) return USERMAP_UNREADABLE;
    fclose(file);
    return USERMAP_READY;
}
