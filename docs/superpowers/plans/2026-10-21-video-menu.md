# VIDEO Menu Live Settings Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** The game's OPTIONS → VIDEO page changes renderer, display mode, resolution, vsync, brightness, contrast/saturation, supersampling and texture filtering live, without a restart.

**Architecture:** Pure helpers (settings, resolution list, what-to-save tracking) are unit-tested on their own. The engine gains runtime entry points for renderer switching (with a one-time GL window rebuild), resolution/display changes, supersampling (an oversized FBO resolved down at present) and texture filtering (a colour-drawing variant of the shader path when filtering is on). The menu page drives those entry points.

**Tech Stack:** C99, SDL2, legacy OpenGL ≤ 2.1 / GLSL 1.20, CMake/CTest.

**Spec:** `docs/superpowers/specs/2026-10-21-video-menu-design.md`

## Global Constraints

- Renderer 0 output byte-identical (classic pins); phase 3's exact shader path unchanged with filtering = nearest and supersampling = off (render_gl pins); demo and multiplayer sync unaffected by any VIDEO change.
- Rows and rules exactly as the spec's table and bullets (resolution semantics per display mode and renderer; 8-bit draw size capped at 1600×1200, scaled up beyond; borderless greys RESOLUTION; supersampling fixed heights 1440/2160/4320 at window aspect, greyed when not larger than the window, shader path only, internal size wins; filtering nearest/bilinear/trilinear, OpenGL only; first switch to OpenGL rebuilds the window once, failure greys it out for the run; page shows values in effect, saves only rows changed).
- New config keys: `duke3d.cfg [Screen Setup] Supersampling`, `GLTextureFilter`; `build.cfg` `supersampling`, `gltexturefilter`. Menu shows "fullscreen" for the `exclusive` display mode (config value unchanged).
- OPTIONS loses BRIGHTNESS (moved to VIDEO); F11 unchanged.
- Build matrix: 1.3D, 1.3D shareware, 1.4, 1.5, 1.5 `DUKE3D_POLYMOST=OFF`; GL tests opt-in (`DUKE3D_GL_TESTS`). AGENTS.md commits + `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Never run in `~/Downloads/DUKE3D`; never touch the real `~/Library` config (redirect HOME in-program for manual runs, revert hooks).

## Review Focus

1. Picking OPENGL in a run that started classic, on a machine where GL fails → message, OPENGL greyed, previous renderer back, window intact. Test in Task 2 (gl_forcefail hook).
2. Changing renderer/resolution/display during demo playback and in a multiplayer game → no desync (demo_sync tests with a mid-demo switch hook). Test in Task 2/3.
3. 8k supersampling beyond GL_MAX_RENDERBUFFER/TEXTURE size → clamped with a message, no black screen. Test in Task 4.
4. Leaving VIDEO after a `-renderer` / Alt+Enter run-only override without touching those rows → saved values unchanged. Test in Task 1 (tracking helper) and Task 6.
5. Filtering on, then fades/flashes and see-through surfaces on the shader path → visible and sensible (approximate), no artefacts; filtering back to nearest → byte-identical to before. Test in Task 5.

---

### Task 1: settings, resolution list and save tracking

**Files:** Create `source/videoopts.c`/`.h` (engine-free helpers). Modify `source/config.c`, `source/buildcfg.c`/`.h`, `source/CMakeLists.txt`. Tests: `tests/video_opts.c`, `tests/game_helpers.c`, `tests/build_cfg.c`.

**Interfaces (produces):**
- `int videoopts_buildreslist(const vmode_t *modes, int nmodes, int desktopw, int desktoph, int displaymode, vmode_t *out, int maxout);` — windowed: modes that fit the desktop + classic sizes (320×200, 640×400, 640×480, 800×600, 1024×768, 1280×1024, 1600×1200 when they fit), deduped, sorted; fullscreen: SDL modes (deduped by size); borderless: returns 0 entries (greyed).
- `void videoopts_drawsize(int reqw, int reqh, int renderer, int *outw, int *outh);` — renderers 0–2 clamp to 1600×1200 keeping aspect; renderer 3 passes through.
- `int videoopts_ssheight(int level);` (0, 1440, 2160, 4320) and `int videoopts_ssavailable(int level, int winw, int winh, int maxfb, int shaderpath, int internalset);`
- Save tracking: `videoopts_t` holding saved values and a per-row "changed" bitmask; `videoopts_saveinto(...)` writes only changed rows.
- Config keys per Global Constraints, clamped, round-tripped.

- [ ] Step 1: failing tests for each helper (fake mode lists incl. duplicates/HiDPI pairs; drawsize cap; ssavailable cases; tracking: untouched rows not saved) and cfg round trips.
- [ ] Steps 2–4: implement; suites pass.
- [ ] Step 5: commits, e.g. `game: add video option helpers and supersampling/filter settings`.

### Task 2: live renderer switching

**Files:** Modify `source/game.c` (`setgamemoderenderer` and a new `setrenderer_live(int)`), `source/build/sdl_driver.c` (rebuild the window once with GL from an 8-bit SDL_Renderer window), `source/editormode.c` only if shared code moves. Tests: `tests/render_gl.c`, demo-sync harness.

**Interfaces:** `int video_setrenderer(int r);` — returns the renderer in effect; 3 on a classic-started run rebuilds the window with GL once; failure → message, `video_glunavailable = 1`, previous renderer restored. Safe mid-frame boundary only (applied between frames).

- [ ] Step 1: failing tests: render_gl — start in 0, switch 0→3→0→3 live, window survives, context created once; forced GL failure (gl_forcefail) → returns previous renderer, flag set, window intact; demo — a mid-demo renderer switch keeps demo sync (extend the demo_sync test with a switch at a fixed tic).
- [ ] Steps 2–4: implement; suites pass; manual check in the game (temporary hook, HOME redirected).
- [ ] Step 5: commit, e.g. `game: switch renderer live from the video options`.

### Task 3: live resolution and display mode

**Files:** Modify `source/build/sdl_driver.c`, `source/game.c`. Tests: `tests/sdl_driver_smoke.c`, `tests/render_gl.c`.

**Interfaces:** `int video_setmode(int displaymode, int w, int h);` — windowed: window size w×h (8-bit draw size via `videoopts_drawsize`, scaled up beyond the cap); fullscreen: display mode w×h; borderless: desktop size (8-bit draw size capped). Re-runs vscrn/status bar layout, palette, fade state; renderer 3 keeps its context.

- [ ] Step 1: failing tests: smoke (dummy driver) — after each mode change xdim/ydim follow the rules; render_gl — renderer 3 resolution/display changes keep the context and the frame fills the window; demo sync unaffected by a mid-demo resolution change.
- [ ] Steps 2–4: implement; manual check of every combination in renderers 0 and 3.
- [ ] Step 5: commit, e.g. `engine: change resolution and display mode live`.

### Task 3b: fade around switches (addendum)

**Files:** Modify `source/game.c` (the video request queue), `source/build/sdl_driver.c` (a present-level fade independent of the game's palette fade). Tests: `tests/render_gl.c`, demo switch test, `tests/sdl_driver_smoke.c`.

**Interfaces:** `video_request*` calls start a transition: fade out over ~0.25 s of real time (present-level multiplier on the final colours, composed with the game's own fade), apply the queued change(s) at black, fade in over ~0.25 s. Several changes queued during one fade apply together. The game's palette/fade state (`palto`, `setpalettefade` values, `curpalette`) and game ticks are untouched.

- [ ] Step 1: failing tests: render_gl — during a transition the presented frame darkens toward black and recovers, in renderers 0 and 3, and the game's fade values are unchanged before/after; smoke — the transition timeline (pure function of elapsed ms); demo switch test still in sync with transitions on.
- [ ] Steps 2–4: implement; check by eye (grabs) a renderer switch, a resolution change and a display change.
- [ ] Step 5: commit, e.g. `game: fade out and in around video switches`.

### Task 4: supersampling

**Files:** Modify `source/build/polymost.c`, `source/build/sdl_driver.c` (scene FBO size, present downscale), `source/editormode.c`/`source/astub.c` (build.cfg). Tests: `tests/render_gl.c`.

**Interfaces:** shader path only; the scene FBO is `videoopts_ssheight(level)` high at the window aspect; xdim/ydim follow it (layout as at that size); the present pass downsamples to the drawable (box filter over the covered texels, after palette lookup and contrast/saturation); screenshots at window size; clamped to GL max sizes with a message.

- [ ] Step 1: failing render_gl checks: at 4k on a smaller window the presented frame equals a box-downsampled 4k render; edges measurably smoother (fewer hard steps) than off; off is byte-identical to before; clamping message when over the GPU maximum (simulate with a hook); fps at 2k/4k/8k recorded.
- [ ] Steps 2–4: implement; suites pass.
- [ ] Step 5: commit, e.g. `engine: supersample renderer 3's shader path`.

