# CMake Build System + SDL2 Port — Design

Date: 2026-10-18
Status: Approved (brainstorming); revised after prototype validation, see
"Changes since approval"

## Goal

Replace the GNU makefiles and Visual Studio projects with a single CMake build that
compiles and links `duke3d` and the `build` map editor as 64-bit binaries on Windows
(MSVC), macOS (arm64), and Linux, using SDL2 + SDL2_mixer.

## Scope

In scope:

1. Vendor icculus's Build engine into `source/build/`.
2. CMake build system (libraries, executables, options, presets).
3. Port SDL 1.2 usage to SDL2 / SDL2_mixer.
4. A headless smoke test for the SDL2 driver layer.
5. GitHub Actions build matrix for Windows, Linux, macOS.
6. Remove obsolete build files; document the new build in `README.md`.

Out of scope (follow-up spec): 64-bit runtime correctness. The code base uses `long`
heavily; on arm64 a syntax probe showed ~1,500 `-Wshorten-64-to-32` warnings in
`engine.c` and ~670 across the game sources. The game therefore cannot boot on any
64-bit target yet:

- On LP64 (macOS, Linux), `cache1d.c` reads the GRP header through `*(long *)`, which
  reads 8 bytes instead of 4, and startup fails with "Not enough memory for file
  grouping system". A scratch probe confirmed that fixing that gets past GRP loading.
- On LLP64 (Windows x64), pointers are kept in 32-bit `long` globals (`frameplace`
  among others), so the binaries build but crash at runtime.

These are left as warnings and are not fixed here.

Also out of scope: the OpenGL renderer (`buildgl.c` / `USE_OPENGL`), Perl and PhysicsFS
support, GameController API migration, the `splash.bmp` splash screen, any DOS/Watcom
support changes, and `extras/`.

## Changes since approval

Building a working prototype in a scratch copy (it built and linked on macOS, and the
smoke test passed) turned up the following differences from the approved design:

1. **Runtime "done" (user decision).** The game can't launch on 64-bit targets (see
   Scope), so the runtime proof for this spec is the headless `sdl_driver_smoke` CTest,
   not the game reaching its title screen. That moves to the 64-bit follow-up spec.
2. **Windows support** means it compiles and links in CI. The smoke test is built on
   Windows but not registered as a test there (LLP64 pointer truncation).
3. **No pushing to try CI (user decision).** The Windows/Linux CI task is an
   iterate-on-CI-output loop, seeded with the MSVC fixes already known.
4. **Executables go to `build/bin/`.** The editor's output name `build` collided with
   the `build/source/build` binary directory.
5. **audiolib:** `usrhooks.c` is excluded (`sounds.c` defines the same `USRHOOKS_*`
   functions; the old `.vcproj` excluded it too), and no include directory is exported
   (`audiolib/assert.h` would shadow `<assert.h>` in the game).
6. **Compile flags:** on GCC/Clang, `-Wno-error=` for `implicit-function-declaration`,
   `implicit-int`, `int-conversion`, `incompatible-pointer-types`, and `return-mismatch`.
   Each is added only if the compiler supports it. Clang 16+/GCC 14+ reject the K&R-era
   code in `astub.c` without them.
7. **Game-side files need no SDL2 changes.** `dukemusc.c`'s `Mix_LoadMUS_RW` call is
   commented out, `audiolib/dsl.c` uses APIs that still exist, and `source/fx_man.c`
   isn't built at all (the build uses `audiolib/fx_man.c`). The port is entirely in
   `sdl_driver.c`.
8. **No `SDL_TEXTINPUT`.** `keyboard.c` converts DOS scancodes to ASCII itself.
9. **SDL2-specific driver fixes** found during the port: the timer callback must return
   its interval (SDL2 took the old `return(1)` literally, so 1000 Hz), the mouse wheel
   arrives as `SDL_MOUSEWHEEL`, and `VBE_setPalette` must present immediately because
   `palto()` fades never call `nextpage()`.
