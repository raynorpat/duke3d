#ifndef DUKE3D_USERMAPS_H
#define DUKE3D_USERMAPS_H

#include <stddef.h>

typedef struct { char *name, *path; int source; } usermap_entry;
typedef struct { usermap_entry *entries; size_t count; } usermap_list;

enum { USERMAP_READY = 0, USERMAP_PATH_TOO_LONG = -1, USERMAP_UNREADABLE = -2 };

/* Lists own their strings; initialize to zero before the first scan. */
int usermaps_scan(usermap_list *list, const char *current_dir, const char *profile_dir);
void usermaps_free(usermap_list *list);
const char *usermap_source(int source);
int usermap_check(const usermap_entry *entry, size_t path_capacity);

#endif
