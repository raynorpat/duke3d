# 64-bit Cleanup (Core) — Design

Date: 2026-10-19
Status: Approved (brainstorming)

This is sub-project 1 of 3 for running duke3d on 64-bit targets. It follows the CMake +
SDL2 work (`2026-10-18-cmake-sdl2-design.md`).

1. **Core 64-bit cleanup** (this spec)
2. Demos, the build editor's map save path, and network play on 64-bit (separate spec)
3. Loading original DOS `.SAV` files (separate spec)

## Goal

`duke3d` boots, renders and plays single-player correctly as a 64-bit binary:

- on macOS arm64, verified locally with the user's GRP;
- on Linux x86_64 and Windows x64, built and tested in CI without game data.

Saves made by the 64-bit build load back correctly on every OS. Maps, ART, CON, GRP, RTS,
palette and lookup files keep working byte-for-byte.

## Scope

In scope:

1. Mechanical retype of `long` to fixed-width types across the engine, game and audiolib.
2. Pointer-width types for every runtime address.
3. CON script addresses stored as 32-bit byte offsets into `script[]`.
4. Compile-time format invariants and pointer-truncation errors.
5. A default configuration when `duke3d.cfg` is missing.
6. The four SDL driver fixes deferred from the previous spec that become reachable once
   the game boots.
7. Tests: `con_compile`, `game_boot`, and the existing `sdl_driver_smoke` (now also on
   Windows).

Out of scope:

- Demo recording/playback, editor map saving, and network play (sub-project 2). They
  must still compile; they aren't verified.
- Reading DOS saves (sub-project 3). Saved structs stay DOS-sized so that work remains
  straightforward.
- New features, refactors unrelated to word size, and the pre-existing dead
  `button != 4 && button != 5` branch in `sdl_driver.c` (kept for fidelity).

## Design

### 1. Mechanical retype

A committed script, `tools/retype_long.py`, rewrites C sources in place. It runs over
`source/build/*.[ch]`, `source/*.[ch]` and `source/audiolib/*.[ch]`:

- `long`, `signed long` and `long int` → `int32_t`
- `unsigned long` and `unsigned long int` → `uint32_t`
- `long long` and its signed/unsigned forms are left unchanged.
- Comments and string/character literals are left unchanged.

After the script runs:

- `#include <stdint.h>` is added once to `source/build/platform.h`, which every
  translation unit already reaches.
- `source/types.h` `int32`/`uint32` (and any other `long`-based typedefs there) become
  `int32_t`/`uint32_t`.
- C-library functions whose prototypes use `long` (`strtol`, `atol`, `labs`, `ftell`,
  `lseek` and similar) keep their system declarations. Results are cast at call sites
  where the compiler requires it. The script never edits system headers.
- `printf`-family format specifiers for retyped values change from `%ld`/`%lu`/`%lx` to
  `%d`/`%u`/`%x`, wherever `-Wformat` reports them.

A prototype of the retype on 2026-10-19 compiled with zero errors. It left 208 unique
compiler-reported pointer sites (95 `-Wint-to-pointer-cast`, 77 `-Wpointer-to-int-cast`,
36 `-Wshorten-64-to-32`). By file: `a.c` 55, `engine.c` 42, `menues.c` 34, `build.c` 13,
`gamedef.c` 12, `sdl_driver.c` 12, `cache1d.c` 11, `multivoc.c` 7, and the rest under 7
each. These sites are the work list for sections 2 and 3.

### 2. Runtime addresses → `intptr_t` / `uintptr_t`

Every variable, array element, struct member, parameter or return value that holds a
memory address gets a pointer-width type. None of these is ever written to a file. Known
members of this class:

- `frameplace`, `FP_OFF()`, and the `screen`/`horizlookup` buffer arithmetic in the engine
  and the SDL driver
- `waloff[]` (tile buffers)
- The address-carrying parameters and locals of the C replacements for Ken's assembly
  (`a.c`, `pragmas.h`, `pragmas.c`): `bufplc`, `p`, `palookupoffse`, `globalbufplc` and
  their equivalents
