# Windows Saved Games Folder Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** On Windows, `duke3d.cfg` and saves live in `Saved Games\Duke Nukem 3D\`. Existing files in the game folder are copied there on first run.

**Architecture:**
- `setup_homedir()` in `source/global.c` gets a Windows branch that sets `ApogeePath`. Every per-user file already goes through `ApogeePath`.
- The Windows API lookup lives in a separate file, `source/win32_savedir.c`, so `<windows.h>` never meets the game's own typedefs.
- A Windows-only CTest checks the first-run copy using an empty GRP.

Spec: `docs/superpowers/specs/2026-10-19-windows-saved-games-design.md`.

**Tech Stack:** C99, Win32 (`SHGetKnownFolderPath`), CMake ≥ 3.24, CTest.

## Global Constraints

- **Where to work.** Only in the worktree `/Users/raynorpat/Projects/duke3d-64bit`, branch `windows-saved-games`. Never touch `/Users/raynorpat/Projects/duke3d`. Never push.
- **Commits.** One short line with a subsystem prefix, and no trailers or metadata.
- **Scope.** macOS and Linux behaviour is unchanged.
- **Folder order.**
  1. `DUKE3D_SAVE_DIR`, if set and not empty.
  2. `FOLDERID_SavedGames` via `SHGetKnownFolderPath`, converted with `WideCharToMultiByte(CP_ACP)`.
  3. `%USERPROFILE%\Saved Games`.
  4. Today's `.\`.

  `\Duke Nukem 3D\` is appended to the chosen base.
- **First-run copy.**
  - Files: `duke3d.cfg` and `game0.sav`–`game9.sav`.
  - A file is copied only when it exists in the working directory and not yet in the save folder.
  - The originals are kept.
  - A failed write deletes the partial copy.
- **Testing locally.** Windows can't be run here. Check compilation with `/opt/homebrew/bin/x86_64-w64-mingw32-gcc -fsyntax-only`; CI (MSVC) is the real check after the user pushes.
- **Search.** Use `/usr/bin/grep`.

---

### Task 1: Choose the Windows save folder and carry files over

**Files:**
- Create: `source/win32_savedir.c`, `source/win32_savedir.h`
- Modify: `source/global.c` (`setup_homedir`, around line 855; add a helper before it)
- Modify: `source/CMakeLists.txt` (WIN32: add the source, and link `shell32 ole32`)

**Interfaces:**
- Produces: `int win32_savedgames_dir(char *buf, size_t len);`. It returns 0 and writes the folder path with no trailing separator, or returns -1.

- [ ] **Step 1: Write `source/win32_savedir.h`**

```c
/*
 * The Windows "Saved Games" folder, kept apart from the game sources so
 * <windows.h> never meets their typedefs.
 */
#ifndef _INCL_WIN32_SAVEDIR_H_
#define _INCL_WIN32_SAVEDIR_H_

#include <stddef.h>

/* Writes the user's Saved Games folder, without a trailing separator, in
   the ANSI code page the game's file calls use. Returns 0, or -1 if it
   can't be found or represented. */
int win32_savedgames_dir(char *buf, size_t len);

#endif
```

- [ ] **Step 2: Write `source/win32_savedir.c`**

```c
/*
 * See win32_savedir.h.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <initguid.h>
#include <shlobj.h>
#include <knownfolders.h>
#include <stdio.h>
#include <stdlib.h>

#include "win32_savedir.h"

int win32_savedgames_dir(char *buf, size_t len)
{
    PWSTR wide = NULL;
    BOOL lossy = FALSE;
    const char *profile;
    int ok = 0;

    if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_SavedGames, 0, NULL, &wide)))
        ok = WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, wide, -1,
                                 buf, (int)len, NULL, &lossy) != 0 && !lossy;
    CoTaskMemFree(wide);
    if (ok)
        return 0;

    profile = getenv("USERPROFILE");
    if (profile != NULL && profile[0] != '\0' &&
        snprintf(buf, len, "%s\\Saved Games", profile) < (int)len)
        return 0;
    return -1;
}
```

- [ ] **Step 3: Add the Windows branch to `setup_homedir()`**

In `source/global.c`:
- Under the existing `#if` include area near the top, add `#if PLATFORM_WIN32` / `#include "win32_savedir.h"` / `#endif` after `#include "duke3d.h"`.
- Directly above `int setup_homedir (void)`, add the copy helper:

```c
#if PLATFORM_WIN32
/* Copies name from the working directory into ApogeePath unless it's
   already there. A failed write leaves no partial copy behind. */
