# User Maps Menu Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Select and launch a custom single-player map from a permanent USER MAPS episode-menu entry.

**Architecture:** An independent C scanner supplies a combined, sorted list of map filenames, source labels, and load paths. The existing menu handler renders that list and passes the selected path through the existing difficulty and custom-map loading flow. Profile discovery uses `GetPathFromEnvironment`, not a new OS-specific config-location policy.

**Tech Stack:** C99, existing SDL2/Build menus, POSIX/Windows file enumeration, CMake/CTest.

**Spec:** `docs/superpowers/specs/2026-10-23-user-maps-menu-design.md`

## Global Constraints

- Scan the current working directory, its `maps/`, the existing user config/save directory, and its `maps/`, without recursion.
- Preserve the 128-byte `boardfilename` field and save/demo formats.
- Expose the feature in registered/full versions (1.3D, 1.4, 1.5), preserving the existing shareware restrictions.
- Leave the multiplayer menu unchanged; do not change the process working directory.
- Verification must use temporary profile folders, not write to the user's real config or map files.
- Keep existing artwork, fonts, sounds, input conventions, and normal difficulty behavior; no new dependencies.
- Commits use short `menu:`, `tests:`, or `docs:` messages with no metadata.

## Review Focus

- Duplicate names from different sources must launch the chosen file; Task 1 pins paths and Task 2 pins the selection.
- Profile and working directory may coincide or alias each other; Task 1 tests overlapping roots, including a symlink where supported.
- A file can disappear between list selection and skill confirmation; Task 2 tests both points without starting a game.
- Empty lists and repeated cancel/reopen sequences must not change the active custom map; Task 2 tests stale-selection cleanup.
- Very long paths and hundreds of maps must remain safe and navigable; Task 1 tests storage/boundaries and Task 2 tests scrolling and clipping.

## Files and responsibilities

- Create `source/usermaps.h` and `source/usermaps.c`: directory enumeration, ordering, path identity, ownership, and selection checks; no drawing or game-state mutations.
- Modify `source/menues.c`: permanent episode entry, scrolling picker, pending selection, and back navigation.
- Modify `source/CMakeLists.txt`: compile the scanner into `duke3d_game`.
- Create `tests/usermaps.c` and `tests/usermaps.cmake`: scanner assertions and disposable cross-platform fixtures.
- Create `tests/usermaps_menu.c` and `tests/usermaps_menu.cmake`: real menu-handler navigation tests using the pattern in `tests/v099_menus.c`, with a disposable profile/data fixture wrapper.
- Modify `tests/CMakeLists.txt`: register tests and their headless SDL environment.
- Modify `README.md`: entry point, four locations, full-version scope, and path limit.

### Task 1: Discover and validate maps from four sources

**Interfaces:**

- `source/usermaps.h` defines `usermap_entry { char *name; char *path; int source; }` and `usermap_list { usermap_entry *entries; size_t count; }`.
- `int usermaps_scan(usermap_list *list, const char *current_dir, const char *profile_dir)` replaces a zero-initialized or previously populated list; returns 0 on success and -1 on allocation failure, leaving an empty list on failure.
- `void usermaps_free(usermap_list *list)` releases owned strings/storage and resets the list.
- `const char *usermap_source(int source)` returns CURRENT, CURRENT / MAPS, PROFILE, PROFILE / MAPS for source indices 0 through 3.
- `int usermap_check(const usermap_entry *entry, size_t path_capacity)` returns `USERMAP_READY` (0), `USERMAP_PATH_TOO_LONG` (-1), or `USERMAP_UNREADABLE` (-2). Check length including the terminator, then regular-file/readability status.

- [ ] **Step 1: Add failing scanner tests and fixture registration.**

  `tests/usermaps.cmake` creates disposable current/profile roots, mixed-case extensions, same-name files in all four locations, a directory ending in `.map`, ignored nested files, a filename longer than 16 characters, and 300 map files. `tests/usermaps.c` calls the declared API and asserts all four source/path pairs, case-insensitive name ordering with source-index tie breaking, regular-file filtering, no recursion, and an unchanged working directory. Add cases for absent roots, identical roots, equivalent paths, and directory symlinks where the host permits creating them. Check a path of 127 bytes is accepted with capacity 128 when readable; a path of 128 bytes returns USERMAP_PATH_TOO_LONG. Remove a scanned file and expect USERMAP_UNREADABLE. Repeated scans/free must reset state.

- [ ] **Step 2: Confirm the tests fail before implementation.**

  Run `cmake -S . -B build-user-maps -DDUKE3D_FETCH_SDL=OFF`, then `cmake --build build-user-maps --target usermaps_test`. Expect missing scanner declarations/definitions; record the failure before adding implementation.