- `palookup` offsets and the `cache1d.c` `allocache` handles
- audiolib `multivoc.c` voice buffer addresses
- Any further site the compiler reports

`FP_OFF(x)` is redefined as `((intptr_t)(x))` in `unix_compat.h`, `win32_compat.h` and
`dukewin.h`.

### 3. CON script addresses → 32-bit byte offsets

Today the CON compiler (`gamedef.c`) and the actor code store absolute addresses into
32-bit data:

- `script[]` (jump and label targets)
- `labelcode[]`
- `actorscrptr[]`
- `hittype[].temp_data` slots `t[1]` (move), `t[4]` (action) and `t[5]` (AI)

They are dereferenced with 4-byte arithmetic such as `*(long *)(g_t[4]+16)`.

All of these instead store **byte offsets from `script`**, kept in `int32_t`. `duke3d.h`
defines:

```c
#define SCRIPT_PTR(off) ((int32_t *)((char *)script + (off)))
#define SCRIPT_OFF(ptr) ((int32_t)((char *)(ptr) - (char *)script))
```

Rules:

- Every store of a script address uses `SCRIPT_OFF(...)`, and every dereference of a
  stored script address uses `SCRIPT_PTR(...)`. For example, `*(long *)(g_t[4]+16)`
  becomes `*SCRIPT_PTR(g_t[4]+16)`.
- `actorscrptr[]` changes from `long *` to `int32_t`. A value of `0` still means "no
  script", as before.
- The interpreter's live pointers (`insptr`, `scriptptr`, `moveptr`) stay real
  `int32_t *` pointers. They are converted only at the store and load boundaries above.
- `script[]` stays an `int32_t` array, so each element stays 4 bytes and the
  `MAXSCRIPTSIZE` size is unchanged.
- The savegame code in `menues.c` that converts script pointers to offsets on write and
  back on read is deleted, because the values are already offsets. The on-disk bytes for
  these fields are unchanged.

### 4. Format invariants

These `_Static_assert` checks live next to the type definitions (`build.h` and
`duke3d.h`):

- `sizeof(sectortype) == 40`, `sizeof(walltype) == 32`, `sizeof(spritetype) == 44`
- `sizeof(script[0]) == 4`
- `sizeof(struct weaponhit)` and `sizeof(struct animwalltype)` equal their 32-bit sizes,
  computed from the field lists and asserted as literal numbers.

New build flags:

- GCC/Clang: `-Werror=pointer-to-int-cast` and `-Werror=int-to-pointer-cast`, added to
  `duke3d_compile_options` through the existing supported-flag check.
- MSVC: `/we4311 /we4312 /we4302`.

### 5. Default configuration

When `duke3d.cfg` is missing, `CONFIG_ReadSetup` no longer calls `Error()`. Instead it:

1. calls `CONFIG_SetDefaults()`;
2. creates an empty script with `SCRIPT_Init(setupfilename)`;
3. maps keys from `keydefaults[]`, mouse buttons from `mousedefaults[]` and joystick
   buttons from `joystickdefaults[]` (all in `_functio.h`), using the same calls the
   loader uses for parsed values;
4. continues normally, so `CONFIG_WriteSetup` writes a complete `duke3d.cfg` on exit.

`setupfilename` grows from 128 bytes to `MAX_PATH`, because a long macOS home path
overflowed it.

### 6. Deferred SDL driver fixes (`source/build/sdl_driver.c`)

1. `cleanup_vesa_modelist` drops modes whose width isn't a multiple of 4. This removes the
   `assert(surface->pitch == surface->w)` abort and the skewed rendering.
2. `sdl_mouse_wheel_filter` negates `y` when `event->direction ==
   SDL_MOUSEWHEEL_FLIPPED`.
3. `go_to_new_vid_mode` calls `SDL_SetWindowSize` even while fullscreen, so toggling back
   to windowed restores the new mode's size.
4. `go_to_new_vid_mode` calls `SDL_InitSubSystem(SDL_INIT_VIDEO)` only when
   `SDL_WasInit(SDL_INIT_VIDEO)` is 0.

