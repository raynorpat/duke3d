# CMake Build System + SDL2 Port — Design

Date: 2026-10-18
Status: Approved (brainstorming), pending implementation plan

## Goal

Replace the GNU makefiles and Visual Studio projects with a single CMake build that
compiles and links `duke3d` and the `build` map editor as 64-bit binaries on Windows
(MSVC), macOS (arm64), and Linux, using SDL2 + SDL2_mixer.

## Scope

In scope:

1. Vendor icculus's Build engine into `source/build/`.
2. CMake build system (libraries, executables, options, presets).
3. Port SDL 1.2 usage to SDL2 / SDL2_mixer.
4. GitHub Actions build matrix for Windows, Linux, macOS.
5. Remove obsolete build files; document the new build in `README.md`.

Out of scope (follow-up spec): 64-bit runtime correctness. The code base uses `long`
heavily; on arm64 a syntax probe showed ~1,500 `-Wshorten-64-to-32` warnings in
`engine.c` and ~670 across the game sources. These are left visible as warnings and are
not fixed here. A crash during gameplay on 64-bit is an expected, known issue for this
spec.

Also out of scope: the OpenGL renderer (`buildgl.c` / `USE_OPENGL`), Perl and PhysicsFS
support, GameController API migration, any DOS/Watcom support changes, and `extras/`.

## Layout and targets

```
CMakeLists.txt                  project(duke3d C), options, includes cmake/Dependencies.cmake
CMakePresets.json               "default" (RelWithDebInfo) and "debug" presets, binaryDir build/
cmake/Dependencies.cmake        SDL2 / SDL2_mixer: find_package, FetchContent fallback
source/CMakeLists.txt           duke3d and buildeditor executables
source/audiolib/CMakeLists.txt  audiolib STATIC library
source/build/                   vendored engine sources (see Vendoring)
source/build/CMakeLists.txt     buildengine STATIC library
.github/workflows/build.yml     CI matrix
```

Targets:

- `duke3d_compile_options` (INTERFACE): shared compile flags and definitions; linked by
  every target below.
- `buildengine` (STATIC): `engine.c cache1d.c sdl_driver.c mmulti.c a.c pragmas.c`,
  plus `unix_compat.c` on Linux/macOS or a new `win32_compat.c` on Windows. Links
  `duke3d::SDL2` PUBLIC. `unix_compat.c` `#error`s without `PLATFORM_UNIX`, yet
  `win32_compat.h` declares the `hbits[]` table that only `unix_compat.c` defines. The
  new `win32_compat.c` defines that table for Windows, plus any other symbols the
  Windows link step reports missing. It's a local addition, recorded in `VENDORED.md`.
- `audiolib` (STATIC): `fx_man.c dsl.c ll_man.c multivoc.c mv_mix.c mvreverb.c nodpmi.c
  pitch.c user.c usrhooks.c`, the same set the old Makefile and `.vcproj` used. Links
  `duke3d::SDL2_mixer`.
- `duke3d` (executable): `actors.c animlib.c control.c config.c game.c gamedef.c
  global.c keyboard.c menues.c player.c premap.c rts.c scriplib.c sector.c sounds.c
  dukemusc.c`. Links `buildengine`, `audiolib`, `duke3d::SDL2_mixer`, `duke3d::SDL2`,
  plus `SDL2::SDL2main` on Windows.
- `buildeditor` (executable, `OUTPUT_NAME build`): `astub.c` plus the engine's `build.c`.
  Links `buildengine`. The engine's own `bstub.c` is excluded because `astub.c` is
  Duke's replacement for it. The target is not named `build`, so it can't collide with
  the `build/` binary directory or generator-reserved names.

The include root for game code stays `source/`. All `#include "buildengine/X.h"` lines
become `#include "build/X.h"`, and audiolib's `"../buildengine/platform.h"` becomes
`"../build/platform.h"`. This is a plain textual rewrite with no shim headers.

Options:

| Option | Default | Effect |
|---|---|---|
| `DUKE3D_SHAREWARE` | `OFF` | `ON` → `VOLUMEONE`, else `VOLUMEALL` |
| `DUKE3D_CONTROLS_MENU` | `ON` | defines `CONTROLS_CONFIG_MENU=1` |
| `DUKE3D_FETCH_SDL` | `AUTO` | `AUTO`: fetch only if not found; `ON`: always fetch; `OFF`: never fetch (error if missing) |

If `CMAKE_BUILD_TYPE` is unset for a single-config generator, default it to
`RelWithDebInfo`.

## Compiler flags and platform definitions

Definitions:

| Definition | Applied to | Reason |
|---|---|---|
| `USE_SDL=1` | all (PUBLIC on `buildengine`) | SDL code paths |
| `PLATFORM_UNIX=1` | Linux, macOS | `duke3d.h` / `platform.h` test it by value |
| `PLATFORM_MACOSX=1` | macOS (plus `PLATFORM_UNIX`) | `<machine/endian.h>`, no `<malloc.h>`, mmulti tweaks |
| `PLATFORM_WIN32=1` | Windows | `win32_compat.h`, `dukewin.h` |
| `UDP_NETWORKING=1` | `buildengine` | Selects mmulti's transport (otherwise `#error`) |
| `USE_EXECINFO=1` | Linux only | Backtraces, matching the old Makefile |
| `VOLUMEALL` / `VOLUMEONE` | `duke3d`, `buildeditor` | From `DUKE3D_SHAREWARE` |
| `CONTROLS_CONFIG_MENU=1` | `duke3d` | From `DUKE3D_CONTROLS_MENU` |
| `_CRT_SECURE_NO_WARNINGS`, `_CRT_NONSTDC_NO_DEPRECATE` | MSVC | Legacy CRT and POSIX names |

`USE_I386_ASM` is never defined. The engine uses the C implementations (`a.c`,
`pragmas.c`) on every platform, because MSVC x64 has no inline asm and the target CPUs
include arm64.

Flags:

- GCC and Clang: `-funsigned-char -fno-strict-aliasing -W -Wall -Wno-unused`; add
  `-fno-omit-frame-pointer` on `buildengine`.
- MSVC: `/J` (unsigned char), `/W3`, `/wd4996`.
- Optimization comes from the build type.
- `animlib.c` compiles with `-O0` on macOS, matching the old Makefile, which did this to
  work around an optimizer bug in animation playback.
- `C_STANDARD 99`, `C_EXTENSIONS ON`.
- Warnings are not errors.

Platform link libraries:

- Windows: `ws2_32 winmm`
- Linux: `m`
- macOS: none beyond SDL2's own frameworks

## SDL2 port

Most of the work is in `source/build/sdl_driver.c`.

- **Video:** `SDL_SetVideoMode` becomes an `SDL_Window`, an `SDL_Renderer`, and an
  ARGB8888 streaming `SDL_Texture`.
  - The engine keeps drawing into an 8-bit `SDL_Surface`. Each frame is converted to the
    texture, then `SDL_RenderCopy` + `SDL_RenderPresent`.
  - `SDL_RenderSetLogicalSize` keeps the aspect ratio when the window is scaled.
  - `SDL_SetColors`/`SDL_SetPalette` → `SDL_SetPaletteColors`.
  - `SDL_Flip`/`SDL_UpdateRect` → texture update and present.
  - Fullscreen uses `SDL_WINDOW_FULLSCREEN_DESKTOP`; toggling uses
    `SDL_SetWindowFullscreen`.
  - `SDL_WM_SetCaption` → `SDL_SetWindowTitle`.
  - `SDL_ListModes`/`SDL_GetVideoInfo` → `SDL_GetDisplayMode`-based queries.
  - `SDL_LoadBMP` and the `SDL_VideoDriverName` diagnostics map to SDL2 equivalents
    (`SDL_GetCurrentVideoDriver`).
- **Keyboard:** replace the keysym table (sized by `SDLK_LAST`) with an
  `SDL_Scancode` → DOS scancode table sized `SDL_NUM_SCANCODES`. Where the code needs
  typed characters (console, chat, name entry), use `SDL_TEXTINPUT`.
- **Mouse:** `SDL_WM_GrabInput` → `SDL_SetRelativeMouseMode`. Use relative motion
  (`xrel`/`yrel`). `SDL_GetMouseState` button masks are unchanged.
- **Events:** the `SDL_SetEventFilter` callback takes SDL2's
  `int (*)(void *, SDL_Event *)` signature. Focus handling moves from `SDL_ACTIVEEVENT`
  to `SDL_WINDOWEVENT`.
- **Joystick:** `SDL_JoystickName(i)` → `SDL_JoystickNameForIndex(i)`. Everything else
  stays.
- **Timer:** `SDL_AddTimer` is unchanged.

Game and audio side:

- `dukemusc.c`: `Mix_LoadMUS_RW(rw, freesrc)`. If no MIDI backend is available,
  music fails gracefully and the game continues without it.
- `fx_man.c`, `audiolib/dsl.c`: SDL2 version APIs (`SDL_GetVersion`,
  `Mix_Linked_Version`). `Mix_OpenAudio`, `Mix_RegisterEffect`, `Mix_SetPostMix`, and
  `SDL_LockAudio` keep working in SDL2.
- `global.c`, `control.c`, `config.c`, `sounds.c`: include and trivial adjustments only.
- `#include "SDL.h"` / `"SDL_mixer.h"` stay as they are, because the SDL2 targets put
  `include/SDL2` on the include path.

