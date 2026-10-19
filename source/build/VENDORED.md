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
