# Loading DOS Saves — Design

Date: 2026-10-19
Status: Approved (brainstorming)

This is sub-project 3 of 3 for running duke3d on 64-bit targets. It follows the core
cleanup and the demos/editor/network work, both merged into `main`.

1. Core 64-bit cleanup (done)
2. Demos, editor map saving and network play on 64-bit (done)
3. **Loading original DOS `.SAV` files** (this spec)

## Goal

The game loads save files written by the original DOS Duke Nukem 3D 1.5 (Atomic Edition,
`BYTEVERSION` 117) through the normal Load menu, on every supported OS and CPU, including
big-endian ones. Its own saves use the same DOS layout, so there is one save format.

## Findings

The block sizes in a real Atomic save (`Episode I/Game0.sav`) give the DOS layout of every
field `saveplayer` writes. Every `dfwrite` call becomes one or more LZW blocks. Each block
starts with a 16-bit compressed length, then a 16-bit uncompressed length and a 16-bit
string count.

- The DOS game was built with 1-byte struct packing:
  - `struct player_struct`: 911 bytes per player, 14576 for `ps[16]`.
  - `struct weaponhit`: 71 bytes; `hittype[4096]` is 290816 bytes.
  - `struct animwalltype`: 6 bytes; `animwall[512]` is 3072 bytes.
- Ours use natural alignment, at 920, 72 and 8 bytes. Packed, our `player_struct` is 915
  bytes: DOS's 911 plus 4, because `char *palette` is 8 bytes instead of 4. The field order
  is the same in all three.
- Every other block already matches DOS byte for byte in size: the header, thumbnail,
  `wall`/`sector`/`sprite`, the sprite lists, `cyclers`, `po`, `msx`/`msy`, the sprite
  queue, mirrors, `show2dsector`, `actortype`, `boardfilename`, clouds, `script`,
  `actorscrptr`, `pskyoff`, the animate arrays and the trailing scalars.
- Pointer-valued data is stored as byte offsets in DOS saves and in ours: `script[]`
  entries, `actorscrptr[]`, `hittype[].temp_data` slots 1, 4 and 5 (from `script`), and
  `animateptr[]` (from `sector`). The DOS saves also store two "which entries are
  offsets" bitmaps (`scriptptrs`, `MAXSCRIPTSIZE` and `MAXSPRITES` bytes). Our values are
  always offsets, so the bitmaps carry no information we need.
- `loadplayer` reads `ps[]` raw, so a DOS save's `palette` field would leave a 32-bit DOS
  address in a 64-bit pointer.
- `dfwrite` delta-encodes each element against the previous one, byte by byte, using the
  `dasizeof` it is called with. A serialized buffer written with the same `dasizeof` and
  count therefore produces exactly the bytes DOS produced.

Test data:

- 46 Atomic saves in `~/Downloads/pc-duke-nukem-3d-atomic-edition-savegame/` (Episodes I–IV,
  levels 1–11). Every one starts with `BYTEVERSION` 117.
- The Atomic CD image `~/Downloads/Duke Nukem 3D (USA) (Atomic Edition)/`, whose
  `ATOMINST/` folder holds the Atomic `DUKE3D.GRP`, `GAME.CON`, `DEFS.CON` and `USER.CON`.

## Design

### 1. One save format: the DOS 1.5 layout, little-endian

`saveplayer`, `loadplayer` and `loadpheader` in `source/menues.c` read and write every
block in the DOS layout, in the existing order, with every multi-byte value
little-endian:

- **Serialized blocks.** Each block is converted to a little-endian byte buffer, then
  passed to `dfwrite` with the same `dasizeof` and count the DOS code used. Loading does
  the reverse: `kdfread` into a byte buffer, then decode. This covers:
  - the packed structs `player_struct` (911 bytes), `weaponhit` (71) and `animwalltype` (6);
  - every multi-byte array and scalar (`wall`, `sector`, `sprite`, the `short` and
    `int32_t` arrays, `script`, the `ud` fields and the rest), so that a big-endian host
    reads and writes the same bytes as a little-endian one.

  Single-byte arrays (thumbnail, `show2dsector`, `actortype`, `boardfilename`, the
  bitmaps) are passed through unchanged.
