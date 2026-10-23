# Vendored: icculus Build engine

- Upstream: https://github.com/icculus/buildengine
- Commit: `f3dce0a10bff90e1ed5cf8ec186b3731237a86da` (2021-07-12)
- License: Ken Silverman's Build license (`BUILDLIC.TXT`), **not** the GPL.
  icculus's port notes are in `LICENSE.md`.

Only the files the duke3d targets compile or include were copied:

```
a.c a.h bstub.h build.c build.h cache1d.c cache1d.h display.h doscmpat.h
engine.c engine.h engine_protos.h mmulti.c platform.h pragmas.c pragmas.h
sdl_driver.c unix_compat.c unix_compat.h win32_compat.h
BUILDLIC.TXT LICENSE.md
```

## Current source layout

The import list above records the original filenames. The local module split
moves their implementations as follows, retaining the original license notices
and public header names:

| Imported file | Current files |
|---|---|
| `a.c` | `draw.c` |
| `mmulti.c` | `network.c` |
| `cache1d.c` | `cache.c` (allocator/aging/diagnostics), `filesystem.c` (GRP, file I/O and LZW) |
| `pragmas.c` | `math.c`, together with the math/table helpers extracted from `engine.c` |
| `engine.c` | `engine.c` (world/renderer core), `display.c` (views, pixels/text and palette operations), `tiles.c` (tile/voxel resources and sprite bookkeeping), plus the helpers in `math.c` |
| `sdl_driver.c` | `sdl_main.c` (startup/events), `sdl_input.c` (devices/scancodes), `sdl_window.c` (video/presentation), `sdl_sys.c` (timing/diagnostics) |

`engine_internal.h` supplies only the declarations needed between these
modules. `engine.c` still owns the `ENGINE` definitions from `build.h`, includes
`polymost.c`, and bridges its private GL state for extracted display operations.
The four SDL files compile separately and use `sdl_internal.h` for their private
interfaces. Video resources stay in `sdl_window.c`, input accumulators and the
scancode map in `sdl_input.c`, and timer/debug state in `sdl_sys.c`.
`sdl_main.c` retains platform startup and synchronous event dispatch; the keyboard
map is initialized at the same point in startup. `../videoopts.c` keeps its
existing ownership. The split
preserves the existing algorithms, release guards, public signatures, resource
lifecycle and GL batch-flush behavior. Historical filenames below refer to
those implementations before the split.

## Local changes

- Removed the optional PhysicsFS backend, DOS platform/Watcom branches and
  obsolete assembly dispatch. The portable C engine and native GRP/file I/O
  are retained. The imported `doscmpat.h` has been removed; the original import
  list above is historical provenance.

- `VENDORED.md`: added (this file).
- `CMakeLists.txt`: added (build glue).
- `win32_compat.c`: added. Defines `hbits[]` for Windows; upstream only defines it in
  `unix_compat.c`, which refuses to build without `PLATFORM_UNIX`.
- `win32_compat.h`: removed `<dos.h>` and the legacy port-I/O stubs; only map `snprintf` to `_snprintf` before VS2015; under MSVC, define
  `S_IRWXU`, `S_IRUSR`, `S_IWUSR`, `S_IRGRP`, `S_IWGRP` and `S_IROTH` (the
  game uses them for `mkdir` and `savetemp`).
- `sdl_driver.c`: ported from SDL 1.2 to SDL2:
  - window + renderer + streaming ARGB texture; the 8-bit surface is
    blitted through its palette on every present
  - keycode table replaced by an `SDL_Scancode` table (positional, so the
    old AZERTY special cases are gone); key repeat ignored
  - mouse wheel via `SDL_MOUSEWHEEL`; grab via relative mouse mode
  - timer callback returns its interval (SDL2 took the old `return(1)`
    literally and ran the game at 1000 Hz)
  - `VBE_setPalette` presents immediately, because Duke's fades
    (`palto()`) never call `nextpage()`
  - `splash.bmp` support and the PowerPC/Gestalt AltiVec probe removed
  - `screencapture`: include `<malloc.h>` for `alloca` on Win32 and fall
    back for the `S_IRUSR`-style permission macros
  - `VBE_setPalette`: pass `&fmt_swap[start]` to `SDL_SetPaletteColors`
    (upstream passed `fmt_swap`, handing uninitialized colors to SDL when
    `start != 0`); deliberate upstream bug fix
  - `cleanup_vesa_modelist`: drop modes whose width isn't a multiple of 4
    (SDL pads 8-bit surface rows to 4 bytes, but the engine assumes
    `bytesperline == width`)
  - `sdl_mouse_wheel_filter`: negate `y` for `SDL_MOUSEWHEEL_FLIPPED`
  - `go_to_new_vid_mode`: set the window size even while fullscreen, so
    leaving fullscreen restores the new mode's size
  - `go_to_new_vid_mode`: call `SDL_InitSubSystem(SDL_INIT_VIDEO)` only when
    `SDL_WasInit(SDL_INIT_VIDEO)` is 0
- `cache1d.c`: `suckcache` passes `copybuf` a count of 4-byte words (it
  passed a byte count, copying 4x too much); pre-existing upstream bug
- All files: `long` → `int32_t` and `unsigned long` → `uint32_t` via
  `tools/retype_long.py`, which also adds `#include <stdint.h>` to every file it
  changes (64-bit port; restores the original 32-bit `long` semantics)
- 64-bit addresses: values holding memory addresses are `intptr_t`/`uintptr_t`
  or real pointers:
  - `frameplace`, `frameoffset`, `bakframeplace[]`, `FP_OFF` (now
    `((intptr_t)(x))`), `waloff[]`, `voxoff[][]`, `palookupoffse[]`,
    `bufplce[]`, `globalbufplc`, `slopalookup[]`, `asm2`, `asm3`
  - `cache1d.c` `cachestart` and the `allocache`/`initcache` handles
  - the address parameters of the `a.c` routines
  - prototypes in `a.h`, `engine_protos.h`, `display.h`, `engine.h`,
    `cache1d.h`
