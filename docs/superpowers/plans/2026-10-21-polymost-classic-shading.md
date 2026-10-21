# Classic Shading and Draw Tiers for Polymost OpenGL Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Renderer 3 draws through three tiers (GL 1.1 immediate/vertex arrays, GL 1.5 VBOs, GL 2.1 GLSL+FBO) and the 2.1 tier renders palette indices resolved through classic's shade tables, so it looks like the classic renderer while keeping Polymost's perspective.

**Architecture:** A small runtime GL loader picks the tier. Polymost's immediate-mode calls go through a thin submit layer (begin/vertex/end, texture bind, state) that each tier implements. The 2.1 tier draws into an FBO: an index attachment (tile texels uploaded as indices) plus shade/depth data; a resolve shader maps index+shade through palookup and palette textures (also applying fades), translucency blends through `transluc` with framebuffer copies, and the result is scaled to the window (optionally from a lower internal size the game sees as `xdim`/`ydim`). The 1.1/1.5 tiers keep RGB textures but use a shade-table brightness ramp and a fitted fog.

**Tech Stack:** C99, SDL2 legacy GL context (≤2.1 on macOS), GLSL 1.20, CMake/CTest.

**Spec:** `docs/superpowers/specs/2026-10-21-polymost-classic-shading-design.md` (comparison notes: session scratchpad `phase3-compare/notes.md`, with polymost.c line references)

## Global Constraints

