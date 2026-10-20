# Polymost in the Build Editor — Design

Date: 2026-10-20
Status: Approved

## Intent

Phase 2 of the Polymost work (phase 1: `docs/superpowers/specs/2026-10-19-polymost-design.md`,
merged to `main` as 499a1cd). The map editor (`build`: `source/build/build.c` plus Duke's stub
`source/astub.c`) gets the Polymost renderers, so a mapper can fly around the map in 3D mode,
mainly in OpenGL, while building it. The editor should behave roughly like JFBuild's editor
*(user's decision)*.

Background research comparing JFBuild's editor (2004 import db5ed47 and 2025 2a8cfbb) with ours
is in the session scratchpad (`jfbuild-editor-recon.md`); its findings are summarised here.

## How JFBuild does it (the model to follow)

- Polymost is used only in 3D mode. **2D map editing stays an 8-bit software mode** in every
  JFBuild version; the renderer setting is kept while in 2D, and 3D re-enters it on return.
- The editor has its own config file, `build.cfg`, with a `renderer` key and separate 2D and 3D
  resolutions (JFBuild 2025 names: `renderer`, `xdim2d`/`ydim2d`, `xdim3d`/`ydim3d`).
- The tile browser draws tiles through `setpolymost2dview` + `polymost_drawtilescreen` in GL.
- Picking the wall/sprite under the cursor in Polymost is a ray-cast already in our polymost.c.

## Goals

1. **Renderer in 3D mode.** The editor's 3D mode can use renderers 0–3, the same numbering as
   the game. 2D mode stays as today (640x480, 16-colour EGA-style, software).
2. **`build.cfg`** *(user's decision)*: the editor's own config file, in the same folder the game
   keeps `duke3d.cfg`. JFBuild-style plain `key = value` lines, `;` comments *(my call: matches
   JFBuild; the editor doesn't link the game's scriplib)*. Keys:
   - `renderer` (0–3, default 0)
   - `xdim3d`, `ydim3d` (3D mode size, default 640x480). In renderer 3 the window gets this size,
     drawn at the real drawable size as in the game.
   - No `xdim2d`/`ydim2d`: our 2D mode stays 640x480 *(my call; a resizable 2D mode is a separate
     feature)*.
   - `fullscreen` (0/1, default 1, today's behaviour) *(my call)*. It covers 2D and 3D, so the
     window doesn't flip on 2D↔3D trips. The `BUILD_WINDOWED` environment variable still forces a
     window and wins over `build.cfg` (the tests rely on it). An Alt+Enter toggle is written back
     on exit, like the cycled renderer. Needs a small setter in `sdl_driver.c` called before the
     first mode set; the game is unaffected (it never calls it).
   - `gltexmiplevel` (0–15, default 0), applied like the game's `GLTextureMipLevel`.
   - `mousesensitivity` (JFBuild 2025 semantics: a decimal multiplier, default 1.0, applied in both
     2D and 3D and scaled so 1.0 is exactly today's speed, i.e. today's `>>1` stays the base)
   - Missing file: the editor starts with the defaults and writes `build.cfg` on exit. Values
     out of range are clamped on read and written back clamped. Unknown keys are ignored.
   - **Key bindings** *(user's decision: `build.cfg` replaces `setup.dat`)*: the 19 editor keys
     that `setup.dat` held, under JFBuild's names and hex format (`keyforward`, `keybackward`,
     `keyturnleft`, `keyturnright`, `keyrun`, `keystrafe`, `keyfire`, `keyuse`, `keystandhigh`,
     `keystandlow`, `keylookup`, `keylookdown`, `keystrafeleft`, `keystraferight`, `key2dmode`,
     `keyviewcycle`, `key2dzoomin`, `key2dzoomout`, `keychat`, e.g. `keyforward = c8`), with
     today's defaults (`keys[]` in `astub.c`). No `keyconsole` (no OSD).
   - **`setup.dat` is phased out.** The editor only ever used its keys; its `option[]` bytes are
     read and ignored today. One-time migration *(my call)*: when `build.cfg` doesn't exist but
     `setup.dat` does, its keys become the starting bindings and are written to `build.cfg` on
     exit. After that `setup.dat` is never read. The editor never deletes or writes `setup.dat`.
     The unused `option[]`/`vesares` code goes.
3. **Command line:** `-renderer N` overrides `build.cfg` for that run, as in the game. It is not
   written back to `build.cfg` *(my call; the game's `-renderer` also doesn't persist)*.
4. **Renderer kept across 2D↔3D.** Going to 2D and back returns to the same renderer and 3D size.
   Use the game's pattern (`setgamemoderenderer` in `source/game.c`: 8-bit mode set, then
   `setgamemode_gl` when renderer 3, then restore the renderer) at editor start and on leaving 2D.
   GL unavailable → fall back to classic, as the game does.
5. **Fix what breaks under OpenGL:**
   - Tile browser (`drawtilescreen` writes through `frameplace`, which is NULL in GL): port
     JFBuild's `setpolymost2dview` + `polymost_drawtilescreen` hook; software renderers keep the
     current blit.
   - F5/F6/F7 (and F8) in 3D call astub's 2D status-bar helpers, which draw through a surface
     that doesn't exist in GL: in 3D mode don't draw them in GL (JFBuild's stub skips them in 3D).
     Classic 3D behaviour stays as today.
   - Quit and save prompts drawn over the previous frame: redraw the last frame first
     (`glredrawlastframe`) in renderer 3.
6. **Polish (all in, user's decision):**
   - Lights glow in OpenGL: set fullbright for palette indices 240–254 as the game does.
   - Look up/down and turning speed independent of frame rate (scale by elapsed time, as
     JFBuild 2025 does), in all renderers.
   - Mouse sensitivity from `build.cfg`.
   - Correct cursor speed on HiDPI in GL (mouse deltas in window points vs drawable pixels).
   - A key to cycle renderers in 3D mode: **Alt+K** *(my call: K is unused in both build.c and
     astub.c in 3D, F-keys are all taken and unmodified F-key handlers ignore modifiers; the user
     can veto)*. Cycles 0→1→2→3→0, skipping 3 if GL is unavailable; shows a short message with the
     new renderer's name. The cycled renderer is what gets written to `build.cfg` on exit.
   - The 3D cursor drawn with `drawline256` (as JFBuild 2025) so it's visible at high resolution.

7. **README** documents `build.cfg` (keys, defaults, the `setup.dat` migration), `-renderer` for the
   editor and Alt+K.

## Non-goals

- Drawing the 2D map in OpenGL, or a resizable/256-colour 2D mode.
- Mouse look in the editor (JFBuild never had it; it conflicts with cursor picking).
- md2/hightile/defs, OSD, texture cache, JFBuild's startup window.
- Closer-to-classic shading (phase 3).

## Anti-goals

- The classic editor (renderer 0) changing behaviour beyond: the 3D view size now comes from
  `build.cfg` (default 640x480 instead of the current 320x200 start / 640x480 after visiting 2D),
  and look/turn speed now scales with time. Maps saved must be byte-identical to what the editor
  saves today for the same edits.
- The game (`duke3d`) changing in any way.
- Losing a user's `setup.dat` key setup (the one-time migration carries it over).

## Constraints

- Same build matrix as phase 1: 1.3D, 1.3D shareware, 1.4, 1.5, and 1.5 with `DUKE3D_POLYMOST=OFF`.
  With Polymost off, `renderer`/`-renderer` 1–3 fall back to 0 (as in the game) and the editor
  builds and runs as today otherwise.
- 64-bit conventions of this tree (int32_t / intptr_t).
- Commit rules in AGENTS.md.
- Never run the editor or game inside `~/Downloads/DUKE3D`; symlink the data.

## Success check

The user opens E1L1 in the editor with `renderer = 3` and flies around in 3D, opens the tile
browser, goes to 2D and back, presses F5–F7, cycles renderers with Alt+K, saves and quits, and
checks that `build.cfg` kept the settings (including key bindings carried over from an existing
`setup.dat`). *(User's decision.)* Automated tests cover what can be
tested headless (config read/write/clamp, renderer fallback, mode helper) in the existing test
style.

## Left to the builder

- Exact parsing/writing code for `build.cfg`, and where it lives (a small editor-side file).
- How the editor finds the `duke3d.cfg` folder (`setup_homedir` in `source/global.c` isn't linked
  into the editor today: link it or copy the logic).
- Keeping the configured renderer separate from the one in use, as the game does
  (`RendererSetting` vs `gamerenderer`), so a GL fallback isn't saved.
- How elapsed time is measured for frame-rate-independent look (the editor's existing timer).
- The message text and duration for the renderer-cycle key.
- Whether astub or build.c holds the F5–F8 3D-mode guard.