- `a.c`: the `vlineasm4`/`mvlineasm4`/`tvlineasm2` loops that relied on 32-bit
  address wraparound use equivalent pointer arithmetic; `rhlineasm4`/
  `rmhlineasm4` index the palookup with the pixel instead of the reverse;
  `slopevlin` reads palookup addresses from `intptr_t slopalookup[]`;
  `tspritevline` adds its step as signed
- `engine.c`: `drawmasks` swaps sprite pointers directly (it swapped half of
  each); `initengine` clears all of `waloff[]`; `loadpics` aligns `pic` with
  `~(intptr_t)15`; `parascan`'s last run of sky tiles stops at its wall's
  right edge (it drew one column more, with that column's bounds left over
  from an earlier, wider view: past the end of the frame after renderer 3)
- `build.h`: `BUILD_STATIC_ASSERT` checks `sectortype`/`walltype`/`spritetype`
  are 40/32/44 bytes
- printf formats match their now-32-bit arguments; C-library narrowings
  (`read`, `lseek`, `sendto`, `recvfrom`, `time`, `strtol`, `strlen`) are
  explicit casts
- `mmulti.c`: `socklen_t` is `int` on Windows (it was `size_t`, which is 8
  bytes on Win64 while Winsock's `recvfrom` takes an `int *`)
- `mmulti.c`: `connect_to_everyone` finds the local player's slot by port as
  well as host, so peers sharing an IP (e.g. two on localhost) get their own
  slots instead of both taking the last one
- `pragmas.h`: `klabs` negates as unsigned, so `klabs(INT32_MIN)` gives
  `INT32_MIN` as x86 `neg` does instead of undefined behaviour (ledger E-009)
- `engine.c`: version-guarded 1.4 engine logic, on `BUILD_ENGINEREV` (see
  ledger E-005)
- `engine.c`, `cache1d.c`: version-guarded 1.3D engine logic, on
  `BUILD_ENGINEREV`: the 1996 logic square root, slope maths, `inside` and the
  1996 LZW format (see ledger E-001 to E-004, E-012)
- `engine.c`: version-guarded 1.1 and 1.0 engine logic, on `BUILD_ENGINEREV`:
  the early `inside`, which compares the two products and keeps a one-byte
  count (see ledger E11-001)
- `engine.c`: `inside` computes coordinate differences, products and determinants
  as unsigned 32-bit arithmetic, preserving each revision's DOS wraparound
  without C signed overflow (E11-001, E-004).
- `engine.c`: version-guarded 1.0 engine logic, on `BUILD_ENGINEREV <
  19960220L`: `cansee`'s side test and `lintersect`; `hitscan` without a
  cliptype (always `CLIPMASK1`) that resets only `*hitsect`; `clipmove`
  accepting a bad sector and gathering every listed sector's walls before
  their sprites (see ledger E10-001 to E10-004)
- `a.c`, `a.h`: the C line drawers are JFBuild's `src/a-c.c` (Ken Silverman's
  own C port of `A.ASM` from `BUILDC.ZIP`, made 64-bit clean), from
  <https://github.com/jonof/jfbuild> commit
  `e06cbd005a8e6d5f38b5a7aa27ea597dd233c16e`, in place of the icculus port's
  register-by-register translation. Kept from the icculus port, so the picture
  matches `A.ASM`'s exactly: `setupslopevlin`/`slopevlin`, the one-pixel
  shortcut in `prevlineasm1`, `tvlineasm2` and the parallax-sky `rhlineasm4`
  family. The four-column `vlineasm4`/`mvlineasm4` loop over the one-column
  routines. Changes to JFBuild's code: our `intptr_t` signatures; per-family
  setup state as in `A.ASM` (JFBuild shares one set); `mhline`/`thline` draw a
  span's last pixel, as `A.ASM` does; the one-column vlines return the final
  texture position. `engine.c`'s rotated-sprite calls use JFBuild's simpler
  `spritevline` arguments. `mhlineskipmodify`, `thlineskipmodify` and
  `is_vmware_running` had no callers and are gone.
- `engine.c`, `sdl_driver.c`, `display.h`, `engine_protos.h`: icculus's dead
  OpenGL code is removed: every `USE_OPENGL` block (which included the missing
  `buildgl.h` and called `dgl*` functions and `opengl_load_library`, none of
  which exist, and didn't compile against SDL2), `sgldebug`, the
  `RENDERER_OPENGL3D` renderer with its name and `opengl3d` environment string,
  and `using_opengl()`. Polymost's OpenGL path replaces it (see below).
- `engine.c`, `build.h`, `engine_protos.h`, `sdl_driver.c`: hooks for Polymost
  (see below), all under `POLYMOST`/`USE_OPENGL`.

# Vendored: JFBuild polymost.c

- Upstream: https://github.com/jonof/jfbuild, `src/polymost.c`
- Commit: `db5ed47a8b327754afb9b2ecc8c755f04e9e9f2c` (2004-09-15), the first
  import of Ken Silverman's Polymost renderers
- License: Ken Silverman's Build license (`BUILDLIC.TXT`), as for the rest of
  `source/build/`. The file's header is kept.

`engine.c` `#include`s it, as JFBuild's does, when `POLYMOST` is defined (the
`DUKE3D_POLYMOST` CMake option, on by default, which also defines `USE_OPENGL`
and links OpenGL). The first commit that adds it holds the upstream file
unchanged.

The local source is organized as `polymost.c` (scene traversal, projection and
shared entry points), `polymost_soft.c` (software spans/debug rendering and its
depth buffer), and `polymost_opengl.c` (GL state, textures, shaders/framebuffers,
polygon submission and map tessellation). The shared file dispatches a projected
polygon to its backend and uses private helpers for GL state transitions.
Repeated sky/fog setup is shared within the OpenGL backend.

`engine.c` still includes `polymost.c`, which includes the backend files in the
same translation unit, with the GL include guarded by `USE_OPENGL`. CMake marks
all three as `HEADER_FILE_ONLY`; they are not separate objects. This retains
static engine/renderer state and public interfaces without exporting globals.
Rasterization equations, frame bindings, batch-flush wrappers and call ordering
are retained.