- Renderer 0 unchanged (all classic pins byte-identical); demo sync unaffected in every renderer; renderers 1–2 unchanged.
- GL entry points beyond 1.1 loaded at runtime via `SDL_GL_GetProcAddress` + version/extension checks; the binary must run on a 1.1 driver.
- Settings: `duke3d.cfg [Screen Setup]` `GLDrawPath` = `auto` (default) | `immediate` | `vbo` | `shader`; `GLInternalWidth`/`GLInternalHeight` (0 = window, default 0). `build.cfg`: `gldrawpath`, `glinternalwidth`, `glinternalheight`, same meanings. Unavailable path → best available + printed message; configured value kept.
- Internal resolution (2.1 only): `xdim`/`ydim` = internal size; square pixels as today's 8-bit windows; largest fitting scale, centred, black bars; mouse scaled from window coordinates.
- 2.1 translucency: exact through `transluc`; batch framebuffer copies. Distance shade per pixel from classic's visibility formula, quantised to whole palookup steps.
- Speed bar (user's Mac, 2.1 tier, default drawable, vsync off): attract demos ≥ 60 fps average and ≥ 80% of today's renderer 3. If missed after reasonable batching, stop and report with numbers.
- Success target before pinning (2.1, `horiz == 100`, E1L1 viewpoints): mean abs per-channel diff ≤ 8; region means (shaded back room, far street) within 3 of classic. Relax only with measurements and a recorded reason.
- GL tests stay opt-in (`DUKE3D_GL_TESTS`); Linux CI has no GL at runtime.
- Build matrix: 1.3D, 1.3D shareware, 1.4, 1.5, 1.5 with `DUKE3D_POLYMOST=OFF`. 64-bit conventions. AGENTS.md commits + `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Never run in `~/Downloads/DUKE3D`.

## Review Focus

1. A driver reporting 2.1 but failing shader compile/link or FBO completeness → falls back to 1.5/1.1 with a message, no black screen. Test in Task 1 (forced failure hook) and Task 4.
2. Palette changes mid-frame and fades in 2.1 (underwater, flash, title fade, ANM frames) → correct colours with no texture rebuild. Test in Task 5.
3. Window resize / fullscreen toggle / Alt+K into renderer 3 with an internal size set → FBO reallocated, picture scaled correctly, mouse maps correctly. Test in Task 7.
4. Mirrors and camera screens (render-to-tile) and screenshots on 2.1 → correct classic colours. Test in Task 5.
5. Many translucent sprites (explosions/smoke) on 2.1 → correct blend and frame rate within the bar. Measured in Task 6.

---

### Task 1: GL loader, tiers and settings

**Files:** Create `source/build/glext.c`/`.h` (or a section of sdl_driver.c — builder's call, one clear owner). Modify `source/build/sdl_driver.c` (`setgamemode_gl` ~798), `source/build/polymost.c` (init), `source/config.c` (~81, ~315, ~699 next to `GLTextureMipLevel`), `source/buildcfg.c`/`.h`, `source/astub.c`/`source/editormode.c` (pass the editor's values), the game's mode helper in `source/game.c`. Tests: `tests/game_helpers.c` (cfg round trip/clamp), `tests/build_cfg.c`, a pure tier-selection unit test.

**Interfaces (produces):**
- `enum { GLPATH_AUTO, GLPATH_IMMEDIATE, GLPATH_VBO, GLPATH_SHADER };`
- `int gl_choosepath(int requested, int glmajor, int glminor, int hasvbo, int hasglsl, int hasfbo);` — pure; returns the path to use (best available ≤ requested; AUTO = best).
- `extern int gldrawpath;` (in use) set at GL init; engine globals `glinternalwidth`, `glinternalheight` (0 = window) and `gldrawpathrequest`.
- Function pointers for VBO (1.5) and GLSL/FBO (2.0/2.1 + `GL_EXT_framebuffer_object` or core-2.1-available FBO entry points as the driver offers) loaded at runtime; NULL when unavailable.
- `int gl_forcefail;` test hook (or env var) to simulate shader/FBO failure.

- [ ] Step 1: failing tests: `gl_choosepath` table (1.1 driver → IMMEDIATE for every request; 1.5 no GLSL → VBO for AUTO/SHADER; 2.1 full → SHADER for AUTO; request IMMEDIATE on 2.1 → IMMEDIATE); cfg: `GLDrawPath = "shader"`/`"bogus"` → SHADER/AUTO, written back lowercase; `GLInternalWidth = -5` → 0; build.cfg the same keys.
- [ ] Step 2: run → FAIL. Step 3: implement loader, selection, settings plumbing in game and editor; print the chosen path at GL init ("OpenGL path: shader (2.1)"). No rendering change yet: every path still draws immediate mode.
- [ ] Step 4: suites pass in build, build-15, build-nopm; render_gl (opt-in) passes unchanged.
- [ ] Step 5: commits, e.g. `engine: load OpenGL 1.5 and 2.1 entry points and pick a draw path`, `game: add GLDrawPath and GL internal size settings`, `editor: add gldrawpath and GL internal size to build.cfg`.

### Task 2: submit layer, vertex arrays and VBOs

**Files:** Modify `source/build/polymost.c` (every `bglBegin/bglVertex/bglTexCoord/bglEnd` site: drawpoly, sky, rotatesprite, 2D drawing, tile screen, map view), new submit code in polymost.c or `source/build/glsubmit.c`. Test: `tests/render_gl.c`.

**Interfaces (produces):** `void glsub_begin(int prim); void glsub_texcoord(float u, float v); void glsub_color(float r,float g,float b,float a); void glsub_vertex(float x,float y,float z); void glsub_end(void); void glsub_flush(void);` — immediate tier emits GL calls directly; vertex-array tier batches into client arrays and draws on state change/flush; VBO tier streams batches into a VBO (orphan + subdata or a ring). Batches flush on any texture/state change so draw order is preserved.

- [ ] Step 1: failing render_gl check: the same E1L1 viewpoints drawn with `GLDrawPath` immediate, vbo (and vertex arrays on the 1.1 tier) produce byte-identical frames.
- [ ] Steps 2–4: implement, run, suites + render_gl pass; measure attract-demo fps per path on the Mac (record in report).
- [ ] Step 5: commits, e.g. `engine: route Polymost's drawing through a submit layer`, `engine: draw Polymost batches with vertex arrays and VBOs`.

### Task 3: 1.1/1.5 tiers closer to classic

**Files:** Modify `source/build/polymost.c` (shade ramp ~739–748, fog ~339–347/~1540/`FOGSCALE` ~86, glow/fullbright ~594–611). Test: `tests/render_gl.c` (+ comparison harness logic adapted from the scratchpad's `phase3-compare/compare.c`).

**Interfaces (produces):** `void polymost_buildshaderamp(void)` — at palette load, per palookup shade 0..numshades-1, the mean brightness ratio of the shade table vs shade 0 (used instead of `(32-shade)/32`); fog parameters fitted so distance darkening tracks classic's visibility formula (linear `GL_FOG_START/END` or `GL_EXP` density from `globalvisibility`/sector visibility).

- [ ] Step 1: failing measurement test in render_gl: for the comparison viewpoints at `horiz==100`, renderer 3 (vbo and immediate) vs renderer 0 mean abs diff must be lower than today's (pin today's first in this step, then require improvement), and the two region means move toward classic.
- [ ] Steps 2–4: implement; lit sprites (fire) not washed out (glow pass blend/brightness fixed); suites pass; record the before/after diffs and region means.
- [ ] Step 5: commits, e.g. `engine: shade Polymost GL by the palette's shade tables`, `engine: fit Polymost GL fog to classic visibility`, `engine: stop the glow pass washing out lit sprites`. Pin the new values.

### Task 4: 2.1 index pipeline — opaque and masked surfaces

**Files:** Modify `source/build/polymost.c`, `source/build/sdl_driver.c` (FBO lifetime with the GL window, resize), new shader sources (embedded strings). Test: `tests/render_gl.c`.

**Interfaces (produces):**
- Index textures: tiles uploaded as single-channel indices (no palette conversion, nearest, no mips); index 255 = transparent (discard).
- Scene FBO: index attachment + shade/depth data (format builder's choice) at the render size; depth buffer.
- Palette resources: palookup texture (numpalookups × numshades × 256) and palette texture (256 RGB), uploaded on palette/palookup change.
- Resolve shader: per pixel, shade = clamp(surface shade + classic visibility(depth), 0, numshades-1) quantised to whole steps; colour = palette[palookup[pal][shade][index]].
- `polymost_beginframe_shader()/polymost_resolve_shader()` hooked into the frame (drawrooms → drawmasks → 2D → present).

- [ ] Step 1: failing render_gl check (shader path): E1L1 viewpoints at `horiz==100` vs renderer 0 meet the target (mean ≤ 8, region means within 3) for walls/floors/ceilings/sky/masked walls/sprites (translucency excluded until Task 6 — choose viewpoints without translucent surfaces, or mask them).
- [ ] Step 2: run → FAIL. Step 3: implement. Shader compile/link or FBO incompleteness → print and drop to the VBO path (Review Focus 1; test with the forced-failure hook).
- [ ] Step 4: pass; suites pass; capture side-by-sides into the report.
- [ ] Step 5: commits, e.g. `engine: draw Polymost as palette indices into a framebuffer on GL 2.1`, `engine: resolve palette indices through classic's shade tables`.

### Task 5: 2.1 — palettes, fades, 2D, fullbright, mirrors, screenshots

**Files:** Modify `source/build/polymost.c`, `source/build/sdl_driver.c` (fade quad / lastframe / `setpalettefade` / screenshots on the shader path), `source/menues.c` `glsetgamepalette` (no texture rebuild on the shader path). Test: `tests/render_gl.c`.

- [ ] Step 1: failing render_gl checks on the shader path: underwater/slime palette and a palette fade (`setpalettefade`) produce the same colours as renderer 0 at the same frame (mean ≤ 8); palette change triggers no `gltexinvalidateall`; rotatesprite HUD + status bar + text + menus match renderer 0 (mean ≤ 8); a camera screen / mirror (render-to-tile) shows classic colours; `screencapture` writes the resolved frame.
- [ ] Steps 2–4: implement (resolve pass applies the fade; phase 1's fade quad and lastframe remain for 1.1/1.5 and are adapted for the shader path's re-present); fullbright via the shade tables, no glow pass on this tier; suites pass.
- [ ] Step 5: commits per area.

### Task 6: 2.1 — exact translucency

**Files:** Modify `source/build/polymost.c` (masked walls, sprites with cstat translucency, `transluc`), shaders. Test: `tests/render_gl.c`.

**Interfaces:** `transluc` uploaded as a 256×256 index texture; translucent draws read the destination index from a copy of the index attachment (copy batched per depth-sorted group, `glCopyTexSubImage2D` or FBO blit), output the blended index.

- [ ] Step 1: failing render_gl check: a viewpoint with translucent webs/glass and one with a translucent sprite group (spawned explosions/smoke in a harness scene) match renderer 0 (mean ≤ 8 over the region).
- [ ] Steps 2–4: implement; measure attract-demo fps vs the speed bar (≥ 60 and ≥ 80% of today's r3); if missed after batching, stop with numbers (spec rule). Suites pass.
- [ ] Step 5: commit `engine: blend translucent Polymost surfaces through the translucency table`.

### Task 7: internal resolution

**Files:** Modify `source/build/sdl_driver.c` (mode set: `xdim`/`ydim` = internal size on the shader path, present scaling, mouse scaling, resize), `source/game.c` mode helper / `source/editormode.c` (pass the setting). Test: `tests/render_gl.c`, `tests/sdl_driver_smoke.c` if headless parts exist.

- [ ] Step 1: failing checks: with `GLInternalWidth/Height = 320x200`, renderer 3 on the shader path has `xdim==320, ydim==200`; the presented frame is the internal frame scaled by the largest integer-or-nearest factor that fits, centred with black bars; an internal 320x200 frame matches renderer 0 at 320x200 (mean ≤ 8 at `horiz==100`); a window resize keeps the internal size; mouse coordinates map window → internal. On 1.1/1.5 the setting is ignored with a message.
- [ ] Steps 2–4: implement; check the editor (3D mode, tile browser, cursor) at an internal size; suites pass.
- [ ] Step 5: commits, e.g. `engine: render renderer 3 at a GL internal size and scale it up`.

### Task 8: EDuke32 ports — voxel slab speedup, contrast and saturation (addendum)

**Files:** Modify `source/build/a.c` (`drawslab`), the shader path's present shader (polymost.c / sdl_driver.c), `source/config.c`, `source/buildcfg.c`/`.h`, the game's video menu (`source/menues.c`). Tests: a classic voxel render check (pin unchanged), render_gl, `tests/game_helpers.c`, `tests/build_cfg.c`. Reference patches (untrusted, read only): session scratchpad `eduke32-patches/a.patch` (27d32c7c), `b.patch` (5924cd30), `notes.md`.

- [ ] Step 1: `drawslab`: hoist the palette lookup out of the inner loop as 27d32c7c does (keep `while` unless the do/while is proven safe for every caller). Classic output byte-identical (a voxel render check pins it; add one if none exists).
- [ ] Step 2: contrast and saturation on the shader path's present pass only, with 5924cd30's formula (contrast 0.5–1.5, saturation 0–2, Rec.709 luma), applied after the palette lookup and fade. Duke's 8-level brightness (`britable`, F11, `ScreenGamma`) is unchanged. Defaults 1.0 = identity (shader-vs-classic pins unchanged at defaults).
- [ ] Step 3: settings `duke3d.cfg [Screen Setup] GLContrast`, `GLSaturation` (decimals, clamped), `build.cfg` `glcontrast`, `glsaturation`; game video menu sliders shown only when the shader path is active. Tests: cfg round trip/clamp; render_gl: non-default values change the presented frame as the formula says, defaults leave it identical; screenshots include them.
- [ ] Step 4: suites pass; commits, e.g. `engine: hoist drawslab's palette lookup out of its loop`, `engine: add contrast and saturation to renderer 3's shader path`, `game: add GL contrast and saturation settings`.

### Task 9: display mode and vsync (addendum)

**Files:** Modify `source/build/sdl_driver.c` (window flags at the 8-bit and GL window creation, Alt+Enter toggle, swap interval / `SDL_RENDERER_PRESENTVSYNC`), `source/config.c`, `source/buildcfg.c`/`.h`, `source/astub.c`/`source/editormode.c`, the game's video menu. Tests: `tests/sdl_driver_smoke.c`, `tests/game_helpers.c`, `tests/build_cfg.c`.

**Interfaces:** `enum { DISPLAY_WINDOWED, DISPLAY_BORDERLESS, DISPLAY_EXCLUSIVE };` with an engine setter/getter replacing or extending phase 2's `setvideofullscreen`; `void setvideovsync(int on)`. Borderless = today's `SDL_WINDOW_FULLSCREEN_DESKTOP`; exclusive = `SDL_WINDOW_FULLSCREEN` with `SDL_SetWindowDisplayMode` at the configured resolution (nearest supported mode). `BUILD_WINDOWED` still forces windowed. Alt+Enter toggles windowed ↔ the configured full-screen style.

- [ ] Step 1: failing tests: cfg keys `duke3d.cfg [Screen Setup] DisplayMode` (`windowed`|`borderless`|`exclusive`; an existing `ScreenMode`/fullscreen value maps to borderless when unset) and `VSync` (0/1, default 1); `build.cfg` `displaymode`, `vsync` (phase 2's `fullscreen` key read for compatibility: 1 → borderless); smoke: the setters' flags/getters, BUILD_WINDOWED precedence.
- [ ] Steps 2–4: implement for 8-bit and GL windows in game and editor; vsync via `SDL_GL_SetSwapInterval(0/1)` (GL) and recreating the SDL renderer with/without `SDL_RENDERER_PRESENTVSYNC` (8-bit); game video menu entries; suites pass; manual check on the Mac: each display mode in renderer 0 and 3, vsync on/off (fps capped vs uncapped), Alt+Enter.
- [ ] Step 5: commits, e.g. `engine: add borderless and exclusive fullscreen and a vsync switch`, `game: add display mode and vsync settings`, `editor: add displaymode and vsync to build.cfg`.

### Task 10: docs, matrix, speed and hand-over

**Files:** Modify `README.md`.

- [ ] Step 1: README: renderer 3's three paths and how one is chosen, `GLDrawPath`, `GLInternalWidth/Height`, `GLContrast`/`GLSaturation`, `DisplayMode`, `VSync` and the `build.cfg` equivalents, what each tier looks like.
- [ ] Step 2: matrix (1.3D, 1.3D shareware, 1.4, 1.5, 1.5 OFF) builds and passes; render_gl (opt-in) passes on the Mac with each forced path.
- [ ] Step 3: speed table in the report: attract-demo fps per path, today vs now.
- [ ] Step 4: commit `docs: document renderer 3's draw paths and internal size`.
- [ ] Step 5: hand over the user's check: attract demos in renderer 3 on the Mac (shader path), plus each forced path, plus an internal 320x200 run.