static void carry_over(const char *name)
{
    char dst[MAX_PATH], buf[4096];
    FILE *in, *out;
    size_t n;
    int bad = 0;

    GetPathFromEnvironment(dst, sizeof(dst), name);
    if (_access(dst, 0) == 0) return;
    if ((in = fopen(name, "rb")) == NULL) return;
    if ((out = fopen(dst, "wb")) == NULL) { fclose(in); return; }
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        if (fwrite(buf, 1, n, out) != n) { bad = 1; break; }
    if (ferror(in)) bad = 1;
    fclose(in);
    if (fclose(out) != 0) bad = 1;
    if (bad) remove(dst);
}
#endif
```

Inside `setup_homedir()`, insert this branch before the final `#else` (whose body is `sprintf(ApogeePath, ".%s", PATH_SEP_STR);`):

```c
#elif PLATFORM_WIN32
    /* Saved Games\Duke Nukem 3D. DUKE3D_SAVE_DIR overrides the Saved Games
       folder so the tests keep their files out of the real profile. */
    static const char *carry[] =
    {
        SETUPFILENAME, "game0.sav", "game1.sav", "game2.sav", "game3.sav",
        "game4.sav", "game5.sav", "game6.sav", "game7.sav", "game8.sav", "game9.sav"
    };
    char base[MAX_PATH], dir[MAX_PATH];
    const char *env = getenv("DUKE3D_SAVE_DIR");
    int i;

    if (env != NULL && env[0] != '\0')
        snprintf(base, sizeof(base), "%s", env);
    else if (win32_savedgames_dir(base, sizeof(base)) != 0)
        base[0] = '\0';

    sprintf(ApogeePath, ".%s", PATH_SEP_STR);
    if (base[0] != '\0' && strlen(base) + sizeof("\\Duke Nukem 3D\\") <= sizeof(ApogeePath))
    {
        _mkdir(base);
        snprintf(dir, sizeof(dir), "%s\\Duke Nukem 3D", base);
        if (_mkdir(dir) == 0 || errno == EEXIST)
        {
            snprintf(ApogeePath, sizeof(ApogeePath), "%s\\", dir);
            for (i = 0; i < (int)(sizeof(carry)/sizeof(carry[0])); i++)
                carry_over(carry[i]);
        }
        else
            fprintf(stderr, "Couldn't create the save folder %s; using the game folder.\n", dir);
    }
```

The declarations sit at the start of the branch's block, which is the function body. That's fine in C99, and the code stays legal because nothing precedes them in that branch.

- [ ] **Step 4: Build glue**

In `source/CMakeLists.txt`, after the `target_link_libraries(duke3d_game PUBLIC ...)` call:

```cmake
# The Windows Saved Games folder (SHGetKnownFolderPath, CoTaskMemFree).
if(WIN32)
  target_sources(duke3d_game PRIVATE win32_savedir.c)
  target_link_libraries(duke3d_game PUBLIC shell32 ole32)
endif()
```

- [ ] **Step 5: Check it compiles for Windows**

```bash
cd /Users/raynorpat/Projects/duke3d-64bit && /opt/homebrew/bin/x86_64-w64-mingw32-gcc -std=c99 -Wall -fsyntax-only source/win32_savedir.c
/opt/homebrew/bin/x86_64-w64-mingw32-gcc -std=gnu99 -fsyntax-only -funsigned-char -DPLATFORM_WIN32=1 -DUSE_SDL=1 -DVOLUMEALL \
  -Isource -Isource/build -isystem /opt/homebrew/include/SDL2 source/global.c 2>&1 | /usr/bin/grep -E "error|carry_over|setup_homedir|win32_savedgames" | head -20
```

