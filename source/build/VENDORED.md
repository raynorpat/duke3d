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
