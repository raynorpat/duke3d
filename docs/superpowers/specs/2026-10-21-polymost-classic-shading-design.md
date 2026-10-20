# Classic Shading for Polymost OpenGL — Design

Date: 2026-10-21
Status: Approved

## Intent

Phase 3 of the Polymost work. Phase 1 (`docs/superpowers/specs/2026-10-19-polymost-design.md`)
brought JFBuild 2004's Polymost into the game; phase 2
(`docs/superpowers/specs/2026-10-20-editor-polymost-design.md`) brought it to the editor's 3D mode.
Both kept JFBuild 2004's OpenGL look. Phase 3 makes renderer 3 look like the classic renderer in
colour and light — "the crisp look of software" and classic's distance darkening *(user's words)* —
while keeping Polymost's true perspective, "a hallmark of polymost" *(user)*. It also modernises how
renderer 3 draws, in three tiers by OpenGL version *(user's decision)*.

## What the comparison showed

Side-by-side captures of E1L1, classic vs renderer 3 at the same camera (session scratchpad
`phase3-compare/`, with `notes.md` and polymost.c references):

- **Texture crispness already matches**: renderer 3 samples nearest, no mipmaps, anisotropy 1
  (polymost.c ~104–122). Not a phase 3 problem.
- **Distance darkening is the main gap**: classic darkens toward black in whole shade steps
  (engine.c ~973–976, ~1069–1070); renderer 3 uses black exponential fog that is far too weak
  (polymost.c ~339–347, ~1540, `FOGSCALE` ~86). Measured means: shaded back room 2.7 classic vs 23.4
  GL; far street 8.5 vs 28.1.
- **Sector shade is linear in GL** ((32-shade)/32, polymost.c ~739–748) where the palette's shade
  tables fall off faster (shade 28: 0.03 classic vs 0.12 GL). Dark sectors look brighter and greyer.
- **Lit sprites wash out** (fire peach/yellow instead of red-orange) from the shaded base pass plus
  the full-colour glow pass (polymost.c ~594–611).
- **Translucency** uses fixed alpha 0.66/0.33 (polymost.c ~741–746) instead of classic's
  translucency table; webs and glass look lighter and greyer.
- **Look up/down**: GL uses true perspective, classic shears. **Kept as is** *(user)*; the sky's
  vertical position follows from it.

## Goals

1. **Three drawing tiers in renderer 3** *(user's decision)*:
   - **GL 1.1**: immediate mode, plus vertex arrays where they help (oldest supported path).
   - **GL 1.5**: vertex buffer objects.
   - **GL 2.1**: GLSL shaders and framebuffer objects, with the palette-index pipeline below.
   Renderer 3 picks the best tier the driver supports. A setting forces a lower tier for testing.
   Renderer numbering, `-renderer`, the editor's Alt+K and the game's renderer selection are
   unchanged; tiers live inside renderer 3 *(user)*.
2. **GL 2.1 tier: palette-index rendering** *(user's decision)*:
   - Surfaces (walls, floors, ceilings, masked walls, sprites, sky, rotatesprite/HUD, 2D drawing)
     draw the tile's **palette indices**, not RGB, with each surface's shade/palookup number and
     its view distance (as classic computes visibility).
   - A resolve pass looks each pixel up through the palookup (shade) tables and the current
     palette, so distance darkening and sector shade match classic exactly.
   - Palette changes (underwater/slime/night vision/title/ANM frames) and palette fades/flashes are
     done in the resolve pass, replacing texture rebuilds and phase 1's fade quad on this tier.
   - Fullbright (indices 240–254 etc.) follows classic: the shade tables already keep those
     entries bright, so no separate glow pass on this tier.
   - Translucent masked walls and sprites blend through classic's translucency table (`transluc`),
     by palette index, matching classic's two levels *(my call: it's the only way to match classic
     exactly; the builder may choose how — e.g. read-back of the destination index in the shader
     via the framebuffer or ordered passes)*. GL 2.1 can't read the destination in a shader, so
     this needs framebuffer copies; batch them (e.g. per depth-sorted group) to meet the speed bar.
     **Exactness wins** unless the speed bar below is missed; if it is, stop and report with
     measurements rather than silently approximating.
   - **Distance shade per pixel** *(my call)*: classic's visibility formula applied to each pixel's
     own depth, quantised to whole palookup steps. Classic's per-column (walls) / per-row (floors)
     banding comes from its sheared projection and is not copied under true perspective.
   - Masked/transparent texels use index 255 as classic does (alpha test on index).
3. **Internal resolution** *(user: both)*:
   - Default: the window's real drawable size (as today).
   - A setting picks a lower internal size (e.g. 320x200, 640x400); the frame is scaled up to the
     window with nearest filtering ("hard pixels"), 2D overlays included. GL 2.1 tier only
     *(my call: needs an FBO)*.
   - **The game sees the internal size** *(my call)*: `xdim`/`ydim` become the internal size, so
     the HUD, status bar, menus, text and the widescreen field of view are laid out exactly as
     classic lays them out at that size. Mouse/cursor coordinates are scaled from the window.
   - **Pixel shape** *(my call)*: the same as the 8-bit modes' window today (square pixels; e.g.
     320x200 shows as a 16:10 picture, as the classic 320x200 window does), scaled by the largest
     factor that fits and centred with black bars. So an internal 320x200 frame lines up with a
     classic 320x200 capture.
4. **GL 1.1 and 1.5 tiers: closer, not exact**:
   - Shade follows the palette's shade tables instead of the linear ramp (e.g. a per-shade
     brightness table derived from the palookups at palette load).
   - Distance fog strength and curve fitted to classic's visibility darkening (linear or exp fog
     tuned so the measured means above land close to classic).
   - Lit sprites lose the washed-out look as far as fixed function allows.