- **The codec.** It lives in a new pair of files, `source/savegame.c` and
  `source/savegame.h`:
  - put/get helpers for 8-, 16- and 32-bit little-endian values into a byte buffer,
    built from shifts, never from `memcpy` of an integer;
  - one encode and one decode function per struct type (`walltype`, `sectortype`,
    `spritetype`, `player_struct`, `weaponhit`, `animwalltype`, `player_orig`), field by
    field in declaration order;
  - array helpers for `short` and `int32_t`.

  `menues.c` keeps the call order and the decisions about what to save.
- **`palette`.** `player_struct.palette` is written as 4 zero bytes. On load those bytes
  are skipped, and each `ps[i].palette` is set to `(char *) &palette[0]`. The game already
  switches it for water, slime and other effects as play continues.
- **Bitmaps.** The `scriptptrs` bitmaps are read and ignored, and written as zeros, as
  today.
- **The core work's asserts.** The `BUILD_STATIC_ASSERT`s in `duke3d.h` for `weaponhit`
  (72) and `animwalltype` (8) existed only so saves kept their size. They are removed,
  because the save layout no longer depends on in-memory sizes. The `sectortype`,
  `walltype`, `spritetype`, `script[0]` and `input` asserts stay, because maps and demos
  still read and write those structs directly.

### 2. Refusing files that aren't DOS 1.5 saves

- **Version check.** `loadplayer` and `loadpheader` keep the `BYTEVERSION` check, which
  already rejects 1.3D (103) and other versions.
- **Layout check.** In addition, before decoding `ps[]`, `loadplayer` peeks at the next
  LZW block's uncompressed length. If it isn't 14576, the file is refused with
  `"<file> is not a DOS 1.5 save (player block is N bytes)"`. The peek uses a new
  `kdfpeekleng(int32_t fil)` in `source/build/cache1d.c`. It reads the 4-byte block header
  and seeks back. Nothing is loaded into game state before this check, so a refused file
  leaves the running game untouched.
- **What it catches.** This rejects saves from the 64-bit build before this change (14720)
  and from the original 32-bit Linux port (16 × the natural 32-bit size), both of which
  share `BYTEVERSION` 117.

### 3. Headless load check: `-loadverify <file>`

- **Behaviour.** A new command-line switch, `-loadverify <file>`, parsed with `CheckParm`
  (so it takes `-`), makes the game start headless as it does today with
  `SDL_VIDEODRIVER=dummy`, then load `<file>` with `loadplayer` instead of entering the
  title loop.
- **Success.** It prints one line and exits 0:
  ```
  SAVESTATE <health> <armor> <curweapon> <ammo> <volume> <level> <x> <y>
  ```
  The fields:
  - `health`: `sprite[ps[0].i].extra`
  - `armor`: `ps[0].shield_amount`
  - `curweapon`: `ps[0].curr_weapon`
  - `ammo`: `ps[0].ammo_amount[ps[0].curr_weapon]`
  - `volume`, `level`: `ud.volume_number`, `ud.level_number`
  - `x`, `y`: `ps[0].posx`, `ps[0].posy`
- **Refusal.** A refused save prints the refusal message and exits 2.
- **Shared interface.** The `v13d` plan's Task 17 consumes this interface for 1.3D saves.
  `volume` is included here so it identifies episodes, not only levels.
- **Load path.** `loadplayer` also takes an explicit file name for this path. Today it
  only takes a slot number.

## Test setup

- **Data.** A one-time setup step copies `ATOMINST/DUKE3D.GRP`, `GAME.CON`, `DEFS.CON` and
  `USER.CON` out of the CD image (an ISO 9660 track, read-only) into
  `~/duke3d-data/atomic15/`. The extractor is a committed script,
  `tools/extract_iso_files.py`, which takes the `.bin`, a target directory and file names.
  Nothing from the commercial data goes into the repo.