Error handling keeps the existing pattern: on SDL init failure, print `SDL_GetError()` and
exit through the engine's existing fatal-error path.

## Dependencies

`cmake/Dependencies.cmake`:

1. Unless `DUKE3D_FETCH_SDL=ON`, try `find_package(SDL2 CONFIG)` and
   `find_package(SDL2_mixer CONFIG)`.
2. If either is missing and `DUKE3D_FETCH_SDL` isn't `OFF`, use `FetchContent` with
   pinned release tarballs (SDL 2.32.10, SDL_mixer 2.8.1) and `URL_HASH SHA256`.
   - Build both statically.
   - SDL_mixer: `SDL2MIXER_VENDORED=ON`, WAV and MIDI (Timidity) enabled, every other
     format (Opus, FLAC, MOD, MP3, Ogg, WavPack, GME) disabled.
3. Define `duke3d::SDL2` and `duke3d::SDL2_mixer` INTERFACE aliases over whichever
   targets were found (`SDL2::SDL2` / `SDL2::SDL2-static`, `SDL2_mixer::SDL2_mixer` /
   `SDL2_mixer::SDL2_mixer-static`).
4. On Windows with shared SDL, a post-build step copies `$<TARGET_RUNTIME_DLLS:...>` next
   to each executable.

Minimum CMake version: 3.21 (needed for `TARGET_RUNTIME_DLLS` and presets v3).

## CI

`.github/workflows/build.yml`, triggered on push and pull_request:

| Runner | SDL | Generator |
|---|---|---|
| `ubuntu-latest` | `apt-get install libsdl2-dev libsdl2-mixer-dev` | Ninja |
| `macos-latest` (arm64) | `brew install sdl2 sdl2_mixer` | Ninja |
| `windows-latest` | FetchContent fallback | Visual Studio 2022, x64 |

Each job configures with `cmake --preset default` (Windows overrides the generator), builds
both executables, and uploads them as workflow artifacts. Windows is the job that tests the
fetch path.

## Vendoring

- Source: `https://github.com/icculus/buildengine`, commit
  `f3dce0a10bff90e1ed5cf8ec186b3731237a86da` (2021-07-12).
- Copy only the files the targets need: engine sources and headers, `build.c`/`build.h`,
  the compat headers (`unix_compat.*`, `win32_compat.h`, `platform.h`, `display.h`,
  `doscmpat.h`, `a.h`, `pragmas.h`, `cache1d.h`, `engine.h`, `engine_protos.h`,
  `names.h`, `bstub.h`), plus `BUILDLIC.TXT` and `LICENSE.md`.
  - The Build engine is under Ken Silverman's license, not the GPL. The license file
    ships alongside the code.
- Add `source/build/VENDORED.md` with the upstream URL, the commit hash, and the list of
  local changes.
- Commit order: (1) a pure, unmodified copy; (2) the include-path rewrite and CMake;
  (3) the SDL2 port. This keeps diffs against upstream reviewable.

## Removed and kept files

Removed: `source/Makefile`, `source/Makefile.macshipped`, `source/audiolib/Makefile`,
`source/duke3d.sln`, `source/duke3d.vcproj`, `source/duke3d.dsp`, `source/duke3d.dsw`.

Kept unchanged: `source/makefile.dos`, `source/makefile.lnk` (historical Watcom build),
everything in `extras/` and `testdata/`.

`.gitignore`: add `/build/`, anchored to the repo root so `source/build/` isn't
ignored.

## Definition of done

1. All three CI jobs configure and build `duke3d` and `buildeditor` with zero errors.
   Warnings are allowed.
2. On local macOS arm64, `cmake --preset default && cmake --build build` succeeds.
   - With no game data, `./build/source/duke3d` starts and exits cleanly with its
     missing-GRP error.
   - With a `DUKE3D.GRP` present: the SDL2 window opens, the title screen renders with
     correct colors, and keyboard input works in the menus.
   - Crashes past that point that come from 64-bit type issues are documented, not fixed.
3. The obsolete build files are removed; the Watcom files remain.
4. `README.md` has a "Building" section with per-OS prerequisites (Homebrew, apt, or
   nothing on Windows) and the two commands `cmake --preset default` and
   `cmake --build build`.

## Risks

- **64-bit runtime bugs:** expected; handled by the follow-up spec.
- **sdl_driver.c keyboard mapping:** this is where regressions are most likely
  (missing or misnamed keys). Mitigation: build the scancode table from the DOS scancode
  list in `keyboard.h`, then check the menus by hand.
- **Windows-only compile errors** in `win32_compat.h` under modern MSVC x64: CI is the
  only detector. Fixes stay minimal and are recorded in `VENDORED.md`.
- **SDL2_mixer MIDI** on systems without Timidity patches: music is silent and the game
  doesn't fail.