5. **Settings** *(names my call)*:
   - `duke3d.cfg` `[Screen Setup]`: `GLDrawPath` (`auto` default, `immediate`, `vbo`, `shader`),
     `GLInternalWidth` / `GLInternalHeight` (0 = window size, default 0).
   - `build.cfg`: `gldrawpath`, `glinternalwidth`, `glinternalheight` with the same meanings.
   - A requested path the driver lacks falls back to the best available, with a printed message;
     the configured value is kept (as with the renderer setting).
6. **README** documents the tiers and settings.

## Non-goals

- Classic's look-up/down shearing (true perspective stays).
- Any change to renderers 0–2.
- The editor's 2D mode.
- md2/hightile/texture packs, texture cache, OSD.
- GL 3+ core profile (macOS's legacy context tops out at 2.1, which is the top tier).

## Anti-goals

- Classic renderer 0 changing at all (pins stay byte-identical).
- Demo sync affected in any renderer.
- Renderer 3 getting slower in a way a player notices on the tier their machine picks. **Speed
  bar** *(my call)*: on the user's Mac (2.1 tier) at the default drawable size, the attract demos
  average at least 60 fps, and at least 80% of today's renderer 3 frame rate measured the same way
  (uncapped, vsync off for the measurement).
- Losing what phase 1/2 made work in renderer 3: fades, status bar, menus, cameras, mirrors,
  screenshots (which should show the resolved classic-coloured frame), widescreen, the map views,
  the editor's tile browser and prompts.

## Constraints

- Same build matrix as before: 1.3D, 1.3D shareware, 1.4, 1.5, 1.5 with `DUKE3D_POLYMOST=OFF`.
- The GL entry points beyond 1.1 must be loaded at runtime (SDL_GL_GetProcAddress / extension
  checks), so the binary still runs on a 1.1 driver. Linux CI has no GL at runtime; GL tests stay
  opt-in (`DUKE3D_GL_TESTS`).
- 64-bit conventions; AGENTS.md commit rules; never run in `~/Downloads/DUKE3D`.

## Success check

*(User's bar.)* On the GL 2.1 tier, side-by-side captures with classic are hard to tell apart, and
an automated opt-in test measures the pixel difference between classic and renderer 3 at
`horiz == 100` (looking straight ahead, where the projections line up) for a set of E1L1 viewpoints
and fails if it grows beyond the pinned value. **Target before pinning** *(my call)*: per
viewpoint, the mean absolute per-channel difference is at most 8 (of 255), and the comparison's
measured region means (shaded back room, far street) are within 3 of classic; the builder may
relax a target only with measurements and a recorded reason. The GL 1.1 and 1.5 tiers look clearly closer to
classic than today (measured on the same viewpoints and pinned too). The user watches the attract
demos in renderer 3 on their Mac (2.1 tier) as the final check.

## Left to the builder

- Index framebuffer format (e.g. R8/LUMINANCE8 colour attachment for indices + a second channel or
  attachment for shade/distance, or encoding shade into the index lookup at draw time) and the
  resolve shader's structure.
- How classic's visibility/distance-to-shade formula is reproduced per pixel (from `globalvisibility`,
  sector `visibility` and the depth), matching engine.c's integer maths closely enough for the
  pixel-difference bar.
- How translucency through `transluc` is achieved on 2.1.
- Batching/VBO layout for the 1.5 tier; which vertex-array uses help on 1.1.
- How mirrors/cameras (render-to-tile) and screenshots work in the index pipeline.