- **CMake cache variables:**
  - `DUKE3D_ATOMIC_GRP`: path to the Atomic `DUKE3D.GRP`;
  - `DUKE3D_DOS_SAVES_DIR`: path to the folder of DOS 1.5 saves (searched recursively for
    `*.sav`, case-insensitive).

  The DOS save tests are registered only when both are set.

## Testing

1. **Codec unit test `savegame_codec`** (CTest, always runs, no game data). It covers each
   struct and helper:
   - encode a known value, then compare against expected little-endian bytes;
   - decode those bytes back, then compare field by field;
   - check the encoded sizes are 911, 71, 6, 32, 40, 44 and 16.

   The expected bytes are built independently in the test with explicit byte literals.
2. **`dos_save_load`** (one CTest per save, needs both cache variables):
   - runs `duke3d -loadverify <save>` in a scratch run directory containing the Atomic GRP
     and CONs;
   - compares the `SAVESTATE` line with values decoded by `tests/dos_save_decode.py`, an
     independent Python decoder of the DOS layout (LZW blocks and field offsets);
   - the decoder runs at test time, with `python3 -I`, so no values derived from the saves
     are committed.
3. **`dos_save_roundtrip`** (needs both cache variables):
   - for 3 saves, one each from Episodes I, II and IV, loads the save headless and
     immediately saves it to a scratch slot with a new `-saveverify <slot>` switch, which
     runs `saveplayer` after the load and exits;
   - the decoder then compares the two files block by block. Every block's decoded bytes
     must be identical, except the two `scriptptrs` bitmaps and the 4 `palette` bytes of
     each player.
4. **`dos_save_refuse`** (needs both cache variables). Each of these must exit 2 with
   the refusal message:
   - **An old-layout save.** This stands in for a save from the 64-bit build before this
     change. The decoder writes it as a copy of a DOS save whose player block is replaced
     by a 14720-byte block (each player padded with 9 zero bytes). The replacement is a
     stored block: uncompressed length 14720 and string count 0, which `uncompress`
     copies raw. That way no LZW compressor is needed.
   - **A different `BYTEVERSION`.** A copy of a DOS save with its version patched to 103.
     The version block is stored uncompressed, so this is a 1-byte change.
5. **Existing suite** still passes: `sdl_driver_smoke`, `con_compile`, `render_e1l1` and
   `game_boot`.
6. **Manual (macOS, real window):**
   - The controller loads 4 saves (one per episode) through the Load menu and checks the
     status bar against the decoder.
   - The user plays a couple of minutes from one save and checks that enemies, doors and
     elevators move.
   - The user saves, then loads the new save.
7. **Big-endian.** No big-endian machine or CI runner is available. Correctness comes
   from construction: shift-based helpers and no raw multi-byte I/O in the save code. The
   codec unit test checks byte output against literals, which a big-endian host would also
   need to match. Running the suite on a big-endian host isn't part of this work.

## Out of scope

- The original DOS game loading saves from this build. That would need correct
  `scriptptrs` bitmaps, which the build can't reconstruct.
- DOS 1.3D saves. They belong to `v13d` Task 17, which builds on the `-loadverify`
  interface and the layout check here.
- Big-endian support anywhere other than saves (maps, ART, demos, network), and running
  the tests on big-endian hardware.
- Network savegame loading (packet 126) beyond what `loadplayer` already does. It calls
  the same loader and inherits the new format.

## Definition of done

1. macOS arm64 builds with zero errors.
2. `ctest` passes:
   - without game data: the existing tests plus `savegame_codec`;
   - with `DUKE3D_ATOMIC_GRP` and `DUKE3D_DOS_SAVES_DIR` set: all 46 `dos_save_load`
     tests, `dos_save_roundtrip` and `dos_save_refuse`.
3. The manual checks pass.
4. When the user pushes, all three CI jobs build and pass.
5. `README.md` says DOS 1.5 saves load, and documents the two cache variables and the
   extraction step.
