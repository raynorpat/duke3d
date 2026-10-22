# duke3d

A portable C port of 3D Realms' Duke Nukem 3D, built on Ken Silverman's Build engine. It
started as the icculus.org port (the first Linux/Mac/Windows one) and now builds
with CMake for 64-bit Windows, macOS and Linux. Video, input and audio go
through SDL2; music is played by an emulated OPL3, so no MIDI device or
SDL_mixer is needed. The same source builds the 1.5 (Atomic Edition) game, 1.4,
1.3D, and the 1.3D, 1.1 and 1.0 shareware releases.

No game data is included. You need your own copy of the game (see
[Game data](#game-data)).

The original 3D Realms README is below.

## Quick start

Requirements: a C99 compiler and CMake 3.24 or newer. SDL2 is used if CMake
finds it, and otherwise downloaded and built from source (needs network access).

| OS | Setup |
|---|---|
| macOS | `brew install cmake sdl2` |
| Linux (Debian/Ubuntu) | `sudo apt install cmake build-essential libsdl2-dev` |
| Windows | Visual Studio 2022 with "Desktop development with C++", and CMake. SDL2 is fetched automatically. |

From the repository root:

```
cmake --preset default
cmake --build --preset default
```

This builds `build/bin/duke3d` (the game) and `build/bin/build` (the map
editor). With Visual Studio they are in `build/bin/RelWithDebInfo/`. The
`debug` preset does the same in `build-debug/` with a Debug build.

To run, put the game data in a folder and start the game from it:

```
cd /path/to/your/duke3d-data
/path/to/repo/build/bin/duke3d
```

## Game data

The game reads its data from the working directory:

- `DUKE3D.GRP`
- `GAME.CON`, `DEFS.CON` and `USER.CON`
- optionally `DUKE.RTS` and demo files (`*.DMO`)

Lookups fall back to a case-insensitive match, so the filename case doesn't
matter. The data must match the version you built:

| Build | Data needed |
|---|---|
| default (1.5) | 1.5 / Atomic Edition |
| `DUKE3D_VERSION=1.4` | 1.4 |
| `DUKE3D_VERSION=1.3D` | 1.3D full release |
| `DUKE3D_VERSION=1.3D` with `DUKE3D_SHAREWARE=ON` | 1.3D shareware |
| `DUKE3D_VERSION=1.1` with `DUKE3D_SHAREWARE=ON` | 1.1 shareware |
| `DUKE3D_VERSION=1.0` with `DUKE3D_SHAREWARE=ON` | 1.0 shareware |

At startup the game checks `DUKE3D.GRP` against the release GRPs (1.3D, 1.4,
1.5 Atomic Edition and the shareware releases, by size and CRC-32) and prints a
warning if it's one of those but not the one the build expects. Other GRPs,
such as modified ones, aren't checked.

## Choosing a game version

`DUKE3D_VERSION` selects which release's game logic and file formats are built.
It affects saves, demos and multiplayer: a build only plays with peers of the
same version and edition, and refuses others. Details of the 1.3D and 1.4
variants are in [docs/v13d/reference.md](docs/v13d/reference.md), and of 1.0 and
1.1 in [docs/v10-v11/reference.md](docs/v10-v11/reference.md).

Use a separate build directory for each variant:

```
cmake -S . -B build-v13d -DDUKE3D_VERSION=1.3D
cmake --build build-v13d

cmake -S . -B build-v13dsw -DDUKE3D_VERSION=1.3D -DDUKE3D_SHAREWARE=ON
cmake --build build-v13dsw

cmake -S . -B build-v11sw -DDUKE3D_VERSION=1.1 -DDUKE3D_SHAREWARE=ON
cmake --build build-v11sw

cmake -S . -B build-v10sw -DDUKE3D_VERSION=1.0 -DDUKE3D_SHAREWARE=ON
cmake --build build-v10sw
```

`DUKE3D_SHAREWARE=ON` with 1.4 or 1.5 is rejected at configure time: only 1.3D,
1.1 and 1.0 had a shareware release. The reverse holds for 1.1 and 1.0: they
exist here only as shareware builds.

### Playing 1.3D

A 1.3D build needs the files from the original 1.3D release in its working
directory: the full release for a full build, the shareware release for a
shareware build. None of them ship with this repository. Besides `DUKE3D.GRP` and
the three CON files, copy `DUKE.RTS` and the demos: 1.3D keeps `DEMO1.DMO` and
the other demos next to the GRP rather than inside it, and the title screen
only plays the ones it finds.

A 1.3D build plays the original 1.3D demos, compiles 1.3D CON scripts, and
loads saves made by the DOS 1.3D executable of the same
edition (copy them into the save folder as `game0.sav` to `game9.sav`). Its
own saves use the same DOS 1.3D layout.

### Playing 1.1 and 1.0

Both were only released as shareware. A 1.1 or 1.0 build needs `DUKE3D.GRP`, the three
CON files, `DUKE.RTS` and the demos from the matching shareware release in its working
directory. The release comes as a zip holding `DUKE3DS._1` to `DUKE3DS._4` and an
`INSTALL.EXE`, which won't run here. Instead, join the four parts and extract them
(7-Zip's `7zz` reads the result):

```
cat DUKE3DS._1 DUKE3DS._2 DUKE3DS._3 DUKE3DS._4 > all.bin
7zz x -o<dest> all.bin
```

Details are in [docs/v10-v11/reference.md](docs/v10-v11/reference.md).

Each build plays its own release's CON scripts and its three demos, and loads saves made
by the DOS executable of the same release (copy them into the save folder as `game0.sav`
to `game9.sav`). 1.0 saves carry no version field, so the 1.0 build recognises them by
their layout. Its own saves use the DOS layout of its release. Multiplayer works between
our builds of the same release only.

## Build options

Pass these with `-D` when configuring.

| Option | Default | Effect |
|---|---|---|
| `DUKE3D_VERSION` | `1.5` | Game version: `1.0`, `1.1`, `1.3D`, `1.4` or `1.5` |
| `DUKE3D_SHAREWARE` | `OFF` | Build a shareware version (`VOLUMEONE`); needs `DUKE3D_VERSION=1.3D`, `1.1` or `1.0` (1.1 and 1.0 are always shareware) |
| `DUKE3D_POLYMOST` | `ON` | Build the Polymost renderers (see [Renderers](#renderers)); needs OpenGL. `OFF` builds the classic renderer only |
| `DUKE3D_CONTROLS_MENU` | `ON` | In-game controls configuration menu |
| `DUKE3D_FETCH_SDL` | `AUTO` | `AUTO` uses installed SDL2 and fetches it if missing; `ON` always builds SDL2 2.32.10 from source; `OFF` fails if SDL2 isn't installed |

The default build type is `RelWithDebInfo`. The Watcom/DOS makefiles
(`source/makefile.dos`, `source/makefile.lnk`) are kept for reference only.

## Renderers

The game has four renderers, from Ken Silverman's Polymost as ported in
JFBuild:

| Number | Renderer |
|---|---|
| 0 | Classic Build software renderer (default) |
| 1 | Polymost solid colour (Ken's debug view) |
| 2 | Polymost software (polygons into the 8-bit framebuffer) |
| 3 | Polymost OpenGL |

Pick the renderer on the VIDEO page (OPTIONS, then VIDEO...) at any time,
in a game or the attract demo, or set `Renderer` under `[Screen Setup]` in
`duke3d.cfg`, or use `-renderer N` for one run (the config keeps its own
value; see [The VIDEO page](#the-video-page)). A number outside 0-3 is taken
as the nearest one. Renderer 3 draws at the window's real pixel size
(unless `GLInternalWidth`/`GLInternalHeight` is set on the 2.1 path) and
uses a legacy (non-core) OpenGL context. Windowed, it opens at the size
asked for, even over the 1600x1200 the 8-bit modes stop at. Once renderer 3
has been used, its window and context stay until the program exits, and
renderers 0-2 and the editor's 2D mode are shown through them, looking as
before (switching renderer doesn't make a new window). A run that started in
renderer 0-2 has no OpenGL context; the first switch to OpenGL builds the
window once with one. If OpenGL isn't available, the game prints a message
and uses renderer 0 (at startup) or keeps the renderer it was in (from the
VIDEO page). The map editor has
its own settings (see [Map editor](#map-editor)).

A `DUKE3D_POLYMOST=OFF` build has renderer 0 only: 1 and 2 draw with it
without a message, and 3 prints `OpenGL renderer unavailable (this build has
no OpenGL); using the classic renderer.`

Other settings, all under `[Screen Setup]`:

- Widescreen and tallscreen: modes wider than 4:3 get a wider view. The
  status bar's artwork is carried out to both screen edges, while its
  numbers and icons and the rest of the HUD stay in the 4:3 area. This
  applies to every renderer. 320x200, 640x400 and other 4:3 modes draw as
  before. The small HUD (the smallest screen size, with no status bar) is
  anchored to the screen edges in widescreen: health at the left edge, ammo
  at the right, and the inventory box just left of the ammo.
- `GLTextureMipLevel` (default 0, at most 15): in renderer 3, drop this many
  mipmap levels from every texture. Over 0 looks blocky.
- `ScreenshotFormat`: `png` (default), `jpg` or `bmp`. Screenshots are named
  `duke0000.png` and so on, and go to a `Screenshots` folder inside the one
  under [Config and saves](#config-and-saves). They show the screen as it
  is, fades and flashes included, in every renderer.

#### Renderer 3's draw paths

Renderer 3 draws with one of three paths, each needing more of the driver:

| Path | `GLDrawPath` | Needs | What it does |
|---|---|---|---|
| 1.1 | `immediate` | any OpenGL | Client vertex arrays, with fog and a per-texture approximation of classic's shade ramps |
| 1.5 | `vbo` | OpenGL 1.5 vertex buffers | The same drawing, with the vertices in vertex buffer objects |
| 2.1 | `shader` | OpenGL 2.1 with GLSL and framebuffer objects | Draws the whole frame as palette indices, then looks them up through the game's own shade tables and palette, so shading, palette swaps and fades follow classic. Translucent sprites, walls and tiles blend through the translucency table, as classic does |

- `GLDrawPath` under `[Screen Setup]` in `duke3d.cfg` is `auto` (the default),
  `immediate`, `vbo` or `shader`, in any case, written in quotes, such as
  `GLDrawPath = "shader"` (unquoted, it reads as `auto`). `auto` takes the
  best path the driver has. A named path the driver can't do falls back to the best it can,
  with a message such as `OpenGL path shader unavailable; using vbo.`, and
  every start prints the path in use (`OpenGL path: shader (2.1)`). A path
  better than the one named is never used: `vbo` on a driver that has the
  shader path uses `vbo`.
- The 1.1 and 1.5 paths are closer to classic's shading than before, but not
  exact. Only the 2.1 path matches classic's shading, palette changes and
  translucency.
- `GLInternalWidth` and `GLInternalHeight` (default 0, at most 7680 and 4320):
  draw the 2.1 path at this size and scale the result up to the window,
  keeping its aspect ratio, with black bars where it doesn't fill. 320 by 200
  gives classic's own pixels. Both must be set, and a size under 320x200 is
  raised to it. Otherwise, and on the 1.1 and 1.5 paths, the window's size is
  used, with a message. `build.cfg` has `glinternalwidth` and
  `glinternalheight` for the editor.
- `GLContrast` (0.5 to 1.5) and `GLSaturation` (0 to 2), default 1.0: adjust
  the 2.1 path's final image, and do nothing on the other paths. In
  `duke3d.cfg` write them as quoted decimals, such as `GLContrast = "1.25"`;
  an unquoted decimal is read as a whole number. `build.cfg` has `glcontrast`
  and `glsaturation`; the editor has no control for them. The brightness
  setting (`britable`) is separate and works as before; it is now a row on
  the VIDEO page, and F11 still steps it.

#### Display mode and vsync

- `DisplayMode` under `[Screen Setup]` is `windowed`, `borderless` (the
  default) or `exclusive`, written in quotes, such as `DisplayMode =
  "windowed"` (unquoted, it reads as borderless). Borderless is a desktop-sized window without a mode
  change. Exclusive is true fullscreen: it switches the display to the
  smallest mode at least as big as the game's size (so 640x400 may pick
  1920x1200 and scale up). If the display has no such mode, it uses borderless
  and prints a message. Unknown values read as borderless.
- `VSync` is 1 (the default) or 0. With it off, renderer 3 runs as fast as it
  can. The SDL renderer behind renderers 0-2 may still be held to the
  display's rate by the system.
- Alt+Enter toggles between a window and the chosen fullscreen style
  (borderless if you chose windowed). That, and the exclusive-to-borderless
  fallback, last for the run only and aren't saved. Setting the environment
  variable `BUILD_WINDOWED` forces a window for that run, in the game and the
  editor.
- All of these can be changed live on the VIDEO page, described next.

#### The VIDEO page

OPTIONS, then VIDEO..., has nine rows. Each change applies at once, in a game,
in the attract demo or in a network game, without a restart, and it doesn't
touch the game state or demo and network sync. Choice rows (RENDERER, DISPLAY,
VSYNC, SUPERSAMPLE, FILTERING) step forward with Enter, a click or Right and
back with Left, skipping choices that aren't available. A row with nothing
else to offer is greyed, and a note under the rows says why for the row the
cursor is on.

| Row | Choices | Rules |
|---|---|---|
| RENDERER | CLASSIC, POLY SOLID, POLY SOFT, OPENGL (0-3) | The first switch to OPENGL in a run that started in renderer 0-2 builds the window once with OpenGL. If that fails, the previous renderer is kept with a message, OPENGL is skipped for the rest of the run and the page says `OPENGL COULD NOT START`. A `DUKE3D_POLYMOST=OFF` build offers CLASSIC only |
| DISPLAY | WINDOWED, BORDERLESS, FULLSCREEN | FULLSCREEN is `exclusive` in the config. Going to FULLSCREEN keeps the size if the main display has it as a mode, else takes the desktop's size, else the largest mode. Going to WINDOWED keeps the size if it fits the desktop, else takes the largest that does |
| RESOLUTION | a list of sizes | Left and Right pick a size and Enter applies it (moving off the row drops the pick). Windowed: sizes up to the desktop's from the display modes, plus the current size. Fullscreen: the modes the main display reports, at their highest refresh rate. Borderless: shows DESKTOP and is greyed, as the window fills the screen |
| VSYNC | ON, OFF | As `VSync` above |
| BRIGHTNESS | slider, 8 levels | Moved here from OPTIONS; F11 still steps it. It is the same setting as before |
| CONTRAST, SATURATION | sliders | The 2.1 path only; greyed otherwise |
| SUPERSAMPLE | OFF, 2K, 4K, 8K | Renderer 3 on the 2.1 path only. Draws at 1440, 2160 or 4320 pixels tall (2560x1440, 3840x2160 and 7680x4320 at 16:9) with the window's aspect, and scales down to the window, so the picture is smoothed. A level not taller than the window is greyed, so it only ever scales down, and a size over what the GPU can do is cut down with a message. It is greyed with the 1.1 and 1.5 paths and renderers 0-2, and while `GLInternalWidth`/`GLInternalHeight` is set (the internal size wins). 8K uses a lot of video memory |
| FILTERING | NEAREST, BILINEAR, TRILINEAR | Renderer 3 only; greyed in renderers 0-2. On the 1.1 and 1.5 paths it sets the texture filter (trilinear uses mipmaps). On the 2.1 path, NEAREST keeps the exact palette-index drawing. BILINEAR and TRILINEAR switch it to colour drawing, see below |

Resolution in the 8-bit renderers (0-2): the size chosen is the size the game
draws its 8-bit picture at, up to the engine's 1600x1200 cap (the aspect is
kept, and the width is rounded down to a multiple of 4). A bigger choice draws
at the largest size that fits and is scaled up to fill the window. The list
also has the classic sizes such as 320x200, 640x400 and 640x480. In borderless
they draw at the desktop's size, also up to the cap. Renderer 3 draws at the
window's real size, or its internal size, or the supersampled size.

Filtering and the 2.1 path: BILINEAR and TRILINEAR draw colours rather than
palette indices. Distance shading still goes through the game's shade tables
and stays exact, with trilinear shading each of the texels it blends. Seeing
through things (translucent sprites, walls and tiles) blends with fixed
amounts, and fades and flashes are worked out by the present step: both are
approximations while filtering is on, close to but not the same as classic's.
Supersampling and the contrast and saturation sliders work with filtering.
Switching to NEAREST and OFF gives the exact 2.1 drawing again. If the colour
programs don't link, a message is printed and the path keeps drawing nearest,
with the filtering choices not offered.

Fades: a change of renderer, resolution or display mode fades the screen to
black (about a quarter of a second), makes the change while it is black, and
fades back in. The same goes for supersampling and filtering. Play and the
menu keep running underneath. While a change is pending the rows that would
ask for another ignore input, so a double press is one change. A failed
change fades back in to what was kept.

What is saved: a row that you change on the page is saved to `duke3d.cfg`
right away (and at exit), and rows you didn't change keep the value already in
the file. So a one-run override is not written: `-renderer N`, Alt+Enter, the
`BUILD_WINDOWED` variable or a fullscreen-to-borderless fallback show on the
page as what is in effect, but leave the file's value until you change that
row. A row you change saves the value you chose even if
the driver can't honour it fully (a supersampling level cut down to the
largest frame, or drawn at the window's size), except that OPENGL, when OpenGL
couldn't start, is saved as the renderer that was kept. The keys are `Renderer`, `ScreenWidth`, `ScreenHeight`,
`DisplayMode`, `VSync`, `GLContrast`, `GLSaturation`, `Supersampling` and
`GLTextureFilter`, all under `[Screen Setup]`. `ScreenWidth` and
`ScreenHeight` are now written to the file as well as read.

- `Supersampling` is `off` (the default), `2k`, `4k` or `8k`, and
  `GLTextureFilter` is `nearest` (the default), `bilinear` or `trilinear`,
  written in quotes like `DisplayMode`. Anything else reads as the default.
  `GLDrawPath` and the internal size stay config-only.

Known gaps:

- The 1.1 and 1.5 paths don't match classic's shading exactly.
- On the 1.1 and 1.5 paths, palette switches (underwater, slime, night
  vision, the title and ending screens, cutscenes) and brightness changes
  rebuild the textures, so they can stutter. The 2.1 path doesn't. On the 1.1
  and 1.5 paths fades and flashes are drawn over the frame; the 2.1 path
  fades the palette.
- No `.md2` models or hi-res texture replacement.
- Polymost's own todo list applies (sprite sorting, the hall of mirrors near
  the zenith, ...).

### Map editor

The editor (`build`) draws its 3D mode in the same four renderers; the 2D mode
is unchanged. A `DUKE3D_POLYMOST=OFF` build's editor is renderer 0 only and
Alt+K stays on it. Its settings are in `build.cfg`, which replaces
`setup.dat`. It is in the config folder under [Config and saves](#config-and-saves) (macOS and
Linux) or the working directory (Windows), is written when the editor exits,
and is read with each value clamped. The editor reads `duke3d.grp` from the
working directory.

| Key | Default | Meaning |
|---|---|---|
| `renderer` | 0 | 3D renderer, 0-3 as in the table above |
| `xdim3d`, `ydim3d` | 640, 480 | 3D mode size; 320-7680 by 200-4320. Renderers 0-2 stop at 1600x1200, so a larger size only applies in renderer 3 |
| `displaymode` | borderless | `windowed`, `borderless` or `exclusive`, as `DisplayMode` above |
| `vsync` | 1 | Vsync on or off |
| `gldrawpath` | auto | Renderer 3's draw path: `auto`, `immediate`, `vbo` or `shader` |
| `glinternalwidth`, `glinternalheight` | 0, 0 | The 2.1 path's internal size, up to 7680 by 4320; 0 is the window's |
| `glcontrast`, `glsaturation` | 1, 1 | The 2.1 path's contrast (0.5-1.5) and saturation (0-2) |
| `gltexmiplevel` | 0 | Mipmap levels dropped in renderer 3, 0-15 |
| `supersampling` | off | Renderer 3 on the 2.1 path draws at 2k, 4k or 8k (`off`, `2k`, `4k`, `8k`) and scales down, as on the VIDEO page |
| `gltexturefilter` | nearest | Renderer 3's texture filter: `nearest`, `bilinear` or `trilinear`, as on the VIDEO page |
| `mousesensitivity` | 1 | Mouse speed, 0.1-10 |
| `keyforward` ... `keychat` | see below | Keys, as two-digit hex scancodes |

The 19 keys, in order, with their defaults: `keyforward` c8 (Up),
`keybackward` d0 (Down), `keyturnleft` cb, `keyturnright` cd, `keyrun` 2a (L.Shift),
`keystrafe` 9d (R.Ctrl), `keyfire` 1d (L.Ctrl), `keyuse` 39 (Space),
`keystandhigh` 1e (A), `keystandlow` 2c (Z), `keylookup` d1 (PgDn),
`keylookdown` c9 (PgUp), `keystrafeleft` 33 (,), `keystraferight` 34 (.),
`key2dmode` 9c (keypad Enter), `keyviewcycle` 1c (Enter), `key2dzoomin` 0d (=),
`key2dzoomout` 0c (-), `keychat` 0f (Tab). A value outside 01-ff gets
its default.

- Without a `build.cfg`, the keys of a `setup.dat` in the working directory are
  taken once; the other settings keep their defaults.
- `-renderer N` (or `/renderer N`) picks the renderer for one run. It isn't
  saved.
- Alt+K cycles to the next renderer in the 3D view, skipping OpenGL when it
  isn't available. The choice is saved on exit.
- Without a `displaymode` line, an old `fullscreen` key is read (1 is
  borderless, 0 windowed) and is not written back.
- Setting the environment variable `BUILD_WINDOWED` forces a window for that
  run. Neither it, Alt+Enter nor the exclusive-to-borderless fallback changes
  `displaymode` in `build.cfg`.

## Running

### Config and saves

Without a `duke3d.cfg` the game starts with default settings and writes one.
Config, saves and screenshots go to:

| OS | Folder |
|---|---|
| macOS | `~/Library/Application Support/Duke Nukem 3D/` |
| Linux | `~/.duke3d/` |
| Windows | `Saved Games\Duke Nukem 3D\` |

On macOS and Linux, a `duke3d.cfg` in the working directory is copied there if
the folder has none. On Windows, an existing `duke3d.cfg` and `game0.sav` to
`game9.sav` in the game folder are copied there on first run. The Windows
folder can be moved with the `DUKE3D_SAVE_DIR` environment variable (a
`Duke Nukem 3D` folder is created inside it).

Original DOS 1.5 (Atomic Edition) saves load on any OS and CPU, and this build
writes saves in the same layout. The original DOS game can't load them back,
though. A 1.3D build loads DOS 1.3D saves instead (see
[Playing 1.3D](#playing-13d)). A 1.4 or 1.5 build reading 1.3D CON
files needs `UseOldVersion = 1` under `[Misc]` in `duke3d.cfg`; a 1.3D build
doesn't.

64-bit status: the game runs as a native 64-bit binary. That's verified on
macOS arm64; Linux and Windows are built and tested in CI. Demo recording and
playback and the editor's map saving are verified on 64-bit. Network play
builds but hasn't been verified on 64-bit.

### Command-line switches

Switches start with `/`, not `-`. Arguments starting with `-` are skipped,
except `-net <file>` (below). Unknown `/` switches print the help
text. Run with `/?` for the full list.

| Switch | Effect |
|---|---|
| `/v#` `/l##` `/s#` | Warp to volume, level and skill |
| `/#` | Load a saved game from slot 0-9 |
| `/m` | No monsters |
| `/ns` `/nm` | No sound, no music |
| `/r` | Record a demo |
| `/dFILE` | Play demo FILE (`.dmo` is added if there's no extension) |
| `/t#` | Respawn: 1 monsters, 2 items, 3 inventory, other value for all |
| `/c#` | Multiplayer mode: 1 DukeMatch (spawn), 2 co-op, 3 DukeMatch (no spawn); in 1.3D builds 0, 1 and 2 |
| `/q#` | Fake multiplayer with 2-8 players; `/a` adds player AI |
| `/xFILE` | Compile FILE instead of `GAME.CON` |
| `/gFILE` | Add a group file; must be last on the command line |
| `/u#########` | Favorite weapon order |
| `/z` | Skip memory check |

For example, to start Episode 1, Level 1 on skill 2:

```
./build/bin/duke3d /v1 /l1 /s2
```

### Multiplayer

Network play is peer to peer over UDP. Every player starts the game with a
config file, using the same game version and data:

```
duke3d -net net.cfg
```

`net.cfg` is a list of keywords:

| Keyword | Meaning |
|---|---|
| `interface <ip>[:<port>]` | Local address and UDP port to bind. The default port is 1635; `:any` picks one |
| `mode peer` | Peer to peer, the default. `server` and `client` are accepted but not implemented |
| `allow <ip>[:<port>]` | Another player's address. Repeat for each other player |
| `broadcast <n>` | Find `n` other players by LAN broadcast instead of `allow`; can't be combined with `allow` |

Two players on one machine:

```
# A: net.cfg
interface 127.0.0.1:23600
mode peer
allow 127.0.0.1:23601

# B: net.cfg
interface 127.0.0.1:23601
mode peer
allow 127.0.0.1:23600
```

## Tests

```
ctest --preset default
```

Most tests need no game data: the SDL driver smoke test, version macros,
fixed-point and drawing helpers, LZW, the OPL3 music path, audio shutdown,
the CON compiler, game helpers, the save codec, and version-option rejection.
They run headless with SDL's dummy video and audio drivers. On Windows a
`save_dir` test checks the Saved Games folder logic.

Tests that need game data are only registered when you point CMake at it:

| Cache variable | Enables |
|---|---|
| `DUKE3D_GRP` | `render_e1l1` (frame checksum, compared with `DUKE3D_E1L1_CHECKSUM`) and `game_boot` (not in 1.3D builds) |
| `DUKE3D_ATOMIC_GRP` and `DUKE3D_DOS_SAVES_DIR` | `dos_save_*`: loads original DOS 1.5 saves and compares with an independent decoder (`tests/dos_save_decode.py`). 1.5 builds only; needs Python 3 |
| `DUKE3D_V13D_DATA_DIR` | In 1.3D builds: CON compile, game boot, demo playback, multiplayer and edition checks against the 1.3D full release. With DOS reference saves in its `refsaves/` folder (listed in `tests/data/v13d_save_refs.cmake`), also `dos_save_v13d_*` and `save_refuse_v13d_*`; needs Python 3 |
| `DUKE3D_V13D_SW_DATA_DIR` | The same for a 1.3D shareware build |
| `DUKE3D_V11_SW_DATA_DIR`, `DUKE3D_V10_SW_DATA_DIR` | The same for a 1.1 or 1.0 shareware build, using that release's folder. DOS reference saves go in its `refsaves/` folder |
| `DUKE3D_NET_PEERS` | 1.3D, 1.1 and 1.0 builds: other builds, as `<exe>=<data folder>` entries, to check that multiplayer refuses mismatched versions |

A 1.5 refusal peer (`net_refuse_v10_sw_3`) can't run on the shareware data, so it needs a
data folder of its own. Make one in a git-ignored build directory, for example
`build-v15/netpeer`: a symlink to a 1.5 `DUKE3D.GRP`, plus copies of `testdata/*.con`.
Then list the 1.5 executable with it in `DUKE3D_NET_PEERS`, for example
`-DDUKE3D_NET_PEERS="build-v15/bin/duke3d=build-v15/netpeer"` (use absolute paths).

For example:

```
cmake --preset default -DDUKE3D_GRP=/path/to/DUKE3D.GRP
cmake --build --preset default
ctest --preset default
```

The DOS save tests need the Atomic Edition files. `tools/extract_iso_files.py`
copies them off an Atomic CD image (the image is only read):

```
python3 tools/extract_iso_files.py "<Track 1>.bin" ~/duke3d-data/atomic15 \
    ATOMINST/DUKE3D.GRP ATOMINST/GAME.CON ATOMINST/DEFS.CON ATOMINST/USER.CON
cmake --preset default \
    -DDUKE3D_ATOMIC_GRP=$HOME/duke3d-data/atomic15/DUKE3D.GRP \
    -DDUKE3D_DOS_SAVES_DIR=<folder of .sav files>
```

CI (`.github/workflows/build.yml`) builds and tests 1.5 on Ubuntu, macOS and
Windows, and 1.4, 1.3D and 1.3D shareware on Ubuntu. It has no game data, so
only the data-free tests run there.

## Project layout

| Path | Contents |
|---|---|
| `source/` | Game code, the map editor (`astub.c`) and the CMake target for both |
| `source/build/` | The Build engine (icculus port, SDL2 video), JFBuild's Polymost and stb_image_write (public domain or MIT); see `VENDORED.md` there |
| `source/audiolib/` | Jim Dose's audio library over SDL audio, with a DOS-style MIDI driver and emulated OPL3 (DBOPL) |
| `cmake/` | SDL2 lookup and fetch, compiler flags and version definitions |
| `tests/` | CTest tests and their helpers |
| `testdata/` | The 1.5 CON scripts used by tests |
| `tools/` | Save extraction and 1.3D reference scripts |
| `docs/v13d/` | Notes on the 1.3D and 1.4 variants |
| `docs/v10-v11/` | Reference, rulings and notes for the 1.0 and 1.1 shareware variants |

# Duke Nukem 3D (v1.5 CD Version) Source Code Release - April 1, 2003

## LEGAL STUFF

"Duke Nukem" is a registered trademark of Apogee Software, Ltd. (a.k.a. 3D Realms).

"Duke Nukem 3D" copyright 1996 - 2003 3D Realms.  All trademarks and copyrights reserved.

This is the complete source code for Duke Nukem 3D version 1.5, buildable as detailed in the next section.

The code is licensed under the terms of the GPL (gnu public license).  You should read the entire license (filename "GNU.TXT" in this archive), so you understand your rights and what you can and cannot do with the source code from this release.

All of the Duke Nukem 3D data files remain copyrighted and licensed by 3D Realms under the original terms.  You cannot redistribute our data or data files from the original game.  You can use our data for personal entertainment or educational purposes.  If you need the data, you can order a Duke Nukem 3D CD from our store on our website.

Please do not contact us for possible commercial exploitation of Duke Nukem 3D -- we will not be interested.

Please note that this is being released without any kind of support from Apogee Software, Ltd / 3D Realms Entertainment.  We cannot help in getting this running and we do not guarantee that you will be able to get it to work, nor do we guarantee that it won't blow up your computer if you do try and use it.  Use at your own risk.

## SPECIAL THANKS

First, we'd like to thank all the fans of 3D Realms and Duke Nukem.  We wouldn't be where we are without you and without your support.

Second, we'd like to thank Charlie Wiederhold for putting this release together.  We had decided some time ago to release the source code, but people are busy and it always seemed to be low priority.  A couple of weeks ago, Charlie got a source archive from Scott, and came back a few days later with it all compiling.

Thanks also to Ken Silverman and Jim Dose for allowing us to include some of their source in this build, so we have it all in one archive.

Finally, we'd like to thank the whole Duke Nukem Forever team.  These guys are all working incredibly hard on the next installment of Duke Nukem.

Enjoy the source code.  We can't wait to see what people do with it.  And we really look forward to being able to play the game under XP, with sound, internet play, hardware acceleration and whatever else you're clever enough to put in :)

George Broussard & Scott Miller
3D Realms

# HOW TO COMPILE THE SOURCE CODE (by Charlie Wiederhold)

This source code release was compiled on March 20th, 2003 using the materials in this archive.

Duke was originally written with the Watcom 10.0 C/C++ compiler. This code has been updated to allow it to compile under the free version of the Watcom compiler available from their webpage (http://www.openwatcom.org).

It was compiled under both Open Watcom 1.0 and Watcom 11.0c for Windows.  Both are free to download, and I didn't notice much of a difference between the two when using them. This means, thankfully, that anybody can work with this code right away without trying to find an out of production compiler or wait for someone to port it to other modern compilers.

Step 1) Install Watcom C/C++ onto your system.

Step 2) When you install, make sure that you select DOS 32 Bit as one of your target environments to build for.

Step 3) Choose a place you want to work from and unzip the contents of this .ZIP file into that directory.

From here you have two choices on how to work. You can use the command line compiler that comes with Watcom, or you can use the IDE (basically the development studio that manages your files, options for compiling, editing files, debugger, etc). Whichever one you choose depends on what you are comfortable with.

## IF YOU USE THE COMMAND LINE COMPILER

- In a Command Prompt, go into the Source directory where you should find a MAKEFILE. and a MAKEFILE.LNK file.
- At the Command Prompt type "wmake" or "wmake makefile" without the quotes.
- This should compile and create a DUKE3D.EXE file which you can then copy into the directory with your Duke 3D data and run.

## IF YOU USE THE IDE

- Start up the Watcom IDE and go to File -> Open Project.
- Find the directory where you've unzipped the Source files into, and you should see a DUKE3D.WPJ, select this and hit "OK".
- Click the "Make Current Target" button, or press "F4" or go to the menu Targets -> Make. You'll see lots of Warnings as it compiles, that's normal.
- This will create a DUKE3D.EXE file in the same directory where the DUKE3D.WPJ was located, which can then be copied in the directory with your Duke 3D data and run.

## ONCE YOU HAVE DUKE3D.EXE COMPILED

- If you own Duke 3D version 1.5, you are set... simply copy your new .EXE into the directory and run it.
- If you own Duke 3D version 1.4 or 1.3d, follow the same steps below except don't download the shareware data.
- If you own neither versions: Download the shareware version of Duke 3D from http://www.3drealms.com (go to Downloads).
- In the directory you have Duke 3D installed now, copy the four files in the TESTDATA directory into your Duke 3D directory. Now you should be able to play the game well enough to test, though unfortunately there will still be some minor issues. Your best results will come from owning a copy of Duke 3D version 1.5, which can still be purchased from the 3D Realms website.

This should be enough to get you started. Unfortunately nobody at 3D Realms will be able to devote the time to answer any support questions if you do encounter problems. However, go to http://www.3drealms.com/forums.html and you will find people discussing the source code in the Duke 3D Source category and able to answer questions. I will try to answer extremely common questions there shortly after the release, but I promise, within a very short time the community will outgrow my knowledge/understanding of this source and be better suited to answer any questions itself.

## MISC NOTES ABOUT THIS RELEASE

- All changes I made from the original are indicated by a "// CTW" style comment. In all but one case I commented out the original code so that it would still be there. I made as few changes as possible since the fun for people is working on the Duke 3D original code, not my personal rewrite/interpretation.

- Unfortunately the source to the SETUP.EXE and SETMAIN.EXE programs used for creating setup configuration files for Duke 3D is nowhere to be found, so if you need a setup utility to create custom .CFG files you'll need to write your own. If we find the source code we'll update this release to include that as well. The code to read .CFG files is within the Duke 3D CONFIG.C source itself, so it gives you something to start with.

- This source includes the Build Engine data (.OBJ files) needed for compiling. The Build Engine is a seperate entity from Duke 3D, though Duke 3D is built upon the Build Engine.

- The BUILDENGINE.ZIP file contains all of the data you need from Ken Silverman's released version to compile the Build Engine itself. Instructions for doing this are contained within that file. I have tested this with the free versions of Watcom and it works. More information about this code at Ken's webpage here: http://www.advsys.net/ken/buildsrc/default.htm

- The AUDIOLIB.ZIP file contains all of the data you need from Jim Dose's sound engine that was used in both Rise of the Triad and Duke 3D to compile the actual Audio Library itself. Instructions for doing this are contained in that file. I have not tested this personally to see if it compiles under the free version of Watcom.

- Sound will be sketchy if you are on any modern system. It does work, but only as well as the original Duke 3D's does.

- Networking works. Scott Miller and I fired up a quick game and it worked fine. The only catch is you need to disable sound for this to work in modern systems. No testing over a modem was done.

- The original .DMO demo files do not work. However recording new ones and playing those back does work, sometimes. I've disabled playback of demos in the source for stability when people first try to run their compiled version. It shouldn't be too hard to find how to re-enable it for those who want to go in and fix it.

- If you would like to play Duke 3D in high res Vesa modes instead of 320x200, download Ken's utility here: http://www.advsys.net/ken/build.htm

- The files in the EXTRAS folder are there purely for curiosity and educational purposes. Things like Allen Blum's experiments with room over room can be found in there, as well as older versions of the source files that ultimately made it into the game. None of these are necessary and are purely, well, extra.

- Duke 3D used DOS/4GW for it's DOS Extender. Watcom 1.0/11.0c comes with a couple free DOS Extenders, however you will need to bind it to the extender in order to distribute the EXE you create to other computers where it wasn't compiled. How you do this depends on which extender you choose to use. I trust that once you get to the point of distributing an EXE you can figure out how to bind it to the extender. Since DOS/4GW was a commercial licensed product, we can't distribute the resources we used to do this.

- All references to TEN (Total Entertainment Network) have been commented out of the version that will be compiled, but left in for the curious to look at.

- This page was an invaluable learning tool for me when working on preparing the source code for release: http://www.clipx.net/ ... The actual information I used can be found here: http://www.clipx.net/ng/zcpp/index.php

- I started working on this having no knowledge of Watcom, DOS based C programming, etc. and got it up and going within a night (and a half...ish) after work hours. So if you have a basic understanding of programming, you shouldn't have much trouble getting this up and running.

- The first things I think everyone would like to see done is an update to the sound engine and an update to the networking to allow play over the internet. But there is no end to the changes people can make, so everyone at 3D Realms looks forward to what people will do now that the source code is finally out there. It's been a long time coming (too long probably), but here you go, have fun!

Charlie Wiederhold

3D Realms