## Local changes

- `long` → `int32_t`, `unsigned long` → `uint32_t` via `tools/retype_long.py`
  (which adds `#include <stdint.h>`).
- 64-bit addresses:
  - the software renderer's z-buffer `zbufmem` is an `int32_t *` holding the
    `realloc` result, indexed by the pixel's offset into the frame
    (`vidp-(char *)frameplace`). Upstream kept the address in a `long` and
    indexed a biased pointer (`zbufoff = zbufmem-(frameplace<<2)`) with
    `(long)vidp`. `zbufoff` is gone.
  - `polymost_dorotatesprite` saves `frameoffset` in an `intptr_t`.
- `ftol`/`dtol`: only the C fallback is kept (the MSVC, Watcom and GCC x87
  versions are gone). It uses `lrintf`/`lrint`, which round to nearest as
  x87 `fistp` does. The engine is built with `-fno-math-errno` (GCC and
  Clang), so they inline rather than call libm.
- Removed, as out of scope (hightile, kplib, md2, defs, OSD):
  - `#include "md2sprite.c"`, `md2tims`, `usemodels` and the `md2draw` calls
    in `polymost_drawsprite`; the `models[]` loop in `gltexapplyprops`; the
    `clearskins()` calls
  - the hightile path of `gloadtile` (only the palette path is left) and its
    `effect` parameter; `gltexcache` keys on the palette alone (no
    `hictinting`, no `darealpalnum` parameter); `drawpoly` has no
    `hicfindsubst`/`hicreplc` lookups or tinting, and `hackscx`/`hackscy` are
    1.0; `polymost_precache` caches the tile with the palette it is given
  - the OSD console commands (`gltexturemode`, `gltextureanisotropy`) and
    `polymost_initosdfuncs`; a `DEBUGGINGAIDS` `OSD_Printf` and a commented-out
    one
  - the `USE_PMDBGKEYS` debug keys
- Classic-look defaults: `gltexfiltermode = 0` (`GL_NEAREST`),
  `glusetexcompr = 0`. `glanisotropy` stays 1.
- OpenGL comes from `"SDL_opengl.h"` (`GL_SILENCE_DEPRECATION` on macOS).
- `rendmode` is defined in `engine.c`, not here.
- `glinfo` (`struct glinfo_t { float maxanisotropy; char bgra, clamptoedge,
  texcompr, texnpot, vbo, glsl, fbo; }`, which upstream got from
  `baselayer.h`; `vbo`/`glsl`/`fbo` are set by `glext.c`) is declared in
  `build.h` under `USE_OPENGL`, not here. `sdl_driver.c` defines it, filled in
  by `setgamemode_gl`, and `nofog` (which upstream declares but no JFBuild
  file of that commit defines), set to 0.
- The forward declaration of `wallfront` is gone: ours is defined, non-static,
  before the `#include`.
- `polymost_dorotatesprite` takes `unsigned char dapalnum` and `unsigned
  short dastat`, as our `dorotatesprite` does.

## Later JFBuild fixes

Ported from later JFBuild commits, retyped as above:

- `1cb8debd775a9592de24f80b9c06fc5ff77dcf12`: `drawline2d` (renderer 1's
  wireframe) steps along the major axis and clips to the view exactly. The
  commit's `build.c` part (the editor's `clearkeys`) isn't taken.