10. **Splash screen removed** (`set_splash()` used SDL 1.2's `SDL_NOFRAME` video mode).
11. **SDL_mixer fetch uses `SDL2MIXER_VENDORED=OFF`**. With every external codec
    disabled, nothing needs vendoring.
12. **`win32_compat.h` edits:** skip `<dos.h>` under MSVC; only map `snprintf` →
    `_snprintf` before VS2015.
13. The debug preset uses `build-debug/`, not a directory inside `build/`.

## Layout and targets

```
CMakeLists.txt                  project(duke3d C), options, bin/ output dir, subdirs
CMakePresets.json               "default" (RelWithDebInfo → build/), "debug" (→ build-debug/)
cmake/Dependencies.cmake        SDL2 / SDL2_mixer: find_package, FetchContent fallback
cmake/CompileOptions.cmake      duke3d_compile_options, duke3d_game_options
source/CMakeLists.txt           duke3d and buildeditor executables
source/audiolib/CMakeLists.txt  audiolib STATIC library
source/build/                   vendored engine sources (see Vendoring)
source/build/CMakeLists.txt     buildengine STATIC library
tests/CMakeLists.txt            sdl_driver_smoke test
tests/sdl_driver_smoke.c
.github/workflows/build.yml     CI matrix
```

All executables are written to `build/bin/` (`build/bin/<Config>/` with
multi-config generators).

Targets:

- `duke3d_compile_options` (INTERFACE): shared compile flags and platform definitions.
- `duke3d_game_options` (INTERFACE): `VOLUMEALL`/`VOLUMEONE`, `CONTROLS_CONFIG_MENU`.
- `buildengine` (STATIC): `a.c cache1d.c engine.c mmulti.c pragmas.c sdl_driver.c`,
  plus `unix_compat.c` (Linux/macOS) or the new `win32_compat.c` (Windows). Exports
  its directory as a PUBLIC include dir. Links `duke3d_compile_options` and
  `duke3d::SDL2` PUBLIC.
- `audiolib` (STATIC): `dsl.c fx_man.c ll_man.c multivoc.c mv_mix.c mvreverb.c
  nodpmi.c pitch.c user.c`. Links `duke3d_compile_options` and `duke3d::SDL2_mixer`.
- `duke3d` (executable): `actors.c animlib.c config.c control.c dukemusc.c game.c
  gamedef.c global.c keyboard.c menues.c player.c premap.c rts.c scriplib.c sector.c
  sounds.c`. Links `duke3d_game_options buildengine audiolib duke3d::SDL2_mixer
  duke3d::SDL2main`.
- `buildeditor` (executable, `OUTPUT_NAME build`): `astub.c` plus
  `source/build/build.c`. Links `duke3d_game_options buildengine duke3d::SDL2main`.
  The engine's `bstub.c` isn't vendored, because `astub.c` replaces it.
- `sdl_driver_smoke` (executable + CTest): links `buildengine duke3d::SDL2main`.

The include root for game code stays `source/`. All `#include "buildengine/X.h"` lines
become `#include "build/X.h"`, and audiolib's `"../buildengine/platform.h"` becomes
`"../build/platform.h"`. This is a plain textual rewrite of `duke3d.h`, `astub.c`,
`dukemusc.c`, `fx_man.c`, `control.c` and `audiolib/multivoc.h`, with no shim headers.

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
| `USE_SDL=1` | all, via `duke3d_compile_options` | SDL code paths |
| `PLATFORM_UNIX=1` | Linux, macOS | `duke3d.h` / `platform.h` test it by value |
| `PLATFORM_MACOSX=1` | macOS (plus `PLATFORM_UNIX`) | `<machine/endian.h>`, no `<malloc.h>`, mmulti tweaks |
| `PLATFORM_WIN32=1` | Windows | `win32_compat.h`, `dukewin.h` |
| `UDP_NETWORKING=1` | `buildengine` (PRIVATE) | Selects mmulti's transport (otherwise `#error`) |
| `USE_EXECINFO=1` | Linux only | Backtraces, matching the old Makefile |
| `VOLUMEALL` / `VOLUMEONE` | `duke3d`, `buildeditor` | From `DUKE3D_SHAREWARE` |
| `CONTROLS_CONFIG_MENU=1` | `duke3d`, `buildeditor` | From `DUKE3D_CONTROLS_MENU` |
| `_CRT_SECURE_NO_WARNINGS`, `_CRT_NONSTDC_NO_DEPRECATE` | MSVC | Legacy CRT and POSIX names |

`USE_I386_ASM` is never defined. The engine uses the C implementations (`a.c`,
`pragmas.c`) on every platform, because MSVC x64 has no inline asm and the target CPUs
include arm64.

Flags:

- GCC and Clang: `-funsigned-char -fno-strict-aliasing -W -Wall -Wno-unused`, plus the
  supported subset of `-Wno-error=implicit-function-declaration`, `implicit-int`,
  `int-conversion`, `incompatible-pointer-types`, and `return-mismatch`. Add
  `-fno-omit-frame-pointer` on `buildengine`.
- MSVC: `/J` (unsigned char), `/W3`, `/wd4996`.
- `c_std_99` compile feature; extensions stay on (CMake default).
- Optimization comes from the build type.
- `animlib.c` compiles with `-O0` on macOS, matching the old Makefile, which did this to
  work around an optimizer bug in animation playback.
- Warnings are not errors.

Platform link libraries (on `buildengine`, PUBLIC):

- Windows: `ws2_32 winmm`
- Linux: `m`
- macOS: none beyond SDL2's own frameworks

## SDL2 port

All of it is in `source/build/sdl_driver.c`:

- **Video:** a `SDL_Window` + `SDL_Renderer` (vsync, with a non-vsync fallback) + an
  ARGB8888 streaming `SDL_Texture`.
  - The engine keeps drawing into an 8-bit `SDL_Surface`.
  - `present_surface()` blits it through its palette into a 32-bit staging surface,
    updates the texture, and presents.
  - `SDL_RenderSetLogicalSize` keeps the aspect ratio. Windows smaller than 640 wide
    open at an integer scale.
  - Fullscreen uses `SDL_WINDOW_FULLSCREEN_DESKTOP`, toggled with Alt+Enter through
    `SDL_SetWindowFullscreen`. The surface survives the toggle, so the old
    pointer-fixup code is gone.
  - Mode lists come from `SDL_GetNumDisplayModes`/`SDL_GetDisplayMode`; titles from
    `SDL_SetWindowTitle`; driver info from `SDL_GetCurrentVideoDriver`.
- **Palette:** `SDL_SetPaletteColors` on the surface's palette (this also fixes the
  upstream offset bug when `start != 0`), followed by an immediate present.