Expected:
- `win32_savedir.c` has no errors.
- `global.c` has no errors in the new code. Pre-existing unrelated MinGW errors elsewhere in the file are acceptable but must be noted.

If `global.c` can't be syntax-checked under MinGW at all, record why and rely on CI.

- [ ] **Step 6: Check macOS still builds and passes**

Run: `cmake --build build 2>&1 | tail -1 && ctest --test-dir build 2>&1 | tail -2`

Expected: 3/3 pass.

- [ ] **Step 7: Commit**

```bash
git add source/win32_savedir.c source/win32_savedir.h source/global.c source/CMakeLists.txt
git commit -m "game: keep config and saves in the Windows Saved Games folder"
```

### Task 2: Tests

**Files:**
- Create: `tests/data/empty.grp` (16 bytes), `tests/save_dir.cmake`
- Modify: `tests/CMakeLists.txt`, `tests/game_boot.cmake`, `tests/dos_save.cmake`

- [ ] **Step 1: The empty GRP**

```bash
cd /Users/raynorpat/Projects/duke3d-64bit && mkdir -p tests/data && printf 'KenSilverman\0\0\0\0' > tests/data/empty.grp && xxd tests/data/empty.grp
```

Expected: `4b656e53696c7665726d616e00000000`, 16 bytes.

- [ ] **Step 2: Check, on macOS, that an empty GRP gets startup as far as `setup_homedir()`**

The macOS branch copies `duke3d.cfg` the same way the Windows branch will, so it proves the run reaches the copy.

```bash
T=$(mktemp -d) && mkdir -p $T/run $T/home && cp tests/data/empty.grp $T/run/duke3d.grp && echo marker > $T/run/duke3d.cfg
(cd $T/run && HOME=$T/home SDL_VIDEODRIVER=dummy BUILD_WINDOWED=1 /Users/raynorpat/Projects/duke3d-64bit/build/bin/duke3d </dev/null >/dev/null 2>&1 & P=$!; sleep 8; kill $P 2>/dev/null)
ls "$T/home/Library/Application Support/Duke Nukem 3D/"; rm -rf $T
```

Expected: `duke3d.cfg` is listed. If it isn't, stop: the Windows test design doesn't hold.

- [ ] **Step 3: Write `tests/save_dir.cmake`**

```cmake
# Windows: on first run the game copies duke3d.cfg and game*.sav from its
# working directory into the save folder. An empty GRP gets startup far
# enough; whatever the game does after that doesn't matter.
#
#   cmake -DDUKE3D=<exe> -DEMPTY_GRP=<empty.grp> -DWORK=<scratch dir>
#         -P save_dir.cmake

foreach(_var DUKE3D EMPTY_GRP WORK)
  if(NOT DEFINED ${_var})
    message(FATAL_ERROR "save_dir: ${_var} is not set")
  endif()
endforeach()

set(_run "${WORK}/run")
set(_saves "${WORK}/saves")
file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${_run}")
file(COPY_FILE "${EMPTY_GRP}" "${_run}/duke3d.grp")
file(WRITE "${_run}/duke3d.cfg" "save_dir marker\n")
file(WRITE "${_run}/game3.sav" "save_dir marker 3\n")

set(ENV{SDL_VIDEODRIVER} dummy)
set(ENV{BUILD_WINDOWED} 1)
set(ENV{DUKE3D_SAVE_DIR} "${_saves}")
execute_process(COMMAND "${DUKE3D}"
  WORKING_DIRECTORY "${_run}"
  TIMEOUT 15
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _out
  ERROR_VARIABLE _out)
message(STATUS "duke3d: ${_result}\n${_out}")

set(_dir "${_saves}/Duke Nukem 3D")
if(NOT EXISTS "${_dir}/duke3d.cfg")
  message(FATAL_ERROR "duke3d.cfg wasn't copied to ${_dir}")
endif()
if(NOT EXISTS "${_dir}/game3.sav")
  message(FATAL_ERROR "game3.sav wasn't copied to ${_dir}")
endif()
# The game never writes saves at startup, so the copy must be exact.
file(READ "${_dir}/game3.sav" _got)
if(NOT _got STREQUAL "save_dir marker 3\n")
  message(FATAL_ERROR "game3.sav was copied wrong: '${_got}'")
endif()
foreach(_f duke3d.cfg game3.sav)
  if(NOT EXISTS "${_run}/${_f}")
    message(FATAL_ERROR "the original ${_f} is gone")
  endif()
endforeach()
message(STATUS "save_dir passed")
```