### Task 5: texture filtering

**Files:** Modify `source/build/polymost.c` (texture parameters; colour-drawing variant of the shader path), `source/build/glext.c` if needed. Tests: `tests/render_gl.c`.

**Interfaces:** `gltexturefilter` = 0 nearest, 1 bilinear, 2 trilinear (mipmapped). 1.1/1.5: set min/mag filters and mipmaps accordingly (rebuild textures on change). Shader path: nearest = today; bilinear/trilinear = draw RGB (texels converted through palookup at the surface's shade per pixel, filtered), distance shading still from the shade tables; translucency = alpha blend at classic's two levels (approximation); fades/flashes = the fade applied in the present pass over RGB (approximation).

- [ ] Step 1: failing render_gl checks: nearest → byte-identical to before on every path; bilinear/trilinear → frame differs from nearest, close to classic in region means (shade-table darkening kept: far-street/back-room means within 3 of nearest's), no off-palette assertion on this mode; translucency and fade visible; switching back to nearest restores byte-identical frames.
- [ ] Steps 2–4: implement; fps per mode recorded.
- [ ] Step 5: commit, e.g. `engine: add texture filtering to renderer 3`.

### Task 6: the VIDEO page

**Files:** Modify `source/menues.c` (OPTIONS case 200: BRIGHTNESS moves out; VIDEO case 710: all rows), `source/game.c` (save on leaving the page via the tracking helper). Tests: captures; any testable glue in `tests/game_helpers.c`.

- [ ] Step 1: layout all rows at 320×200 (tighter spacing or two columns); greyed states per rules; RESOLUTION list from `videoopts_buildreslist` applied on Enter; every other row applies live; leaving the page saves only changed rows.
- [ ] Step 2: verify by driving the menu in renderers 0 and 3 (temporary hook, HOME redirected, reverted): each row's effect, greyed rows, save behaviour with a `-renderer` run override; captures of OPTIONS and VIDEO for the user.
- [ ] Step 3: suites pass in all builds (1.3D and 1.5 menus).
- [ ] Step 4: commit, e.g. `game: put live renderer and resolution options on the VIDEO page`.

### Task 7: docs, matrix and hand-over

**Files:** Modify `README.md`.

- [ ] Step 1: README: the VIDEO page rows and rules, new keys, editor keys, supersampling and filtering notes.
- [ ] Step 2: matrix (1.3D, 1.3D shareware, 1.4, 1.5, 1.5 OFF) builds and passes; build-gl render_gl passes.
- [ ] Step 3: commit `docs: document the VIDEO page's live settings`.
- [ ] Step 4: hand over the user's check (spec Success check) with commands.
