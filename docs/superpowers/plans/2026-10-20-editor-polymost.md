# Polymost in the Build Editor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** The map editor's 3D mode runs in renderers 0–3, configured by a new `build.cfg` that also replaces `setup.dat`.

**Architecture:** A small, engine-free `buildcfg.c` module reads, clamps and writes `build.cfg` (and migrates `setup.dat` keys once); it is unit-tested on its own. The engine gains a fullscreen setter/getter. The editor (`build.c`, `astub.c`) uses a copy of the game's `setgamemoderenderer` pattern at start and on leaving 2D, fixes the GL crash sites, and adds the polish items.

**Tech Stack:** C99, SDL2, legacy OpenGL via Polymost, CMake/CTest.

**Spec:** `docs/superpowers/specs/2026-10-20-editor-polymost-design.md`

## Global Constraints

- Renderers numbered 0 classic, 1 Polymost solid, 2 Polymost software, 3 Polymost OpenGL; 2D mode stays 640x480 16-colour software.
- `build.cfg` lives in the same folder as `duke3d.cfg` (`setup_homedir` logic in `source/global.c`); plain `key = value`, `;` comments, keys: `renderer` (0–3, default 0), `xdim3d`/`ydim3d` (default 640x480), `fullscreen` (0/1, default 1), `gltexmiplevel` (0–15, default 0), `mousesensitivity` (decimal, default 1.0), and the 19 key bindings in hex under JFBuild's names (see Task 1).
- `BUILD_WINDOWED` env var forces a window and wins over `build.cfg`.
- `-renderer N` overrides for the run and is not written back. The configured renderer is kept separate from the one in use; a GL fallback is not saved. Alt+K's cycled renderer is saved.
- The game (`duke3d`) must not change behaviour. Classic editor changes only: 3D size from `build.cfg`, time-scaled look/turn. Saved maps byte-identical for the same edits.
- With `DUKE3D_POLYMOST=OFF`, renderers 1–3 fall back to 0 and everything else works.
- 64-bit conventions (int32_t / intptr_t). Commits: AGENTS.md (short, subsystem-prefixed, one or two lines, no metadata) plus the `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>` trailer.
- Never run the editor/game inside `~/Downloads/DUKE3D`; symlink the data into a scratch dir.

## Review Focus

1. A hand-edited or corrupt `build.cfg` (garbage values, missing `=`, huge numbers, CRLF line ends, unknown keys) → clamped defaults, no crash; the file is rewritten cleanly on exit. Test in Task 1.
2. `setup.dat` shorter than 27 bytes or garbage → defaults for whatever's missing, no crash. Test in Task 1.
3. `renderer = 3` with GL unavailable (dummy video driver) → editor falls back to classic, `build.cfg` keeps 3. Test in Task 3 (mode-helper test under the dummy driver).
4. Repeated 2D↔3D trips in renderer 3 → renderer and 3D size survive each trip, no leak of GL windows/contexts. Covered by Task 3's manual check list and the helper test.
5. Alt+K into renderer 3 when GL fails mid-session → message, stays on the previous renderer, no crash. Test in Task 5 by unit-testing the cycle function with a stub "GL available" flag.

---

### Task 1: build.cfg module

**Files:**
- Create: `source/buildcfg.c`, `source/buildcfg.h`
- Modify: `source/CMakeLists.txt` (link `buildcfg.c` into `buildeditor`)
- Test: `tests/build_cfg.c`, `tests/CMakeLists.txt`

**Interfaces:**
- Produces:
  ```c
  #define BUILDCFG_NUMKEYS 19
  typedef struct {
      int32_t renderer, xdim3d, ydim3d, fullscreen, gltexmiplevel;
      double mousesensitivity;
      unsigned char keys[BUILDCFG_NUMKEYS];
  } buildcfg_t;
  void buildcfg_defaults(buildcfg_t *c);              /* keys = astub.c's current keys[] */
  int  buildcfg_read(buildcfg_t *c, const char *path); /* 0 read, -1 missing; clamps */
  int  buildcfg_write(const buildcfg_t *c, const char *path); /* 0 ok, -1 error */
  int  buildcfg_import_setupdat(buildcfg_t *c, const char *path); /* keys only; 0 ok, -1 missing */
  int  buildcfg_path(char *out, size_t outlen);       /* <config folder>/build.cfg; creates folder */
  ```
  Key names in array order: `keyforward keybackward keyturnleft keyturnright keyrun keystrafe keyfire keyuse keystandhigh keystandlow keylookup keylookdown keystrafeleft keystraferight key2dmode keyviewcycle key2dzoomin key2dzoomout keychat`. Defaults: `c8 d0 cb cd 2a 9d 1d 39 1e 2c d1 c9 47 49 9c 1c 0d 0c 0f`.
  Clamps: renderer 0–3; xdim3d 320–7680, ydim3d 200–4320; fullscreen 0/1 (nonzero → 1); gltexmiplevel 0–15; mousesensitivity 0.1–10.0; key values 0x01–0xff (0 or out of range → default).
  `setup.dat` layout: 8 option bytes, then 19 key bytes; only keys are taken; a short file keeps defaults for missing keys.