- [ ] **Step 4: Register it and keep the other tests out of the real profile**

In `tests/CMakeLists.txt`:
- Change the Windows `_cfg_dir` from `"."` to `"Duke Nukem 3D"`.
- Add after the `sdl_driver_smoke` block:

```cmake
# Windows: config and saves go to Saved Games\Duke Nukem 3D, and existing
# ones in the game folder are copied there. Needs no game data.
if(WIN32)
  add_test(NAME save_dir
    COMMAND "${CMAKE_COMMAND}"
            "-DDUKE3D=$<TARGET_FILE:duke3d>"
            "-DEMPTY_GRP=${CMAKE_CURRENT_SOURCE_DIR}/data/empty.grp"
            "-DWORK=${CMAKE_BINARY_DIR}/sd"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/save_dir.cmake")
  set_tests_properties(save_dir PROPERTIES TIMEOUT 60)
endif()
```

In `tests/game_boot.cmake` and `tests/dos_save.cmake`, next to `set(ENV{USERPROFILE} "${_home}")`, add:

```cmake
# Windows keeps config and saves in Saved Games; point it at the scratch home.
set(ENV{DUKE3D_SAVE_DIR} "${_home}")
```

In `game_boot.cmake`, the existing `CFG_DIR` handling already resolves `"${_home}/${CFG_DIR}/duke3d.cfg"`, so `"Duke Nukem 3D"` lands on `<home>/Duke Nukem 3D/duke3d.cfg`. Update the header comment's description of `CFG_DIR` if it mentions the working directory.

- [ ] **Step 5: Run the macOS suites**

```bash
cd /Users/raynorpat/Projects/duke3d-64bit && cmake -S . -B build >/dev/null && ctest --test-dir build 2>&1 | tail -2
cmake -S . -B build-grp >/dev/null && cmake --build build-grp 2>&1 | tail -1 && ctest --test-dir build-grp -j4 2>&1 | tail -2
```

Expected:
- `build`: 3/3.
- `build-grp`: 49/49.
- `save_dir` isn't registered on macOS.

- [ ] **Step 6: Commit**

```bash
git add tests/data/empty.grp tests/save_dir.cmake tests/CMakeLists.txt tests/game_boot.cmake tests/dos_save.cmake
git commit -m "tests: check the Windows save folder and keep tests out of it"
```

### Task 3: README

**Files:**
- Modify: `README.md` (the "Without a `duke3d.cfg`" paragraph, around line 48)

- [ ] **Step 1: Update the paragraph.**
  - Replace `and the working directory on Windows` with `and \`Saved Games\Duke Nukem 3D\` on Windows`.
  - After that sentence, add: `On Windows, an existing \`duke3d.cfg\` and \`game*.sav\` in the game folder are copied there on first run.`
  - Rewrap at about 80 columns.

- [ ] **Step 2: Commit**

```bash
git add README.md
git commit -m "docs: say Windows keeps config and saves in Saved Games"
```

### After all tasks

- The user pushes; CI (MSVC) builds `win32_savedir.c` and runs `save_dir`.
- If a Windows machine is available, start the game once. `duke3d.cfg` and any saves should appear in `Saved Games\Duke Nukem 3D`.
