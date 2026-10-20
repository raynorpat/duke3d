# Polymost Renderers — Design

Date: 2026-10-19
Status: Approved

## Intent

Bring Ken Silverman's Polymost renderers into this tree's engine, taken from JFBuild's
first import, `src/polymost.c` at commit
[`db5ed47a8b327754afb9b2ecc8c755f04e9e9f2c`](https://github.com/jonof/jfbuild/blob/db5ed47a8b327754afb9b2ecc8c755f04e9e9f2c/src/polymost.c)
(2004-09-15). The engine here is icculus's SDL2 port of Build (`source/build/`, see
`VENDORED.md`), not JFBuild, so Polymost has to be fitted into it.

The point is a hardware renderer that keeps the **classic look**, beside the
classic software renderer. It is not a high-resolution texture pack or model renderer.

## Phases

1. **This design.** The game (`duke3d`) gets Polymost's renderers, matching what JFBuild
   2004 does.
2. Later: the Build editor (`build`) gets them too.
3. Later: the OpenGL output moves closer to the classic renderer's shading (for example
   a palette shader).

Phases 2 and 3 are out of scope here. Phase 1 should simply not get in their way.

## Goals

1. Four renderers, numbered as in Polymost:
   - **0**: classic (today's renderer, unchanged)
   - **1**: Polymost solid colour (Ken's debug view)
   - **2**: Polymost software (polygon renderer into the 8-bit framebuffer)
   - **3**: Polymost OpenGL
2. The renderer is chosen by a `duke3d.cfg` setting. A command-line option overrides it
   for that run. *(User's decision.)* An in-game menu option comes later. Nothing in
   phase 1 should make that hard: switching goes through one engine call (Polymost's
   `setrendermode`, plus a video mode change when moving to or from 3).
3. In the OpenGL renderer the game is fully playable: 3D view, HUD weapon, status bar,
   menus, text, palette fades and flashes, camera screens, mirrors, and screenshots.
4. Rendering matches JFBuild 2004 *(user's decision)*. For OpenGL that means per-tile,
   per-palette RGBA textures, shading by vertex colour, depth by GL fog, and
   `GL_NEAREST` filtering by default, so the textures stay crisp.
5. Builds and runs on the three CI platforms (Linux, macOS, Windows), on all game
   versions (1.3D, 1.3D shareware, 1.4, 1.5).

## Non-goals

- `hightile.c` (texture replacements), `kplib.c` (PNG/JPG loading), `md2sprite.c`
  (models) and the `defs.c` script parser that feeds them *(user's decision)*. Polymost's
  calls into them are removed, or stubbed where removing them would mean rewriting
  Polymost code. `usemodels` is effectively 0.
- JFBuild's on-screen console (`osd.c`). Polymost's console commands (texture filter,
  anisotropy, texture compression) aren't reachable. Their variables keep their defaults.
- The editor (phase 2). The editor build must still compile and work with the classic
  renderer.
- Shading matching the classic renderer's palette ramps (phase 3).
- Polymost's debug keys (`USE_PMDBGKEYS`).
- Later JFBuild Polymost fixes. This is the 2004 code. Bug fixes to it are allowed when
  they're needed for the goals above (see Constraints).

## Anti-goals (failure even if Polymost works)

- **Any change to renderer 0.** With the default setting, the game draws exactly what it
  draws today, and the existing tests (including `render_e1l1`'s checksum) pass unchanged.
- **Any change to game logic.** The renderer must not affect gameplay. All demo sync
  tests pass, and the attract demos stay in sync whichever renderer is used.
- **A second engine.** Polymost goes into the existing `source/build/` engine, as in
  JFBuild: `polymost.c` is `#include`d by `engine.c` and hooked in at the places JFBuild
  hooks it. We don't import JFBuild's `engine.c`, `baselayer`, `sdlayer` or `osd`.

## Constraints

- **Vendored code.** `polymost.c` is copied into `source/build/` and recorded in
  `VENDORED.md` with the upstream URL and commit, in the style of the existing entries:
  a list of local changes, deliberate bug fixes called out. It goes through the same
  64-bit treatment as the rest of the engine: `long` → `int32_t` via
  `tools/retype_long.py`, with addresses as `intptr_t` or real pointers. Polymost's x86
  inline-asm `ftol`/`dtol` pragmas use their C fallback.
- **Licence.** `polymost.c` is under Ken's Build licence, like the rest of
  `source/build/`. Keep its header.
- **Platform layer.** Polymost needs things JFBuild's `baselayer` provided (`bpp`,
  `xres`/`yres`, `glinfo`, `getticks`, the GL context). These come from `sdl_driver.c`.
  For renderer 3, the SDL2 window is created with `SDL_WINDOW_OPENGL` and a GL context
  instead of the `SDL_Renderer` + streaming texture used today, and `nextpage` swaps
  buffers.
- **Renderer 3 resolution** *(user's decision)*: OpenGL draws at the window's real pixel
  size (the drawable size, so 640x400 for the default 320x200 window, the desktop size in
  fullscreen, and Retina pixels on macOS), and `xdim`/`ydim` are that size. HUD, status
  bar, menus and text scale to it through `rotatesprite`, as in JFBuild. `MAXXDIM`/
  `MAXYDIM` and the mode clamps change as needed. Renderers 0–2 keep today's
  configured resolution and SDL scaling.
- **Icculus's dead OpenGL code** *(my call)*: `sdl_driver.c` and `engine.c` already
  have `#ifdef USE_OPENGL` blocks from icculus's SDL 1.2 port. Nothing defines the macro
  and the code doesn't compile against SDL2. The new GL path replaces it, and
  `VENDORED.md` records that. Renderers 1 and 2 draw into the existing 8-bit surface and present as
  renderer 0 does.
- **OpenGL version.** Polymost 2004 uses fixed-function GL with immediate mode
  (`glBegin`/`glEnd`). Use a legacy or compatibility context. On macOS that means the
  2.1 legacy context, which is deprecated but still provided.
- **Build switch.** Polymost compiles in behind `POLYMOST`/`USE_OPENGL` as in JFBuild.
  A CMake option controls it, on by default, and links OpenGL. With it off, the engine
  is today's engine.
- **Version guards.** Polymost isn't version-specific. It must not disturb the
  `BUILD_ENGINEREV`-guarded 1.3D/1.4 logic in `engine.c`.
- **Commits** follow `AGENTS.md`: short, subsystem-prefixed (`engine:`, `build:`,
  `game:`, `docs:`).

## Engine hooks (from JFBuild 2004 `engine.c`)

These are the places JFBuild's `engine.c` calls into Polymost. Each needs an equivalent
in our `engine.c`, `sdl_driver.c` or `display.h`:

| Area | JFBuild 2004 behaviour |
|---|---|
| `drawrooms` | `polymost_drawrooms()`. Returns early when `rendmode != 0`. |
| `drawsprite`, `drawmaskwall` | `polymost_drawsprite` / `polymost_drawmaskwall` when `rendmode`. |
| `rotatesprite` | `polymost_dorotatesprite` when `rendmode`. |
| `setgamemode` | `bpp > 8` selects renderer 3; `polymost_glreset()` after a GL mode set. |
| `setbrightness` | Keeps an RGB `curpalette[]`. In GL, `gltexinvalidateall()` rebuilds textures with the new palette, so fades and flashes work. |
| `clearview`, `clearallviews`, `plotpixel`, `drawline256`, `printext256` | GL versions in renderer 3. `getpixel` returns 0. |
| `setviewtotile` / `setviewback` | Drop to renderer 0 while drawing into a tile (camera screens, screen tilt), then `invalidatetile` it. |
| `preparemirror` / `completemirror` | `completemirror` does nothing when `rendmode` (Polymost draws mirrors itself). |
| `drawmapview` | Does nothing in renderer 3 (JFBuild 2004's own todo). |
| `screencapture` | 24-bit via `glReadPixels` in renderer 3. |
| `hitscan` | `hitallsprites` flag, used by Polymost. |
| `setrollangle`, `invalidatetile`, `setpolymost2dview`, `setrendermode`, `getrendermode` | New engine calls. |

Our `engine.c` is icculus's, not JFBuild's, so names and statics differ. The builder maps
each hook to its equivalent here. Where icculus's engine has no equivalent of a JFBuild
global Polymost reads (`curpalette`, `glinfo`, …), add the minimum needed.

## Duke-side changes

- `config.c`: read and write the renderer setting in `[Screen Setup]` (for example
  `Renderer`, 0–3, default 0).
- `game.c`: a command-line option that overrides it (for example `-renderer N`, in the
  style of the existing `-demoverify`/`-loadverify` long options), checked and clamped.
- The video mode set at startup asks for a GL mode when the renderer is 3.
- The game's own direct framebuffer access, if any is found, needs a GL path or a
  documented gap.
- **ANM cutscenes work in renderer 3** *(my call; the 1.3D logo animation plays right
  before the attract demos)*. `playanm` (`menues.c`) sets the palette with
  `VBE_setPalette` directly and decodes each frame into a tile. In GL, palette changes
  made through `VBE_setPalette` update `curpalette` and invalidate textures as
  `setbrightness` does, and each new frame invalidates the ANM tile.

## Known gaps (accepted for phase 1)

- **Automap textured floors** are missing in renderer 3 (`drawmapview` isn't
  implemented in JFBuild 2004). The automap lines still draw.
- **Palette fades are slow** in renderer 3. Every fade step rebuilds the textures, as in
  JFBuild 2004.
- **Shading differs from classic**: a smooth fog-based fade instead of palette ramps.
  This is phase 3.
- Polymost's own todo list in the file header (sprite sorting, hall-of-mirrors near the
  zenith, …) is inherited.

## Verification

- **Main check (user's):** watch the attract demos play back in each renderer (1, 2, 3)
  on the user's 1.3D data (`~/Downloads/DUKE3D`, symlinked, never run in place). They
  must look right and stay in sync.
- All existing tests pass unchanged with Polymost compiled in, on every game version.
  In particular `render_e1l1` (renderer 0 checksum) and the demo sync tests.
- A headless test renders E1L1's start view in renderer 2 under the SDL dummy driver, in
  the style of `render_e1l1`. *(My call:)* like `render_e1l1`, it is registered only when
  `DUKE3D_GRP` is set, so it runs locally, not in CI (CI has no game data). Its checksum
  may be platform-specific because Polymost uses floating point. Renderer 3 isn't
  tested headless (no GL context under the dummy driver). It's checked by eye.
- **Sync risk, checked by eye** *(my call)*: `-demoverify` doesn't render, so the demo
  sync tests can't catch a desync caused by the renderer. Polymost culls sprites its own
  way, so `animatesprites` sees a different set and writes `hittype[].dispicnum`
  differently (display state that savegames also store). If anything the game calls
  from `animatesprites` touches gameplay state or `krand`, the demos will drift in that
  renderer. The builder checks `animatesprites` for this up front.
- CI builds on all three platforms.

## Left to the builder

- How `sdl_driver.c` switches between the 8-bit surface path and the GL path, and how GL
  functions are reached (direct linking vs. `SDL_GL_GetProcAddress` for extensions).
- The exact config key and command-line option names, as long as they fit the existing
  style.
- Whether `hightile`/`md2` calls are deleted or stubbed, case by case.
- Plan granularity and task order.