- **Keyboard:** an `SDL_Scancode` → DOS scancode table sized `SDL_NUM_SCANCODES`.
  Scancodes are positional, so the AZERTY/`SDLK_WORLD_*` special cases are dropped.
  Key repeat events are ignored, as SDL 1.2 never sent them. Cmd-Q uses `KMOD_GUI`.
- **Mouse:** grabbing uses `SDL_SetRelativeMouseMode` + `SDL_SetWindowGrab`. The wheel
  comes from `SDL_MOUSEWHEEL` (bits 3/4); buttons 4/5 (X1/X2) are ignored.
- **Events:** `SDL_WINDOWEVENT_EXPOSED` re-presents the current frame.
- **Joystick:** `SDL_JoystickNameForIndex`, which can return NULL and is guarded.
- **Timer:** the callback returns `interval` (100 Hz), not `1`.
- **Shutdown:** `shutdown_video()` frees the texture, surfaces, renderer and window.
  It's used by `setvmode(0x3)`, `_uninitengine` and `uninitvesa`.
- **Windows:** `S_IRUSR`-style fallbacks for `screencapture`; `alloca` is replaced by a
  fixed buffer.

Error handling keeps the existing pattern: on SDL failure, print `SDL_GetError()` and
`exit()` with the driver's existing codes.

## Smoke test

`tests/sdl_driver_smoke.c` links `buildengine` alone and stubs the three game callbacks
(`timerhandler`, `keyhandler`, `faketimerhandler`). It then checks:

1. `_platform_init` + `_setgamemode(1, 320, 200)` succeed and set `xdim`, `ydim`, and
   `frameplace`.
2. `VBE_setPalette` succeeds, and `VBE_getPalette` reads every channel back within ±1.
   The 0–63 ↔ 0–255 conversion truncates. A distinct pattern per channel catches a
   swapped or dropped channel.
3. Thirty frames are drawn and presented, then events are pumped.
4. The 100 Hz timer fires 25–100 times in 500 ms. The pre-port `return(1)` gives about
   400, which the test was confirmed to catch.

CTest runs it with `SDL_VIDEODRIVER=dummy` and `BUILD_WINDOWED=1` (timeout 30 s) on
every platform except Windows.

## Dependencies

`cmake/Dependencies.cmake`:

1. Unless `DUKE3D_FETCH_SDL=ON`, try `find_package(SDL2 CONFIG)` and
   `find_package(SDL2_mixer CONFIG)`.
2. If either is missing and `DUKE3D_FETCH_SDL` isn't `OFF`, use `FetchContent` with
   pinned release tarballs and `URL_HASH SHA256`:
   - SDL 2.32.10: `5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165`
   - SDL_mixer 2.8.1: `cb760211b056bfe44f4a1e180cc7cb201137e4d1572f2002cc1be728efd22660`
   - Build both statically.
   - SDL_mixer: `SDL2MIXER_VENDORED=OFF`, with WAV, MIDI (Timidity) and native MIDI
     enabled. FLAC, GME, MOD, MP3, Opus, Vorbis, WavPack, FluidSynth and CMD are
     disabled.
   - `DUKE3D_FETCH_SDL=OFF` with a missing library is a `FATAL_ERROR` that names the
     install commands.