- `61483f00734fd722bb4982786b758f26b7905a8a` (`engine.c`'s `drawmasks`): in
  renderers 1-3, the scan for sprites behind a masked wall tests the
  sprite's screen x against the wall's `dxb1`/`dxb2` (Polymost's float
  extents) instead of `xb1`/`xb2`, which Polymost doesn't fill in. Renderer
  0 keeps the `xb1`/`xb2` test.
  `dxb1`/`dxb2` stay static, as `engine.c` `#include`s this file; the
  commit's `polymosttex` changes aren't taken.
- `fd5d5b6a1c90fc3d582818412742d9f6dccb1d31`: `drawpoly` skips its
  non-power-of-two wall split when the texture is 0 wide (`1.0/tsizx` is
  infinite), which looped over every `int`.
- `368fbfa25503f9c21c366ee27ee08d556c1bf936`: non-power-of-two textures.
  `setgamemode_gl` sets `glinfo.texnpot` from
  `GL_ARB_texture_non_power_of_two` (and logs it). With it, `gloadtile`
  uploads a tile at its own size instead of padding it to a power of two,
  and `drawpoly`'s `xx`/`yy` and `polymost_drawtilescreen`'s pad use that
  size, so `drawpoly`'s non-power-of-two wall split no longer runs. Mipmap
  levels halve as `max(1, x>>1)`, which is what `(x+1)>>1` gave for every
  power-of-two size. Change from upstream: each of those sizes is at least
  1, so a 0-sized tile gets a 1x1 texture (upstream `malloc`ed 0 bytes and
  wrote `pic[0]`) and finite texture coordinates. The commit's `build.c`
  part (the editor's file list, `OSD_SetLogFile`) and its hightile
  (`gloadtile_hi`) part aren't taken.
- `ef311521781fc667d5d145f9c4c112987ec18ca9`: `gltexmiplevel`, like Quake's
  picmip. `uploadtexture` drops that many mipmap levels (clamped to
  `gltexmaxsize-1`, and written back), uploading level `js` as level 0, and
  drops more while a texture is over `1<<gltexmaxsize`. `gltexmaxsize` is the
  log2 of `GL_MAX_TEXTURE_SIZE`, read on the first upload (6 if GL returns 0)
  and again after each `polymost_glreset` (not upstream); both come from
  upstream between the 2004 import and this commit. Both are `int32_t`,
  declared in `build.h` under `USE_OPENGL`; the game sets `gltexmiplevel`
  from `duke3d.cfg`'s `[Screen Setup] GLTextureMipLevel` (0-15 when the
  config is read) before each GL mode set (textures made earlier keep their
  size until `polymost_glreset()`). With the `GL_NEAREST` default, a miplevel
  over 0 looks blocky. Change from upstream: `js` is cut, per texture, to the
  texture's last level, so one smaller than `2^js` (such as a 0-sized tile's
  1x1) still gets a level 0; upstream uploaded nothing and GL drew the
  polygon untextured. Not taken: the OSD variables (`gltexturemaxsize`,
  `gltexturemiplevel`); the commit's `drawpoly` part, which zeroes the
  alpha-test cutoff for invalid textures (E2L7's urinals behind the mirror),
  as our `drawpoly` has no alpha test; its `winlayer.c` part.
- `ccf7aa8d8019a249a4592d7dd9b950afa1cbbd3c`: the GL map view.
  `drawmapview` no longer returns at once in renderer 3, and `fillpolygon`
  hands over to the new `polymost_fillpolygon`, which turns the classic
  span-filler's `asm1`/`asm2`/`globalx1`/`globaly2`/`globalposx`/`globalposy`
  into screen-space texture gradients and draws the polygon with Ken's
  `tessectrap`/`drawtrap` trapezoids. Under `rendmode == 3` only,
  `drawmapview` takes a sector's walls with upstream's loop: it skips
  points collinear with their neighbours (`lastwall`) and relinks `point2`
  over each loop; renderer 0 keeps its loop. `polymost_dorotatesprite`
  pushes and pops the projection matrix around its own. Changes from
  upstream: `polymost_fillpolygon` converts `rx1`/`ry1` into its own float
  arrays (upstream overwrote them in place through a cast); it binds
  `glpic[gltexcache(...)]` (our cache returns an index); `drawmapview` sets
  `globalpal` to the floor's `floorpal`, or the sprite's `pal`, before each
  fill in renderer 3 (upstream used whatever was left in it); a
  relative-aligned floor takes its texture origin from `startwall`'s vertex
  even when the loop skipped it as collinear (upstream used the first kept
  point, which shifts the texture, as on E1L3's sector 107); the alpha test
  gets `glAlphaFunc(GL_GREATER,0.32f)`, as nothing else here sets one, and
  is turned off after the draw so the next 3D frame doesn't inherit it;
  `_glframedrawn()` is called on every fill; the projection pop is under
  `rendmode == 3` and goes back to `GL_MODELVIEW`. With non-power-of-two
  textures, `polymost_fillpolygon` scales u and v by the power-of-two size
  `gloadtile` would have padded to over the tile's size, after taking whole
  repeats off at the polygon's middle (classic fixed point drops them,
  which only a power-of-two texture hides), so a tile that isn't a power of
  two matches its drawing without them within the repeat at the polygon's
  middle (other repeats are phase-shifted). Both scales take an empty tile
  (an animated frame of size 0, which reaches the fill as `drawmapview`
  checks the size before `animateoffs`) as size 1, as `drawpoly` and
  `gloadtile` do. Not taken: the `glpolygonmode` debug
  variable, the `clearview`/`clearallviews` colour fix (we already divide
  by 255), and the `winlayer.c` part.
- `80878afe29b09f07c3ca8a4aecdc54dc7905f571`: map view translucency. In
  every renderer, `drawmapview` sets `settransreverse()` or
  `settransnormal()` from a translucent floor sprite's cstat 512 before
  `tsethlineshift`, so the classic renderer no longer draws it with
  whatever translucency the last draw left set (a classic bug fix, kept on
  purpose), and sets `globalorientation` from cstat 2 and 512 for
  `polymost_fillpolygon`, which draws at alpha 1, 0.66 or 0.33 from it.
  Change from upstream, as its later form does: `GL_BLEND` is off for
  opaque fills, and it is turned off again after every fill.
  render_classic2d's pinned map checksums don't change, as its floor sprite
  is translucent without cstat 512 and the classic state left before it is
  already normal; with cstat 512 added, the 320x200 map checksum goes from
  `763b8c56` (without the fix) to `c9790d70`.
- `a031bac10735daefb0bbb4b379cf11460270729c` (the fullbright feature only, not the 2009 texture cache
  refactor around it): fullbright texels in renderer 3. `gloadtile` builds,
  beside each cached texture, a glow texture (`glpicfb[]`, 0 for none) of
  the texels whose palookup-mapped index is in `[polymosttexfullbright,
  255)`, transparent elsewhere, with the base texture's filter and wrap; a
  reload that finds none deletes it, as `polymost_glreset` does all of
  them, and `gltexapplyprops` updates them too. `polymosttexfullbright`
  (256, off) is in `build.h`. `drawpoly` given `METH_LAYERS` (8) draws the
  polygon again with the glow texture at colour (1,1,1,alpha), fog off,
  blended and without depth writes, then restores the base pass's state;
  `fog_is_on` follows every fog enable and disable. A failed glow-layer
  allocation leaves the base texture without one. `domost`,
  `polymost_drawmaskwall` and `polymost_drawsprite`'s face, wall and floor
  sprites pass it; `dorotatesprite` doesn't. Changes from upstream:
  `drawpoly` strips the flag on entry, so nothing else (renderer 2
  included) sees it; the parallaxed sky gets no glow pass (upstream left out
  only hightile skyboxes); in the non-power-of-two wall split each piece
  draws its base and then its glow (upstream's layer mask emptied on the
  first piece, so later pieces drew nothing). The game sets the threshold.
- Widescreen and tallscreen, in every renderer, in their final upstream form:
  `6b98b22a07e5101fde6942935418c4b7b94aff4e`,
  `0187a80f5327b17bc54251dbc9f5ae4075a8db92`,
  `2c95ef6f945cb5a6b8fa48f437de299538a4177f`,
  `7703ab676f40414ae84c7b0249b8a5841afcbcb6`,
  `9691f25ad865c9af119cbdea1f1633ba58b1ca1d`,
  `88007d9b44962412a06a9e4f19316f6b4872bb13`, with
  `5e8446845831421ed7a0e858baff081118c89d8c` (no wider field of view at
  320x200 and 640x400), `11d083e8521e42d5885ee7583979bf8949dd91e1`
  (rotatesprite's full-precision widescreen scale) and
  `dbee0ff2c358acfcb1d974a254f2b2f973711095` (exported to the game).
  `engine.c`'s new `calcpixelaspect()` (prototyped in `engine.h` and
  `engine_protos.h`) sets `pixelaspect` to 65536 at 320x200 and 640x400 and
  to 1.2 (78643) otherwise; a mode wider than 4:3 sets `widescreen`, a
  taller one sets `tallscreen` and `pixelaspect = divscale16(ydim*320,
  xdim*200)`. Upstream did this in `setgamemode`; we have none, so
  `sdl_driver.c`'s `init_res_lookups` calls it after the vidoption 2/6
  switch to 320x200 and before `setview`, for the 8-bit and GL mode sets
  and GL's resize. `setview` calls `setaspect(mulscale16(65536, xfov),
  pixelaspect)`, `xfov` being `divscale16(xdim*3, ydim*4)` in widescreen
  and 65536 otherwise, and `setaspect` sets `ydimenscale = scale(ydimen,
  yxaspect, mulscale16(200, pixelaspect))`. In widescreen,
  `dorotatesprite` and `polymost_dorotatesprite`'s auto-scaled
  (`dastat&2`) sprites scale with the view's or screen's height
  (`ydimenscale`; `scale(ydim<<16, yxaspect, 200*pixelaspect)`) and centre
  on its width, so the 320x200 HUD is pillarboxed at 4:3 instead of
  stretched; otherwise they run the old code, `stereomode` line included.
  `xyaspect`, `pixelaspect`, `widescreen`, `tallscreen` and `ydimenscale`
  are in `build.h`; `pixelaspect` starts at 65536 (upstream 0), so a
  `setaspect` before the first mode set doesn't divide by 0. At 320x200,
  640x400, every exact 4:3 mode and tallscreen modes `setaspect` gets what
  it got before (65536 and `divscale16(ydim*320, xdim*200)`), so they draw
  as before. Not taken: `MAXXDIM`/`MAXYDIM` (ours are bigger), the new
  default modes, `7703ab6`'s `slopalookup` size and assert, and the
  editor's `qsetmode` parts.
- `2a8cfbb54ac46481276b78411d41285d9daed2ba`: `dastat` is an `unsigned
  short` in `rotatesprite`, `permfifotype`, `dorotatesprite` and
  `polymost_dorotatesprite` (and `engine.h`'s and `engine_protos.h`'s
  prototypes), and `dastat&256` with `dastat&2` marks a tile with square
  pixels: it gets no `xyaspect` correction, and is drawn 1.2 times as wide
  when `pixelaspect` is 65536 (320x200 and 640x400). Duke passes no bit
  over 128 (its callers' `char` and `short` `dastat`s are all at most 255),
  so nothing it draws changes.

## Engine hooks

- `engine.c`: `#include "polymost.c"` after `wallfront`/`bunchfront`, so the
  statics it uses are defined; `rendmode = 0`, `curpalette[]`, `palfadergb`,
  `palfadedelta` and no-op `begindrawing()`/`enddrawing()` macros before it;
  `hitallsprites`, with `hitscan` skipping its sprite clip-mask test when it's
  set (as JFBuild does).