- [ ] **Step 3: Implement the scanner and build wiring.**

  Implement the interfaces above in `source/usermaps.c`, using POSIX directory APIs on Unix and `_findfirst`/`_findnext`/`_findclose` on Windows. Store full strings dynamically, enumerate without `chdir`, and compare canonical directory identities before scanning to avoid duplicate roots. Preserve distinct files with matching names. Keep CURRENT paths relative when the input root is `.`, PROFILE paths absolute, and original filename spelling. Skip missing/inaccessible directories; free all handles/storage on every exit. Connect the module to the game and standalone test targets.

- [ ] **Step 4: Run the scanner tests.**

  Run `cmake --build build-user-maps --target usermaps_test` and `ctest --test-dir build-user-maps -R '^usermaps$' --output-on-failure`. Expect zero failures, including the 300-file and path-boundary cases.

- [ ] **Step 5: Commit the verified scanner and its tests.**

  Stage only Task 1 files; commit as `menu: discover custom maps in game and profile folders`.

### Task 2: Connect the episode menu, picker, and difficulty flow

**Interfaces:**

- Consumes Task 1's `usermaps_scan`, `usermaps_free`, `usermap_source`, and `usermap_check`.
- Existing public `void cmenu(short cm)` and `void menus(void)` remain unchanged.
- Reserve menu 120 for the single-player picker (currently unused). Keep pending-map state private in `menues.c`; never change serialized structures.

- [ ] **Step 1: Add failing real-handler navigation tests.**

  Register `usermaps_menu` linked with `duke3d_game` and the existing renamed-main `game.c` test pattern. Use dummy SDL video/audio and temporary HOME/USERPROFILE/DUKE3D_SAVE_DIR values from `tests/usermaps_menu.cmake`; call `setup_homedir()` only after those overrides apply. Drive `menus()` with actual key state as `v099_menus.c` does. Assert episode row 3 for 1.3D or row 4 for 1.4/1.5 opens menu 120 even with empty `boardfilename`; selecting a map opens 110 with volume 0/level 7; Escape returns to the same picker row and then the USER MAPS episode row. Assert empty-list Enter stays in the picker, 300 entries scroll safely, cancel/reopen does not commit a map, stock selection clears a stale custom-map selection, and deleting a selected file before skill confirmation returns a menu error instead of entering a level. Overlong selection must stay in the picker without truncation. In shareware builds assert the existing episode restrictions and no new selectable entry.

- [ ] **Step 2: Verify the navigation tests fail.**

  Run `cmake -S . -B build-user-maps -DDUKE3D_FETCH_SDL=OFF`, `cmake --build build-user-maps --target usermaps_menu`, and `ctest --test-dir build-user-maps -R '^usermaps_menu$' --output-on-failure`. Expect the permanent-entry/navigation assertions to fail against the old menu.

- [ ] **Step 3: Implement the menu and document its behavior.**

  In `menues.c` replace the conditional full-version USER MAP row with permanent USER MAPS. On opening menu 120 resolve PROFILE with `GetPathFromEnvironment`, scan, and render a vertically scrolling filename/source list at 320x200 coordinates with selected path, position/total, and navigation hints. Clip drawn strings only. Use existing input/sound conventions; the new picker's index/count must not inherit the old byte-sized fields. Retain selection on return from difficulty. Set pending selection on successful map choice; recheck it before skill confirmation, then copy its path to `boardfilename` and use the existing single-player start flow. Clear pending state on stock selection and when leaving the picker. Display NO MAPS FOUND, MAP PATH TOO LONG, and MAP NOT AVAILABLE as appropriate; report allocation failure in the picker. Keep shareware and multiplayer branches intact. Add the README instructions.

- [ ] **Step 4: Verify focused tests, version variants, and the real menu.**

  Run `cmake --build build-user-maps` and `ctest --test-dir build-user-maps -R '^(usermaps|usermaps_menu|game_helpers|savegame_codec|version_macros|input_polling|gamepad)$' --output-on-failure`; expect zero failures. Repeat the new tests and game build in separate `build-user-maps-v13`, `build-user-maps-v14`, and `build-user-maps-v13sw` directories configured with `-DDUKE3D_VERSION=1.3D`, `1.4`, and `1.3D -DDUKE3D_SHAREWARE=ON`, respectively. Build a `0.99` shareware variant and run `v099_menus` to guard the unchanged historical menu. On available hosts run the same tests on Linux/Windows; otherwise report these as unverified. With matching local game data, use disposable data/profile folders to open the real menu, launch a map from each source (including duplicate basenames), and confirm difficulty, back navigation, and long-text rendering. Record any data-dependent checks that cannot be performed.

- [ ] **Step 5: Review and commit the verified menu.**

  Review the final diff against the spec, paying particular attention to exact-source loading, pending state, input handling, path bounds, and compile-time version branches. Run `git diff --check`, then stage only Task 2 files and commit as `menu: launch custom maps from episode selection`. Report commands/results and platform/data limitations with the final change.