3. Define `duke3d::SDL2`, `duke3d::SDL2main` (Windows only, empty elsewhere), and
   `duke3d::SDL2_mixer` INTERFACE aliases over whichever targets exist.
4. On Windows, when at least one library came from `find_package` (possibly shared), a
   post-build step copies `$<TARGET_RUNTIME_DLLS:...>` next to each executable. When
   both are fetched, the step is skipped, because the DLL list is empty and
   `copy_if_different` would fail.

Minimum CMake version: 3.21 (needed for `TARGET_RUNTIME_DLLS` and presets v3).

## CI

`.github/workflows/build.yml`, triggered on push and pull_request:

| Runner | SDL | Generator |
|---|---|---|
| `ubuntu-latest` | `apt-get install libsdl2-dev libsdl2-mixer-dev` | default (Unix Makefiles) |
| `macos-latest` (arm64) | `brew install sdl2 sdl2_mixer` | default (Unix Makefiles) |
| `windows-latest` | FetchContent fallback | default (Visual Studio 2022, x64) |

Each job runs `cmake --preset default`, then `cmake --build --preset default --parallel`.
`ctest --preset default` runs on all but Windows. Each job uploads `build/bin/` as an
artifact. Windows is the job that tests the fetch path.

## Vendoring

- Source: `https://github.com/icculus/buildengine`, commit
  `f3dce0a10bff90e1ed5cf8ec186b3731237a86da` (2021-07-12).
- Copy exactly these 22 files: `a.c a.h bstub.h build.c build.h cache1d.c cache1d.h
  display.h doscmpat.h engine.c engine.h engine_protos.h mmulti.c platform.h pragmas.c
  pragmas.h sdl_driver.c unix_compat.c unix_compat.h win32_compat.h BUILDLIC.TXT
  LICENSE.md`.
  - The Build engine is under Ken Silverman's license, not the GPL. The license file
    ships alongside the code.
- Add `source/build/VENDORED.md` with the upstream URL, the commit hash, the file list,
  and every local change.
- Commit order: (1) a pure, unmodified copy; (2) the include-path rewrite, CMake, and the
  Windows compat changes; (3) the SDL2 port and smoke test. This keeps diffs against
  upstream reviewable.

## Removed and kept files

Removed: `source/Makefile`, `source/Makefile.macshipped`, `source/audiolib/Makefile`,
`source/duke3d.sln`, `source/duke3d.vcproj`, `source/duke3d.dsp`, `source/duke3d.dsw`.

Kept unchanged: `source/makefile.dos`, `source/makefile.lnk` (historical Watcom build),
everything in `extras/` and `testdata/`.

`.gitignore`: add `/build/`, `/build-debug/` and `/build-*/`, anchored to the repo root
so `source/build/` isn't ignored.

## Definition of done

1. On local macOS arm64, `cmake --preset default && cmake --build build` builds
   `build/bin/duke3d`, `build/bin/build` and `build/bin/sdl_driver_smoke` with zero
   errors. Warnings are allowed.
2. `ctest --preset default` passes on macOS, both with installed SDL2 and with
   `-DDUKE3D_FETCH_SDL=ON`.
3. With no game data, `build/bin/duke3d` exits with "Unable to open duke3d.grp".
4. All three CI jobs configure and build all three executables with zero errors, and
   the Linux and macOS jobs pass `ctest`. CI runs when the user pushes; this work
   doesn't push.
5. The obsolete build files are removed; the Watcom files remain.
6. `README.md` has a "Building" section with per-OS prerequisites, the two build
   commands, the output paths, the options, and the known 64-bit issue.

## Risks

- **Windows-only compile errors** (MSVC with winsock/`windows.h` in `mmulti.c`,
  unguarded POSIX headers in game code): CI is the only detector. Fixes stay minimal,
  use `#if PLATFORM_WIN32`/`_MSC_VER` guards, and engine fixes are recorded in
  `VENDORED.md`.
- **Keyboard mapping:** the scancode table can't be exercised until the game boots
  (64-bit spec). The table mirrors the upstream keysym table entry for entry.
- **Vsync presents in `VBE_setPalette`** pace palette fades at the display refresh
  rate. That's intended (it replaces the old `limitrate()`), but it's unverified until
  the game boots.
- **SDL2_mixer MIDI** on systems without Timidity patches: music is silent, and the
  game doesn't fail (Duke's music path is stubbed upstream anyway).
