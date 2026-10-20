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

## Local changes

- `VENDORED.md`: added (this file).
- `CMakeLists.txt`: added (build glue).
- `win32_compat.c`: added. Defines `hbits[]`, `_inp_handler()` and
  `_kinp_handler()` for Windows; upstream only defines them in
  `unix_compat.c`, which refuses to build without `PLATFORM_UNIX`.
- `win32_compat.h`: don't include `<dos.h>` under MSVC (it no longer ships
  one); only map `snprintf` to `_snprintf` before VS2015; under MSVC, define
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
  `~(intptr_t)15`
- `build.h`: `BUILD_STATIC_ASSERT` checks `sectortype`/`walltype`/`spritetype`
  are 40/32/44 bytes
- printf formats match their now-32-bit arguments; C-library narrowings
  (`read`, `lseek`, `sendto`, `recvfrom`, `time`, `strtol`, `strlen`) are
  explicit casts
- `mmulti.c`: `socklen_t` is `int` on Windows (it was `size_t`, which is 8
  bytes on Win64 while Winsock's `recvfrom` takes an `int *`)
- `pragmas.h`: `klabs` negates as unsigned, so `klabs(INT32_MIN)` gives
  `INT32_MIN` as x86 `neg` does instead of undefined behaviour (ledger E-009)
- `engine.c`: version-guarded 1.4 engine logic, on `BUILD_ENGINEREV` (see
  ledger E-005)
- `engine.c`, `cache1d.c`: version-guarded 1.3D engine logic, on
  `BUILD_ENGINEREV`: the 1996 logic square root, slope maths, `inside` and the
  1996 LZW format (see ledger E-001 to E-004, E-012)
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
  versions are gone). It truncates where x87 `fistp` rounds.
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
  texcompr; }`, which upstream got from `baselayer.h`) is declared in
  `build.h` under `USE_OPENGL`, not here. `sdl_driver.c` defines it, filled in
  by `setgamemode_gl`, and `nofog` (which upstream declares but no JFBuild
  file of that commit defines), set to 0.
- The forward declaration of `wallfront` is gone: ours is defined, non-static,
  before the `#include`.
- `polymost_dorotatesprite` takes `unsigned char dapalnum, dastat`, as our
  `dorotatesprite` does.

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
- `engine.c`: `setrendermode` (0-3; 3 needs `bpp > 8`, and a GL mode always
  gets 3; returns the renderer set, where JFBuild returns 0), `getrendermode`,
  `setrollangle` and `invalidatetile`, from JFBuild. Their prototypes are in
  `engine.h` and `engine_protos.h`, without `POLYMOST`, so callers needn't
  check it.
- `sdl_driver.c`: `bpp`, 8 until an OpenGL video mode sets it.
- `source/astub.c` (the editor, not vendored) had its own global `curpalette`;
  it's renamed `curpalettenum`.

## OpenGL video mode (renderer 3)

- `build.h`: `MAXXDIM` 7680 and `MAXYDIM` 4320 (were 1600 and 1200), room
  for 8K drawables. No array sized by them is saved, recorded in demos or
  sent over the network.
- `sdl_driver.c`: `setgamemode_gl(davidoption)`, prototyped in `display.h`
  and `engine_protos.h`. It makes a new `SDL_WINDOW_OPENGL |
  SDL_WINDOW_ALLOW_HIGHDPI` window (the last 8-bit window's windowed size,
  kept by `go_to_new_vid_mode`, and the current fullscreen state) with a legacy context (double buffered, 24-bit depth, swap
  interval 1 or else 0), and only then drops the 8-bit window, renderer,
  texture and surfaces, so any failure returns -1 with the 8-bit mode
  untouched. It sets JFBuild's GL state, fills `glinfo` from whole
  `GL_EXTENSIONS` tokens, sets `bpp = 32`, `xdim`/`ydim` to the drawable
  size clamped to `MAXXDIM`x`MAXYDIM`, `qsetmode = 200` and `frameplace = 0`,
  then calls `setrendermode(3)` and `polymost_glreset()`. Without
  `USE_OPENGL` it returns -1 with an SDL error set.
- `init_new_res_vars` is split: `init_res_lookups` (`ylookup`,
  `horizlookup` through `screen`, `setview`, brightness) is shared with GL,
  where `screen` holds only the lookups and nothing is cleared.
- Leaving GL (any 8-bit mode set, or shutdown) calls `polymost_glreset()`
  while the context is current, deletes the context and window, and sets
  `bpp = 8` and `setrendermode(0)`. JFBuild resets the texture cache the
  same way before and after each GL mode change.
- In GL: `_nextpage` swaps; the fullscreen toggle and
  `SDL_WINDOWEVENT_SIZE_CHANGED` re-read the drawable size and, if it
  changed, call `polymost_glreset()` and redo the mode variables.
  `setupmouse`, the mouse motion clamp and `_idle` check `sdl_window`
  instead of `surface`, and the clamp uses `xres`/`yres`.
- `_setgamemode` (8-bit) still clamps modes over 1600x1200 to 1600x1200,
  now with literals, so the larger `MAXXDIM`/`MAXYDIM` only reach GL.

## Renderer 3: 2D, clears, palette, fades, screenshots

- `engine.c`: JFBuild's renderer 3 branches, under `rendmode == 3` (not
  `qsetmode == 200`): `clearview` and `clearallviews` clear the whole back
  buffer to the palette colour; `plotpixel`, `drawline256` and
  `printext256`/`printext256_noupdate` (through `__printext256`) draw GL
  points and lines; `getpixel` returns 0; `drawmapview` returns at once.
  Changes from upstream: `glClearColor` gets the colour over 255 (upstream
  passes 0-255, which clamps to white); points are placed on pixel centres
  (upstream uses integer corners, plus a "+2 hack" row offset for text).
- `engine.c`: `setpolymost2dview()` from JFBuild, prototyped in `engine.h`
  and `engine_protos.h`.
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
    fade; otherwise the next `_nextpage` applies the fade.
  - `screencapture` writes a 24-bit PCX (three planes per line, as JFBuild's
    PCX writer): from the back buffer if a frame is being drawn, otherwise
    from `lastframe`, as the back buffer is undefined after a swap. The
    fade isn't in it.
  - `numpages` is 2, as in JFBuild's GL mode, and back to 0 on leaving GL.
  - `glredrawlastframe()` (not in JFBuild; prototyped in `display.h` and
    `engine_protos.h`) draws `lastframe`, without the fade, into the frame
    being drawn. Duke's 1.3D load/save menus draw over the previous frame,
    which renderer 3 doesn't keep after a swap.
