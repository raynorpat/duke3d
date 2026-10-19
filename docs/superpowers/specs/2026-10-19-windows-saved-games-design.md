# Windows Saved Games Folder — Design

Date: 2026-10-19
Status: Approved (brainstorming)

## Goal

On Windows, `duke3d.cfg` and save games live in the user's Saved Games folder, under
`Saved Games\Duke Nukem 3D\`, instead of the game's working directory. Existing files in
the working directory are carried over on first run.

## Today

`setup_homedir()` (`source/global.c`) sets `ApogeePath`. Every per-user file goes through
`GetPathFromEnvironment()`, which prefixes it:

- `duke3d.cfg` (`config.c`)
- `game0.sav`–`game9.sav` and multiplayer `gameA_xx.sav` (`menues.c`, `config.c`
  `readsavenames`)
- `tmpsong.mid` (`dukemusc.c`, `fx_man.c`)

The platforms differ:

- **macOS:** `~/Library/Application Support/Duke Nukem 3D/`.
- **Linux:** `~/.duke3d/`.
- **Both:** if `duke3d.cfg` exists in the working directory but not in that folder, it is
  copied there.
- **Windows:** `setup_homedir()` does nothing, so `ApogeePath` is empty and everything
  goes to the working directory.

`setup_homedir()` runs from `Startup()`, after `main()` has already opened `duke3d.grp`.

## Design

### 1. Choosing the folder (`setup_homedir`, Windows branch)

Add a `#elif PLATFORM_WIN32` branch. It picks a base folder, in order:

1. **`DUKE3D_SAVE_DIR`:** the environment variable, if it is set and not empty. It
   exists so tests can stay out of the real profile. A comment in the code says so, and
   the README doesn't mention it.
2. **The Saved Games known folder:**
   - call `SHGetKnownFolderPath(&FOLDERID_SavedGames, 0, NULL, &wide)`;
   - convert the result with `WideCharToMultiByte(CP_ACP, ...)`, so the game's existing
     `fopen`/`open` calls can use it;
   - free it with `CoTaskMemFree`.

   If the call fails, or the path can't be represented in the ANSI code page, continue
   to step 3.
3. **`%USERPROFILE%\Saved Games`:** used if `USERPROFILE` is set.
4. **Nothing:** `ApogeePath` stays empty, which is today's behaviour.

For steps 1–3:
- `ApogeePath` becomes `<base>\Duke Nukem 3D\`, with a trailing backslash because
  `GetPathFromEnvironment` concatenates.
- `CreateDirectoryA` creates `<base>` (needed for step 3) and then `<base>\Duke Nukem 3D`.
  An "already exists" error is fine.
- If creating `Duke Nukem 3D` fails for any other reason, print
  `Couldn't create the save folder <path>; using the game folder.` to stderr and fall
  back to step 4.

The resulting path must fit `ApogeePath` (256 bytes). If it doesn't, use step 4.

### 2. First-run copy (Windows)

After the folder is chosen (steps 1–3 only), the game copies each of these files from
the working directory to the save folder, but only when it exists in the working
directory and not yet in the save folder:

- `duke3d.cfg`
- `game0.sav` … `game9.sav`

A small static helper in `global.c` does the copy, byte for byte, in binary mode. It
closes both files, and deletes a partial copy if the write fails. The
originals stay where they are, so an older build still finds them. Multiplayer
`gameA_xx.sav` files aren't copied.

macOS and Linux behaviour is unchanged.

### 3. Build

The Windows build links `shell32` (`SHGetKnownFolderPath`) and `ole32` (`CoTaskMemFree`).
Add them to `duke3d_game` (which builds `global.c`) under `if(WIN32)` in
`source/CMakeLists.txt`, the same way `buildengine` adds `ws2_32 winmm`. `FOLDERID_SavedGames` comes from `<knownfolders.h>`
with `<shlobj.h>`. With MinGW it may need `INITGUID`, or `<initguid.h>` before
`<knownfolders.h>`.

## Testing

1. **New CTest `save_dir`:**
   - Windows only. It needs no game data, so it runs in CI.
   - It's a CMake `-P` script, `tests/save_dir.cmake`.
   - Run directory setup:
     - a committed 16-byte empty GRP, `tests/data/empty.grp`: `KenSilverman` followed by a
       32-bit file count of 0, so `main()` gets past the GRP check;
     - a `duke3d.cfg` containing a marker line;
     - a `game3.sav` containing marker bytes.
   - It runs `duke3d` with `DUKE3D_SAVE_DIR=<scratch>\saves`, `SDL_VIDEODRIVER=dummy` and
     `TIMEOUT 15`. The game will stop or hang later for lack of data; that doesn't matter.
   - It passes if `<scratch>\saves\Duke Nukem 3D\duke3d.cfg` and `game3.sav` exist with
     the marker contents, and the originals are still in the run directory.
2. **Existing tests stay out of the real profile on Windows:** `game_boot` and the DOS
   save tests set `DUKE3D_SAVE_DIR` to their scratch home.
   - `game_boot`'s Windows `CFG_DIR` becomes the save folder:
     `<home>/Duke Nukem 3D/duke3d.cfg` with `DUKE3D_SAVE_DIR=<home>`.
3. **Unchanged suites:** macOS and Linux keep their existing results.
4. **Not covered automatically:** the real `SHGetKnownFolderPath` path. It's reviewed by
   reading the code. If a Windows machine is available, the user checks it once:
   - start the game;
   - confirm `duke3d.cfg` appears in `Saved Games\Duke Nukem 3D`;
   - confirm a save appears there too.

## Docs

`README.md`: the Windows config location changes from "the working directory" to
`Saved Games\Duke Nukem 3D`. Add one sentence saying that an existing `duke3d.cfg` and
`game*.sav` in the game folder are copied there on first run.

## Out of scope

- Moving the files instead of copying them.
- Copying multiplayer saves.
- Changing the macOS or Linux locations.
- Unicode paths beyond the ANSI code page. Paths that can't be represented fall back to
  the game folder.
