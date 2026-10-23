# duke3d

A portable C port of 3D Realms' Duke Nukem 3D, built on Ken Silverman's Build engine. It
started as the icculus.org port (the first Linux/Mac/Windows one) and now builds
with CMake for 64-bit Windows, macOS and Linux. Video, input and audio go
through SDL2; music is played by an emulated OPL3, so no MIDI device or
SDL_mixer is needed. The same source builds the 1.5 (Atomic Edition) game, 1.4,
1.3D, and the 1.3D, 1.1, 1.0 and 0.99 pre-release beta shareware variants.

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
| `DUKE3D_VERSION=0.99` with `DUKE3D_SHAREWARE=ON` | 0.99 pre-release beta |

At startup the game checks `DUKE3D.GRP` against the release GRPs (0.99 through
1.5, including shareware, by size and CRC-32) and prints a
warning if it's one of those but not the one the build expects. Other GRPs,
such as modified ones, aren't checked.

## Choosing a game version

`DUKE3D_VERSION` selects which release's game logic and file formats are built.
It affects saves, demos and multiplayer. The multiplayer variants require peers of
the same version and edition; the 0.99 work targets single player. Details of 1.3D
and 1.4 are in [docs/v13d/reference.md](docs/v13d/reference.md), of 1.0 and 1.1 in
[docs/v10-v11/reference.md](docs/v10-v11/reference.md), and of the beta in
[docs/v099/reference.md](docs/v099/reference.md).

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