- `build.h`: `palette_t`, `rendmode`, `curpalette`, `palfadergb`,
  `palfadedelta`, `bpp`. `engine_protos.h`: the Polymost entry points.
- `engine.c` draws through Polymost when `rendmode` is not 0, at JFBuild's
  sites: `drawrooms` calls `polymost_drawrooms()` and returns after the
  `umost`/`dmost` setup; `drawsprite`, `drawmaskwall` and `dorotatesprite`
  hand over to their `polymost_` versions; `completemirror` does nothing;
  `setviewtotile` saves `rendmode` and draws to the tile with the classic
  renderer, and `setviewback` restores it and calls `invalidatetile`.
  `setviewtotile` also sets the tile's `picsiz` from its new size (not
  upstream), as Polymost addresses a power-of-two tile through it.
- `engine.c`: `setrendermode` (0-3; 3 needs `bpp > 8`, and a GL mode always
  gets 3; returns the renderer set, where JFBuild returns 0), `getrendermode`,
  `setrollangle` and `invalidatetile`, from JFBuild. Their prototypes are in
  `engine.h` and `engine_protos.h`, without `POLYMOST`, so callers needn't
  check it.
- `sdl_driver.c`: `bpp`, 8 until an OpenGL video mode sets it.
- `source/astub.c` (the editor, not vendored) had its own global `curpalette`;
  it's renamed `curpalettenum`.

## OpenGL video mode (renderer 3)

- `build.h`: `MAXXDIM` 7680 and `MAXYDIM` 4320 (were 1600 and 1200, and
  still are without `POLYMOST`), room for 8K drawables. No array sized by
  them is saved, recorded in demos or sent over the network.
- `sdl_driver.c`: `setgamemode_gl(davidoption)`, prototyped in `display.h`
  and `engine_protos.h`. The first time, it makes a new `SDL_WINDOW_OPENGL |
  SDL_WINDOW_ALLOW_HIGHDPI` window (the last 8-bit window's windowed size,
  kept by `go_to_new_vid_mode`, or the size `_setgamemode` was asked for
  when it clamped it, and the current fullscreen state) with a legacy context (double buffered, 24-bit depth, swap
  interval 1 or else 0), and only then drops the 8-bit window, renderer,
  texture and surfaces, so any failure returns -1 with the mode untouched.
  The window and context then stay until shutdown: later calls set the
  window's size again and reuse them. It sets JFBuild's GL state, fills `glinfo` from whole
  `GL_EXTENSIONS` tokens, sets `bpp = 32`, `xdim`/`ydim` to the drawable
  size clamped to `MAXXDIM`x`MAXYDIM` (on the shader path with an internal
  size, that size: see `glext.c`), `qsetmode = 200` and `frameplace = 0`,
  then calls `setrendermode(3)` and `polymost_glreset()`. Without
  `USE_OPENGL` it returns -1 with an SDL error set.