- [ ] **Step 1: Failing tests** in `tests/build_cfg.c` (no engine, no SDL; temp files under `$TMPDIR`):
  - defaults written then read back equal defaults; the written file contains `renderer = 0`, `xdim3d = 640`, `fullscreen = 1`, `keyforward = c8`.
  - `renderer = 9`, `xdim3d = 99999`, `gltexmiplevel = -3`, `mousesensitivity = 50`, `keyfire = 0` → 3, 7680, 0, 10.0, default `9d`.
  - garbage lines (`renderer`, `= 5`, `xdim3d = abc`, CRLF endings, `; comment`, `unknownkey = 1`) → defaults where invalid, valid lines still applied, `buildcfg_read` returns 0.
  - missing file → returns -1, struct untouched.
  - `setup.dat` of 27 bytes with keys `11 1f ...` → those keys; 10-byte file → first 2 keys from the file, rest default; missing → -1.
- [ ] **Step 2:** Run `ctest -R build_cfg` → FAIL (not built).
- [ ] **Step 3:** Implement `buildcfg.c`. `buildcfg_path` mirrors `setup_homedir`: macOS `~/Library/Application Support/Duke Nukem 3D/`, other Unix `~/.duke3d/`, Windows the current directory; creates the folder if missing. Write via a temp file + rename so a failed write never truncates the old file.
- [ ] **Step 4:** `ctest -R build_cfg` → PASS; full suite passes.
- [ ] **Step 5:** Commit `editor: add a build.cfg reader and writer`.

### Task 2: engine fullscreen setter

**Files:** Modify `source/build/sdl_driver.c`, `source/build/display.h` (or the header that declares `setgamemode_gl`); Test `tests/sdl_driver_smoke.c`.

**Interfaces:** Produces `void setvideofullscreen(int on); int getvideofullscreen(void);` — the setter applies to the next mode set; `BUILD_WINDOWED` still forces 0 (the env check moves so it wins over the setter); the getter reflects Alt+Enter toggles. The game never calls the setter.

- [ ] **Step 1: Failing test** in `sdl_driver_smoke`: with `BUILD_WINDOWED` unset, `setvideofullscreen(0)` then a mode set → `getvideofullscreen()==0`; with `BUILD_WINDOWED=1`, `setvideofullscreen(1)` → still 0.
- [ ] **Step 2:** Run → FAIL. **Step 3:** Implement. **Step 4:** Run → PASS, full suite (game unchanged).
- [ ] **Step 5:** Commit `engine: let the editor choose fullscreen before the first mode set`.

### Task 3: editor reads build.cfg and keeps the renderer across 2D/3D

**Files:** Modify `source/astub.c` (`ExtInit` around line 1884–1905, `ExtUnInit` ~1916), `source/build/build.c` (`main` ~6772–6810 incl. `-renderer`, the 2D→3D return ~4938, the 3D size globals); Test: a new `editor_mode` case (see Step 1).

**Interfaces:**
- Consumes Task 1's API and Task 2's setter.
- Produces in the editor: `static int32_t editorrenderer;` (configured), `int32_t editorrendermode;` (in use) and `int editorsetgamemode(void)` — sets the 8-bit 3D mode at `xdim3d`x`ydim3d` (8-bit modes keep the engine's 1600x1200 clamp), then `setgamemode_gl` if renderer 3 (fallback to 0 with a printed message like the game's), then `setrendermode`, `gltexmiplevel`, and `polymosttexfullbright = 240` in renderer 3.
- `setup.dat` reading and the `option[]`/`vesares` code are removed from `astub.c`; migration through `buildcfg_import_setupdat` only when `build.cfg` is missing (look for `setup.dat` in the current directory as today).
- `-renderer N` parsed in `build.c` main (before `ExtInit` uses the config), clamped like the game's `clamprenderer`; not saved.