cmake -S . -B build-v099sw -DDUKE3D_VERSION=0.99 -DDUKE3D_SHAREWARE=ON
cmake --build build-v099sw
```

`DUKE3D_SHAREWARE=ON` with 1.4 or 1.5 is rejected at configure time. The 0.99,
1.0 and 1.1 variants require it; 1.3D supports both full and shareware builds.

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

### Playing the 0.99 beta

Use the matching beta `DUKE3D.GRP`, `GAME.CON`, `DEFS.CON` and `USER.CON`, with
`DUKE.RTS` if available. Unpack your copy of `d3d_beta.7z` with 7-Zip:

```
7zz x d3d_beta.7z -o/path/to/d3d_beta
cd /path/to/d3d_beta
/path/to/repo/build-v099sw/bin/duke3d
```

The archive contains the data files directly. The original beta has six E1 maps and
no bundled demos. Its normal skill is `/s3`; for example, `/v1 /l1 /s3` starts E1L1.
The beta build loads DOS beta saves and writes its own reloadable saves in the beta
layout. Beta and 1.0 saves are distinguished by layout; their unversioned demo headers
cannot distinguish the two releases. Keep each version's data, demos and saves separate.
Beta multiplayer is outside the verified single-player scope.

The optional [reference-demo workflow](docs/v099/reference.md#regenerating-reference-demos)
uses routes under `tools/v099/routes/` and checks native results against original DOS
playback before installing a reference. No beta data or reference demos ship here.

## Build options

Pass these with `-D` when configuring.

| Option | Default | Effect |
|---|---|---|
| `DUKE3D_VERSION` | `1.5` | Game version: `0.99`, `1.0`, `1.1`, `1.3D`, `1.4` or `1.5` |
| `DUKE3D_SHAREWARE` | `OFF` | Build a shareware version (`VOLUMEONE`); supported with `0.99`, `1.0`, `1.1` and `1.3D`, and required for the first three |
| `DUKE3D_ROUTEBOT` | `OFF` | Build the optional `-routebot` demo author; normal builds do not include it |
| `DUKE3D_V099_SW_DATA_DIR` | empty | Beta data directory for optional beta tests, including references in `refdemos/` and `refsaves/`; needs Python 3 |
| `DUKE3D_POLYMOST` | `ON` | Build the Polymost renderers (see [Renderers](#renderers)); needs OpenGL. `OFF` builds the classic renderer only |
| `DUKE3D_CONTROLS_MENU` | `ON` | In-game controls configuration menu |
| `DUKE3D_FETCH_SDL` | `AUTO` | `AUTO` uses installed SDL2 and fetches it if missing; `ON` always builds SDL2 2.32.10 from source; `OFF` fails if SDL2 isn't installed |

The default build type is `RelWithDebInfo`. CMake is the supported build
system; the legacy DOS/Watcom platform code and makefiles have been removed.

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

- Widescreen and tallscreen: modes wider than 4:3 use Hor+: they show more
  of the world horizontally while preserving the vertical field of view,
  including when looking up or down. The full status bar and multiplayer
  frag bar, including their numbers and icons, stretch across the screen
  without extra artwork at the sides. This applies to every renderer.
  320x200, 640x400 and other 4:3 modes draw as
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
| SUPERSAMPLE | OFF, 2K, 4K, 8K | Renderer 3 on the 2.1 path only. Draws at 1440, 2160 or 4320 pixels tall (2560x1440, 3840x2160 and 7680x4320 at 16:9) with the window's aspect, and scales down to the window, so the picture is smoothed. A level not taller than the window is greyed, so it only ever scales down, and a size over what the GPU can do is cut down with a message. If the GPU can't make the frame (out of video memory, say), supersampling turns off with a message and the 2.1 path draws at the window's size; the row shows, and saves, OFF. It is greyed with the 1.1 and 1.5 paths and renderers 0-2, and while `GLInternalWidth`/`GLInternalHeight` is set (the internal size wins). 8K uses a lot of video memory. While greyed, a level kept from before can still be stepped to OFF |
| FILTERING | NEAREST, BILINEAR, TRILINEAR | Renderer 3 only; greyed in renderers 0-2. On the 1.1 and 1.5 paths it sets the texture filter (trilinear uses mipmaps). On the 2.1 path, NEAREST keeps the exact palette-index drawing. BILINEAR and TRILINEAR switch it to colour drawing, see below |

Resolution in the 8-bit renderers (0-2): the size chosen is the size the game
draws its 8-bit picture at, up to the engine's 1600x1200 cap (the aspect is
kept). A bigger choice draws
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
ask for another ignore input, so a double press is one change. A switch to
OPENGL that fails keeps the renderer, and FULLSCREEN that the display can't
take falls back to borderless (or the mode in effect); both fade back in. A
resolution whose frame can't be allocated still quits with SDL's error, as it
always has.

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
The defaults use WASD to move and strafe, the mouse to look, Space to jump,
E to use, Ctrl or C to crouch, and Shift to run. Left mouse fires, right mouse
kicks, middle mouse uses, and the wheel changes weapons. Mouse look and the
crosshair start enabled; U toggles mouse look and I toggles the crosshair.
Existing configurations keep their saved bindings and settings.

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

Original DOS 1.5 (Atomic Edition) saves load on any OS and CPU, and the default build
writes saves in the same layout. The original DOS game can't load them back,
though. A 1.3D build loads DOS 1.3D saves instead (see
[Playing 1.3D](#playing-13d)). A 1.4 or 1.5 build reading 1.3D CON
files needs `UseOldVersion = 1` under `[Misc]` in `duke3d.cfg`; a 1.3D build
doesn't.

64-bit status: the game runs as a native 64-bit binary. That's verified on
macOS arm64; Linux and Windows are built and tested in CI. Demo recording and
playback and the editor's map saving are verified on 64-bit. Network play
builds but hasn't been verified on 64-bit.

### Gamepads

SDL2-compatible gamepads work automatically alongside keyboard and mouse,
including when connected after startup. SDL's controller mappings provide the
same layout on supported Xbox, PlayStation and other pads. Button names below
use the Xbox layout; A is the bottom face button and X is the left face button.

| Control | Action |
|---|---|
| Left stick | Move and strafe |
| Right stick | Look, with a dead zone to prevent drift |
| Right / left trigger | Fire / quick kick |
| A / B / X / Y | Jump / crouch / use / use inventory item |
| Left / right bumper | Previous / next weapon |
| Left / right stick click | Run / crouch |
| D-pad left / right | Previous / next inventory item |
| D-pad up / down | Medkit / jetpack |
| Back (View/Select) | Map |
| Start | Open or close the menu |

In menus, use the D-pad or left stick to navigate, A to confirm, and B or
Start to go back. The first supported controller is used. Set
`BUILD_SDLJOYSTICK` to a controller's SDL name to select it, or `none` to
disable controller detection. Existing raw joystick configuration remains
available for devices SDL does not recognize as game controllers.

### Command-line switches

Original game switches start with `/`. Native port switches also use `-`, including
`-renderer`, `-net`, `-demoverify`, `-loadverify` and the optional `-routebot`.
The table below describes the default 1.5 build; older versions keep their own switch
sets. Unknown `/` switches print help. Run with `/?` for your build's list.

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
| `DUKE3D_DOS_SAVES_DIR` alone | `save_hardening_codec` in 1.4/1.5 builds: checks compressed save structure and malformed offsets without booting the game or requiring a GRP. The 1.4 check uses the shared layout, not an original 1.4 capture; needs Python 3 |
| `DUKE3D_V13D_DATA_DIR` | In 1.3D builds: CON compile, game boot, demo playback, multiplayer and edition checks against the 1.3D full release. With DOS reference saves in its `refsaves/` folder (listed in `tests/data/v13d_save_refs.cmake`), also `dos_save_v13d_*` and `save_refuse_v13d_*`; needs Python 3 |
| `DUKE3D_V13D_SW_DATA_DIR` | The same for a 1.3D shareware build |
| `DUKE3D_V11_SW_DATA_DIR`, `DUKE3D_V10_SW_DATA_DIR` | The same for a 1.1 or 1.0 shareware build, using that release's folder. DOS reference saves go in its `refsaves/` folder |
| `DUKE3D_V099_SW_DATA_DIR` | Beta CON compile, boot, real-renderer camera, demo-format and DOS-save tests. Installed `refdemos/E1L<n>.DMO` files enable per-map sync checks; E1L5 also enables the 60-second episode-ending check; `refsaves/` holds the listed DOS save references. No beta network tests; needs Python 3 |
| `DUKE3D_NET_PEERS` | 1.3D, 1.1 and 1.0 builds: other builds, as `<exe>=<data folder>` entries, to check that multiplayer refuses mismatched versions |

Data-enabled 1.0, 1.1 and 1.3D builds automatically test refusals using small
protocol peers for mismatched BYTEVERSIONs. Names ending in `_protocol_117`
exercise the 1.5 full-version handshake; `_protocol_27` exercises the 1.5
shareware handshake, which has no build configuration. These peers test the
game's rejection path and require no other game's data. Optional
`DUKE3D_NET_PEERS` entries additionally exercise complete external game builds;
each entry needs the matching executable and data folder, using absolute paths.

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

To enable the beta data tests in a separate build:

```
cmake -S . -B build-v099sw -DDUKE3D_VERSION=0.99 -DDUKE3D_SHAREWARE=ON \
    -DDUKE3D_V099_SW_DATA_DIR=/path/to/d3d_beta