- `init_new_res_vars` is split: `init_res_lookups` (`ylookup`,
  `horizlookup` through `screen`, `setview`, brightness) is shared with GL,
  where `screen` holds only the lookups and nothing is cleared.
- Leaving GL (any 8-bit mode set, or shutdown) calls `polymost_glreset()`
  and sets `bpp = 8` and `setrendermode(0)`; only shutdown deletes the
  context and window. JFBuild resets the texture cache the same way before
  and after each GL mode change. Once there is a context, the 8-bit modes
  (renderers 0-2, the editor's 2D mode) are shown through it instead of an
  SDL_Renderer: the surface is blitted through its palette to RGBA,
  uploaded to a texture (power-of-two sized without
  `GL_ARB_texture_non_power_of_two`) and drawn with `gl_fitrect`'s
  letterboxing, nearest pixel, into the drawable; mouse motion is scaled
  from window points to the frame's pixels, as the SDL_Renderer's logical
  size did.
- In GL: `_nextpage` swaps; the fullscreen toggle and
  `SDL_WINDOWEVENT_SIZE_CHANGED` re-read the drawable size and, if it
  changed, call `polymost_glreset()` and redo the mode variables. With no
  frame being drawn, the last frame is drawn again, stretched to the new
  size, and swapped, so a still screen (which isn't redrawn) and its fades
  survive. At an internal size only where the frame shows changes (and a
  still screen is shown again).
  `setupmouse`, the mouse motion clamp and `_idle` check `sdl_window`
  instead of `surface`, and the clamp and `setupmouse`'s centre use the
  window's size, as SDL's mouse events are in points, not the (on a
  high-DPI display, bigger) drawable's pixels. `getmousearea` (ours, for
  the editor's cursor) is the window's size, or at an internal size the
  frame's part of it.
- `_setgamemode` sizes the window (or exclusive's display mode) as asked.
  8-bit modes draw at that size, over 1600x1200 clamped to it (the
  editor), or with `setvideodrawfit(1)` (the game) at that size or in
  borderless at the window's (the desktop), capped to 1600x1200 keeping
  the aspect (`videoopts_drawsize`, so `videoopts.c` is in this library);
  shown scaled. The larger `MAXXDIM`/`MAXYDIM` only reach GL. The 8-bit
  surface has its own pixels with rows exactly its width apart, so any
  width works (SDL pads its own to 4). `setvideodisplaymodenext` gives the
  next mode set a display mode, so a new mode and size change the screen
  once. A mode set leaves the window's size, position and display mode
  alone when they haven't changed (a moved window stays put, also when the
  first GL window replaces it); a new windowed size is given only while
  windowed (in full screen it waits for the return) and centred on the
  bounds of the display the window was on. Exclusive takes the main
  display's mode exactly the size asked for (the closest only when there's
  none), and full screen moves a window on another display to the main one
  first (untested with two displays). `getvideomodelist` gives the main
  display's modes and desktop size (the VIDEO page's RESOLUTION list).
  `setvideotogglehook` hands Alt+Enter's display mode to the game (a mode
  set) instead of changing it at once; `setvideodisplaymodenext` keeps a
  full-screen style for Alt+Enter, not windowed.
- `setvideofadeout`/`setvideofadein` (the VIDEO page's fade around a
  change): a darkness applied at every present, in real time, over the
  game's palette and fade without touching them. The SDL renderer's texture
  colour mod, `gl_present8`'s colour, a black quad after the fixed function
  tiers' fade quad, and on the shader tier the palette
  `polymost_present_shader` shows through (its new `dark` argument).
  Black held over 2 * `VIDEOFADE_MS` with no `setvideofadein` fades back
  in by itself. Captures leave it out, except a front-buffer read.