- [ ] **Step 1: Failing test.** If `editorsetgamemode` can be exercised headless: add `tests/editor_mode.c` linking the engine + a copy-free hook (put `editorsetgamemode` in a small `source/editormode.c` so the test links it without `build.c`), run under `SDL_VIDEODRIVER=dummy` with `BUILD_WINDOWED=1`: renderer 3 requested → returns 0, `getrendermode()==0`, the configured renderer still 3; renderer 2 → `getrendermode()==2` (POLYMOST builds) or 0 (OFF build).
- [ ] **Step 2:** Run → FAIL. **Step 3:** Implement the wiring; `ExtUnInit` writes `build.cfg` (configured renderer, or Alt+K's choice; the current `getvideofullscreen()`).
- [ ] **Step 4:** Full suites (1.3D, 1.5, nopm). Manual smoke with BMP/PNG captures (F12) in a scratch dir: editor starts in each renderer 0–3, 2D↔3D three times keeps the renderer and size, `build.cfg` written on exit, a `setup.dat` with remapped keys migrates once.
- [ ] **Step 5:** Commit(s), e.g. `editor: read build.cfg and drop setup.dat`, `editor: keep the renderer and 3D size across 2D mode`.

### Task 4: fix the OpenGL crash sites

**Files:** Modify `source/build/build.c` (`drawtilescreen` ~1692–1760, the F5–F8 3D calls ~6742–6760 or their astub targets, the quit/save prompts ~6863–6895), `source/astub.c` if the guard lives there.

**Interfaces:** Consumes `setpolymost2dview()`, `polymost_drawtilescreen(dax,day,wallnum,dimen)` (returns -1 when not drawn), `glredrawlastframe()`.

- [ ] **Step 1:** Port JFBuild db5ed47's `drawtilescreen` hook (recon: J04 `build.c:2477-2570`): `setpolymost2dview()` once, then per tile `polymost_drawtilescreen(...)` and the software blit only when it returns -1.
- [ ] **Step 2:** In 3D mode under renderer 3, F5/F6/F7/F8 don't call the 16-colour status-bar helpers (classic 3D keeps today's behaviour).
- [ ] **Step 3:** Quit/save prompts call `glredrawlastframe()` before drawing their text in renderer 3.
- [ ] **Step 4:** Verify by running the editor in renderer 3 (scratch dir): tile browser (V in 3D), F5–F8, Esc → quit prompt, with F12 captures judged; run in renderer 0 and 2 to show the classic paths unchanged. Full suites.
- [ ] **Step 5:** Commit(s) `editor: draw the tile browser through Polymost in OpenGL`, `editor: keep F5-F8 and the prompts working in OpenGL`.

### Task 5: polish

**Files:** Modify `source/build/build.c` (mouse ~2262 and ~4978, look/turn ~5038–5089, `showmouse` ~401, key handling for Alt+K), `source/build/sdl_driver.c` only if the HiDPI mouse scale needs an engine helper.

**Interfaces:**
- `int editorcyclerenderer(int current, int glavailable)` (in `editormode.c`, unit-testable): returns the next renderer 0→1→2→3→0, skipping 3 when `glavailable==0`, and skipping 1–3 entirely with Polymost off.
- Mouse: `searchx += (mousx * mousesensitivity) / 2` accumulated with sub-pixel remainder, in 2D and 3D; at 1.0 integer motion equals today's `>>1`. In renderer 3 deltas are scaled from window points to drawable pixels.
- Look/turn: scaled by elapsed time (the editor's `synctics` or `totalclock` delta) so the per-second rate at the classic frame rate equals today's per-frame amounts.
- Alt+K: calls `editorcyclerenderer`, re-runs `editorsetgamemode` when entering/leaving 3, shows the renderer's name with `printmessage256` for about two seconds; a GL failure leaves the previous renderer and shows a message.
- Cursor: `showmouse` draws with `drawline256` (as JFBuild 2025) so it scales with resolution.

- [ ] **Step 1: Failing test** for `editorcyclerenderer` (in `tests/editor_mode.c`): 0→1, 2→3, 3→0, 2→0 with GL unavailable, and OFF build 0→0.
- [ ] **Step 2:** Run → FAIL. **Step 3:** Implement all five items. **Step 4:** Test passes; full suites; manual: Alt+K cycles with captures, mouse speed at sensitivity 1.0 matches before (count `searchx` change for a scripted mouse delta if possible), look speed equal in renderer 0 and 3.
- [ ] **Step 5:** Commit(s) per item, e.g. `editor: cycle renderers with Alt+K`, `editor: scale look and turn by time`, `editor: add mouse sensitivity and HiDPI cursor speed`, `editor: draw the 3D cursor with lines`.

### Task 6: docs, matrix and hand-over

**Files:** Modify `README.md`.

- [ ] **Step 1:** README: a "Map editor" note under Renderers — `build.cfg` location, every key with its default, the `setup.dat` one-time migration, `-renderer N`, Alt+K, `BUILD_WINDOWED`.
- [ ] **Step 2:** Matrix: 1.3D, 1.3D shareware, 1.4, 1.5 with Polymost, 1.5 with `DUKE3D_POLYMOST=OFF`; each builds and passes `ctest`.
- [ ] **Step 3:** Commit `docs: document the editor's build.cfg and renderers`.
- [ ] **Step 4:** Hand over the user's check (spec "Success check") with setup commands for a scratch dir.