cmake --build build-v099sw
ctest --test-dir build-v099sw --output-on-failure
```

To regenerate a reference, use a separate build with `DUKE3D_ROUTEBOT=ON`, Python 3
and DOSBox-X:

```
cmake -S . -B build-v099-routebot -DDUKE3D_VERSION=0.99 \
    -DDUKE3D_SHAREWARE=ON -DDUKE3D_ROUTEBOT=ON
cmake --build build-v099-routebot
sh tools/v099/mkrefdemos.sh /path/to/d3d_beta build-v099-routebot 1
```

The final argument selects E1L1; pass other level numbers 1–6, or omit them to process
all six. Add `--dosbox /path/to/dosbox-x` if it is not on PATH. The tool writes a
reference demo and its `tests/data/v13d_demo_refs.cmake` values only after native
recording, native replay and the DOS capture agree. It stops on a mismatch. See the
[beta reference](docs/v099/reference.md#regenerating-reference-demos) for the god-mode
flags and files it updates.

CI (`.github/workflows/build.yml`) builds and tests 1.5 on Ubuntu, macOS and Windows,
and 1.4, 1.3D full/shareware, 1.1, 1.0 and 0.99 on Ubuntu. It has no game data, so
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

The engine files in `source/build/` are grouped by responsibility:

| File | Contents |
|---|---|
| `engine.c` | World rendering, geometry, collision, map I/O and engine initialization |
| `display.c` | View setup, pixel/text drawing, palettes, brightness and fades |
| `draw.c` | Low-level C scanline, column and slab drawing |
| `tiles.c` | ART/voxel loading, tile operations, animation and sprite lists |
| `filesystem.c` | GRP access, file reads/seeks and compressed reads/writes |
| `cache.c` | Cache allocation, eviction, aging and diagnostics |
| `math.c` | Fixed-point and memory helpers, math tables, angles and square roots |
| `network.c` | Multiplayer transport |
| `polymost.c` | Shared Polymost scene traversal, clipping/projection and renderer dispatch |
| `polymost_soft.c` | Polymost software rasterization and depth buffer (modes 1/2) |
| `polymost_opengl.c` | OpenGL shaders, textures/framebuffers, polygon submission and GL state |

`polymost.c` remains included by `engine.c` and includes its two backend
implementations, with the OpenGL file guarded by `USE_OPENGL`. All three are
listed as include-only sources in CMake, preserving private engine state.
`sdl_driver.c` owns the SDL
window, input, timer and presentation backend. `engine_internal.h` contains
private declarations between engine modules. Existing public header names,
including `a.h`, `cache1d.h` and `pragmas.h`, are retained.

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

The original DOS/Watcom build instructions have been retired. Use the CMake
build instructions above for this port.

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