- `glext.c`/`glext.h` (ours, not JFBuild's): renderer 3's draw paths
  (`GLPATH_AUTO`, `IMMEDIATE`, `VBO`, `SHADER`) and their setting names,
  the pure `gl_choosepath`, and the GL entry points past 1.1 as `bgl…`
  pointers from `SDL_GL_GetProcAddress` (`glActiveTexture` at GL 1.3 or
  `GL_ARB_multitexture`; vertex buffers at GL 1.5 or
  `GL_ARB_vertex_buffer_object`; GLSL at GL 2.0; FBOs from
  `GL_ARB_framebuffer_object` or `GL_EXT_framebuffer_object`), each group
  all loaded or all NULL, so the binary runs on a 1.1 driver.
  `gl_has_extension` moved there from `sdl_driver.c`. `setgamemode_gl` calls
  `gl_initdrawpath`, which sets `glinfo.vbo`/`glsl`/`fbo`, picks `gldrawpath`
  from `gldrawpathrequest` (the game's `GLDrawPath`, the editor's
  `gldrawpath`) and prints it. `gl_forcefail` (a test hook): 1 makes GLSL
  and FBOs unavailable, 2 breaks the shader path's shader, 3 leaves its
  framebuffer incomplete (5 and 6: below). `glinternalwidth`/`glinternalheight` (the game's
  `GLInternalWidth`/`Height`, the editor's `glinternalwidth`/`height`) set
  the shader path's internal size: `gl_internalsize` takes both or neither
  (one alone is the window's size) and at least 320x200, and
  `setgamemode_gl` prints why when it changes them, or ignores them (with
  a message) off the shader path; if the shader path then fails (built at
  once by `polymost_prepare_shader`) it draws at the window's size.
  `xdim`/`ydim` (and `xres`/`yres`, the framebuffer) are that size, so the
  game lays out as classic does at it; `polymost_present_shader` draws the
  frame into `gl_fitrect`'s rectangle of the drawable (the largest scale
  that fits, square pixels, centred, nearest texel) with black bars around
  it; screenshots are the internal-size frame. `glsupersample` (the
  game's `Supersampling` through `setvideosupersample`, the editor's
  `supersampling`) is the shader path's supersampling level: without an
  internal size, `init_gl_res_vars` makes `xdim`/`ydim` `videoopts_sssize`'s
  size for the drawable (1440/2160/4320 high at its aspect), clamped to
  the largest frame (GL's texture, renderbuffer and viewport limits and
  `MAXXDIM`; `gl_maxfbtest` and `gl_ssclamps` are test hooks) with a
  message, or the drawable's size when that isn't larger than it.
  `polymost_present_shader` then draws with `glshdownfrag`: each pixel the
  mean of the texels it covers through the shown palette (fade, contrast,
  saturation and the VIDEO fade included), weighted by coverage. That
  program is linked on its own when first wanted
  (`polymost_supersample_ready`): if it doesn't link, supersampling isn't
  offered and the shader path stays (`gl_forcefail` 5 tests it). Its loop
  takes at most 16 texels a side, so the size is also held to
  `GLSHDOWNMAXSCALE` (14) texels to a pixel. An internal size larger than
  the window still shows through `glshresolve`, nearest.
  Screenshots are scaled down the same way on the CPU (`gl_boxdown`), at
  the window's size. A resize, or `setvideosupersample` (`gl_remake`, the
  resize's rebuild), works the size out again; if the framebuffer fails at
  it, the vbo path draws at the window's size. No `lastframe` texture is
  made at a supersampled size.
  `gltexturefilter` (the game's `GLTextureFilter` through
  `setvideotexfilter`, the editor's `gltexturefilter`) is renderer 3's
  texture filter: nearest, bilinear or trilinear. The 1.1/1.5 paths map it
  to `gltexfiltermode` (`GL_NEAREST`, `GL_LINEAR`,
  `GL_LINEAR_MIPMAP_LINEAR`; their textures always have mipmaps) and
  `gltexapplyprops` sets it on the textures made. On the shader path
  nearest is the palette-index frame above, unchanged; bilinear and
  trilinear draw colour into the same framebuffer instead (`glshcolor`):
  each tap of a tile is its index looked up at the pixel's shade (as the
  index program works it out) in `glshrgb`, the palette's colour of every
  palookup row, so classic's shade tables stay exact, then blended. Masked
  taps of 255 count as uncovered, and a pixel less than half covered is
  discarded. Trilinear adds mip levels made per tile (`polymost_idxmip`):
  four indices a texel, one for each texel of the level above it covers
  (at level 1 the tile's own; deeper the one of that texel's four nearest
  their mean colour), side by side in one texture and addressed in the
  shader, which picks and blends two levels by the pixel's texel
  footprint. Translucent draws blend at 0.66 and 0.33, and the present
  (`glshcresfrag`, `glshcdownfrag` when supersampled) moves the colours
  toward the fade's as `getfadedpalette` moves the palette, then applies
  contrast, saturation and the VIDEO fade: approximations of classic's
  `transluc` and faded palette. Lines, text and clears draw the palette's
  colour. Screenshots read the colour and fade it the same way on the CPU.
  A switch between nearest and the others clears the frame to black. The
  colour programs link on their own when first wanted
  (`polymost_clink`), each failing alone: bilinear or trilinear that
  doesn't link isn't offered (the other still is), a colour present that
  doesn't link takes both, and the shader path draws nearest; a
  supersampled colour present (`glshcdownfrag`) that doesn't link takes
  filtering only while supersampled, so supersampling keeps working at
  nearest (`gl_forcefail` 6, 7 and 8 test them).
- `glsubmit.c`/`glsubmit.h` (ours): renderer 3's vertex submission.
  `polymost.c`'s and `engine.c`'s `glBegin`/`glTexCoord`/`glColor`/
  `glVertex`/`glEnd` (drawpoly, `drawtrap`/`tessectrap`, the tile picker,
  `drawline256`, `printext256`, `plotpixel`) are `glsub_*` calls, as floats
  (`glTexCoord2d`/`glVertex3d` and `glColor4ub` were). On the immediate path
  they become vertex arrays (`glsub_force`, a test hook, can keep
  `glBegin`/`glEnd`); on the vbo path a vertex buffer, given new storage
  each draw; on the shader path vertex arrays again (faster than that
  buffer on macOS's driver). `glsub_flusheach` (a test hook) draws each
  polygon at its end. Fans become triangles so polygons batch; `polymost.c`
  and `sdl_driver.c` define `GLSUB_WRAP`, which wraps their GL state calls
  to draw the batch first (binding the bound texture or enabling an enabled
  cap is skipped while batching), and `gl_swap` draws it before the swap.
  `setgamemode_gl` calls `glsub_init`, `gl_leave` `glsub_uninit`.
  The immediate and vbo paths draw the same pixels as JFBuild's immediate
  mode on the drivers tested (macOS's 2.1).
- `polymost.c` (ours): on the shader path (GL 2.1) everything draws
  classic's palette indices into a framebuffer that keeps the frame, as
  classic's does. Tiles are `GL_LUMINANCE8` index textures (their own
  cache, `glidxtex`, kept by `gltexinvalidate`; `polymost_precache` makes
  them, and `premap.c`'s `docacheit` precaches a level's), drawn by a
  program that looks each texel up in `palookup` (a texture of every pal
  and shade, reloaded when `makepalookup` sets `glshlookupdirty`) at the
  polygon's shade plus, in the 3D view, classic's distance shade for the
  pixel's eye z (`glfogshades`; the parallaxed sky's constant
  `polymost_skyshade`), with index 255 see-through when masked and no glow
  pass. Translucent masked walls, sprites, rotatesprites and map view
  floor sprites blend as classic's do, through `transluc` (a 256x256
  texture) at its two levels, with the index under them read from a copy
  of the framebuffer (`glshdst`): `polymost_transread` copies the
  whole frame before the polygon is drawn unless the copy holds the
  polygon's box with nothing drawn there since (`polymost_drawn` keeps the boxes drawn after a copy;
  lines, text and clears drop it). That's the 3D view, rotatesprite, the map view's floors and the
  tile picker; lines, text, pixels and clears are the index itself
  (`polymost_2dcolor`). `polymost_shaderdraw` binds the framebuffer and
  the program for each; `polymost_drawrooms` clears only its depth, so a
  hole shows what was there, as in classic. `polymost_present_shader`
  draws the frame into the window through `getfadedpalette` (engine.c:
  curpalette, or the base palette faded as `setpalettefade` fades the
  8-bit one); `polymost_capture_shader` reads it the same way for
  screenshots; `polymost_takeframe_shader`/`polymost_putframe_shader` keep
  it, stretched, across a resize. A shader that doesn't compile or link,
  or an incomplete framebuffer, prints why and falls back to the vbo path
  (retexturing, as palette changes don't on the shader path).

## Renderer 3: 2D, clears, palette, fades, screenshots

- `engine.c`: JFBuild's renderer 3 branches, under `rendmode == 3` (not
  `qsetmode == 200`): `clearview` (scissored to the view window, as in
  classic; upstream clears it all) and `clearallviews` (the whole back
  buffer) clear to the palette colour; `plotpixel`, `drawline256` and
  `printext256`/`printext256_noupdate` (through `__printext256`) draw GL
  points and lines; `getpixel` returns 0. (`drawmapview` returned at once
  here until the ccf7aa8 port below.)
  Changes from upstream: `glClearColor` gets the colour over 255 (upstream
  passes 0-255, which clamps to white); points are placed on pixel centres
  (upstream uses integer corners, plus a "+2 hack" row offset for text).