Each fix gets a line in `source/build/VENDORED.md`.

### 7. Windows

Once every address is pointer-width or a script offset, LLP64 needs nothing extra.
`tests/CMakeLists.txt` registers `sdl_driver_smoke` on Windows too; the `if(NOT WIN32)`
guard is removed.

## Testing

1. **Compile-time:** the `-Werror` flags and `_Static_assert` checks from section 4.
2. **`sdl_driver_smoke`** (existing): passes on all three OSes.
3. **`con_compile`** (new CTest, no game data needed, runs in CI):
   - compiles the in-repo `testdata/game.con`, `defs.con` and `user.con` with the game's
     real CON compiler (`gamedef.c`);
   - asserts every stored script address is a byte offset inside the compiled script
     `[0, scriptsize * 4)`: jump and label targets in `script[]`, `labelcode[]`, and every
     non-zero `actorscrptr[]`;
   - asserts that at least one actor got a script, so the test can't pass vacuously.
4. **`game_boot`** (new CTest, CMake `-P` script):
   - Registered only when the configure-time cache variable `DUKE3D_GRP` names an
     existing GRP; otherwise not registered, which is the case in CI.
   - Creates a scratch run directory containing a symlink (or copy on Windows) to the GRP
     and a scratch `HOME`/`USERPROFILE`.
   - Runs `duke3d` twice with `SDL_VIDEODRIVER=dummy`, `BUILD_WINDOWED=1` and
     `execute_process(... TIMEOUT 15)`: once with no arguments (title/demo loop), and
     once with `-v1 -l1 -s2` (Episode 1 Level 1, skill 2).
   - Passes when each run is still alive at the timeout, meaning `execute_process`
     reports a timeout rather than an exit code. Fails on any exit, crash, or output
     containing `Not enough memory`.
   - After the runs, also checks that `duke3d.cfg` was created in the scratch config
     directory.
5. **Manual (macOS, real window):**
   - The controller verifies the title screen, menus and palette fades with window-only
     screenshots.
   - The user verifies at the keyboard:
     - starting E1L1;
     - walking, shooting, opening a door and riding an elevator;
     - finishing the level so E1L2 loads;
     - saving, loading the save in the same session, then loading it again after a
       restart;
     - changing a key binding and confirming it persists after a restart;
     - toggling fullscreen with Alt+Enter;
     - the mouse wheel cycling weapons in the expected direction.

## Definition of done

1. macOS arm64: `cmake --preset default && cmake --build --preset default` builds with zero
   errors, including the new `-Werror` flags and static asserts.
2. `ctest --preset default` passes locally with `-DDUKE3D_GRP=<path to DUKE3D.GRP>`:
   `sdl_driver_smoke`, `con_compile` and `game_boot`.
3. The manual checklist in Testing step 5 passes.
4. When the user pushes, all three CI jobs build and pass `ctest`. `game_boot` is not
   registered in CI.
5. `source/build/VENDORED.md` lists every engine change. `README.md`'s "Known issue"
   paragraph is replaced with a note about what is now supported and what sub-projects 2
   and 3 will add.

## Risks

- **Pointer sites the compiler can't see.** These are addresses stored through `memcpy`,
  unions, or arithmetic without casts. Mitigation: the `game_boot` test, the manual
  playtest, and `-Wshorten-64-to-32` review of every remaining truncation warning.
- **Arithmetic that silently relied on 64-bit `long`** in the icculus-era code. This is
  unlikely, because the code was written for 32-bit `long`. Retyping restores the
  original semantics.
- **CON offset conversion missing a site** shows up as a crash or misbehaving actor
  during play. Mitigation: grep for every `(long *)`/`(int32_t *)` cast applied to `t[`,
  `T[1-6]`, `temp_data`, `actorscrptr`, `labelcode` or `script` values, and route each
  one through the macros.
- **Large diff in vendored code.** Mitigation: the retype is a separate commit produced
  by the committed script, so reviewers can re-run it and diff. Hand fixes go in
  separate commits.