- `engine.c`: `setpolymost2dview()` from JFBuild, prototyped in `engine.h`
  and `engine_protos.h`. It also turns fog off (not upstream);
  `resizeglcheck`, `polymost_drawmaskwall` and `polymost_drawsprite` turn
  it on again.
- `engine.c`: `setbrightness` keeps the palette it's given (`basepal`) and
  clears `palfadergb`/`palfadedelta`, as JFBuild's does. New
  `setpalettefade(r, g, b, offset)` (0-63, offset 0-64): it stores
  `palfadergb = {r<<2, g<<2, b<<2}` and `palfadedelta = min(offset<<2,
  255)`; outside renderer 3 it blends `basepal` the way the game's `palto`
  does and applies the brightness table; in renderer 3 it calls the
  driver's `_glpalettefade()`. Without `POLYMOST` only the blend is built.
- `polymost.c`: `polymost_drawrooms` and `polymost_dorotatesprite` call
  `_glframedrawn()` in renderer 3. So do `clearview`, `clearallviews` and
  `setpolymost2dview`.
- `sdl_driver.c`, in GL:
  - `VBE_setPalette` turns the 6-bit BGR0 entries into 8-bit `curpalette`
    (`v<<2 | v>>4`) and calls `gltexinvalidateall()`; `VBE_getPalette`
    reads `curpalette` back.
  - `_nextpage` copies a drawn frame into a `lastframe` texture (power-of-two
    size, made per mode, none if over `GL_MAX_TEXTURE_SIZE`), draws the
    `palfadergb` quad at `palfadedelta`/256 when it's set (JFBuild's
    `showframe`), swaps and clears `framedrawn`.
  - `_glpalettefade()`: with nothing drawn since the swap it draws
    `lastframe` and the fade quad and swaps, so a still title screen can
    fade; otherwise the next `_nextpage` applies the fade. `nextpage`'s
    permanent-sprite replay after the swap (a dastat 128 sprite drawn
    before any `drawrooms`, such as E1's ending screen) doesn't count as
    drawing: `nextpage` calls `_glframereplayed()` (prototyped in
    `display.h`) after it, which clears `framedrawn`.
  - `screencapture`'s RGB comes from the back buffer. A frame being drawn
    is there; otherwise `lastframe` is drawn there first, as the back buffer
    is undefined after a swap, and with no frame in `lastframe` yet the
    capture fails. The current fade is blended in as the fade quad does, so
    screenshots show it as the 8-bit modes' do (see the stb_image_write
    section). With no `lastframe` texture (over `GL_MAX_TEXTURE_SIZE`) it
    reads the front buffer instead, which has the fade.
  - `numpages` is 2, as in JFBuild's GL mode, and back to 0 on leaving GL.
  - `glredrawlastframe()` (not in JFBuild; prototyped in `display.h` and
    `engine_protos.h`) draws `lastframe`, without the fade, into the frame
    being drawn. Duke's 1.3D load/save menus draw over the previous frame,
    which renderer 3 doesn't keep after a swap.
  - On the shader path none of the `lastframe` and fade quad steps happen:
    `gl_swap` and `_glpalettefade` show the framebuffer through the faded
    palette (`polymost_present_shader`), which keeps the frame for fades,
    `glredrawlastframe` and screenshots; `VBE_setPalette` doesn't
    retexture; `_glframereplayed` binds the window's framebuffer again.

# Vendored: stb_image_write.h

- Upstream: <https://github.com/nothings/stb>, `stb_image_write.h`
- Commit: `a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d` (2026-10-19), v1.16
- License: public domain / MIT (the file's choice), unchanged.

Unmodified. `sdl_driver.c` defines `STB_IMAGE_WRITE_IMPLEMENTATION` and
includes it. `screencapture` replaces the icculus port's PCX writer (the
`pcxheader`, `write_pcx_plane` and the run-length loop are gone): it builds
top-down 24-bit RGB, and the file name's extension picks `stbi_write_jpg`
(quality 90), `stbi_write_bmp` or, for anything else, `stbi_write_png`, all
through `_to_func` onto the file it opened with `O_EXCL`. The 8-bit modes use
the surface's palette (the faded one on screen); the editor's 640-wide 2D
mode uses the 16-colour `vgapal16` and still honours `inverseit`. `build.c`'s
editor captures are named `captxxxx.png`. If there's no picture, the encoder
fails or a write fails, it removes the file and returns -1.

## 0.99 beta hitscan

`BUILD_ENGINEREV < 19960129L` retains the beta's slope-plane intersections behind
ray origins and halves signed height differences before taking their absolute value
in all four plane range tests. E099-001/002, measured from the Jan4 DOS EXE; guarded
branches in `engine.c:hitscan`, cases in `tests/engine_trace.c`. All later revisions
retain their existing sign/range checks.
