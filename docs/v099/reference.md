# 0.99 Beta Reference Material

This file records the reference material for the 0.99 pre-release beta build variant
(spec `docs/superpowers/specs/2026-10-21-v099-variant-design.md`). None of the beta files
are in this repository. Only their location, checksums and the values measured from them
are.

## Data location

`~/Downloads/d3d_beta/`, unpacked from `~/Downloads/d3d_beta.7z`. Tests read it through
`DUKE3D_V099_SW_DATA_DIR`. Nothing is run inside it except writing the `refdemos/` and
`refsaves/` folders. Runs use scratch copies or symlinks.

`README.TXT` (Jan 5 1996) is a note to the publisher: "This version is really stable for
single player. We are working the next week on net/modem and then we should be ready to
release the game."

## The beta

- `DUKE3D.EXE` reports `Duke Nukem 3D Pre-Release "Beta" version .99`. It is 851,779 bytes,
  dated Jan 4 1996, a Watcom C/C++32 program bound to DOS/4GW (LE header at offset 10832).
- `DUKE3D.GRP` is 9,690,241 bytes with 166 entries: six maps `E1L1` to `E1L6`, the CONs,
  7 MIDIs and no demos.
- `GAME.CON`, `DEFS.CON` and `USER.CON` have 2,422, 780 and 572 lines. The loose copies are
  byte-identical to the ones inside the GRP.
- Against 1.0 (whitespace-insensitive), 423 lines of `GAME.CON`, 49 of `DEFS.CON` and 383
  of `USER.CON` differ.
- The EXE has the shareware order screens, `/r` ("Record a demo"), `demo1.dmo`,
  `game0.sav` and the save/load menus, so the beta can record demos and has saves.
- Old CON keywords the scripts use: `activate` once, `shadeto` twice, and
  `definelevelname` with a single time (no 3D Realms par time). `endoflevel`, `ifplayersl`
  and `rotatespritea` are not used.

## Checksums

Verified with `md5 -q` on 2026-10-21.

| File | MD5 |
|---|---|
| DUKE3D.EXE | `18c7f243e10e6953c298c24e4c798e41` |
| DUKE3D.GRP | `56b35e575eba7f16c0e19628bd6bd934` |
| GAME.CON | `90130f9a2f0b0ab42975f2602e1456ba` |
| DEFS.CON | `53a1e1568da0b9078e199cb312268717` |
| USER.CON | `b2bf074f6af92d87393fbb8ef60986da` |

The GRP MD5 is the edition check for the data tests. Recheck all five after each DOSBox
session.

## EDuke32 reference commits

These commits in the EDuke32 repository (`https://voidpoint.io/terminx/eduke32`) are GPL.
They are hypotheses to confirm against the EXE: EDuke32 aims to run the scripts, not to
match the EXE tic for tic.

| Commit | Meaning |
|---|---|
| `8ea1a4d8` | "Add support for shareware 0.99/1.0/1.1". Version detection from keywords, `shadeto`, `activate`, startup parameters by version, LIZTROOP and RPG smoke gates. <https://voidpoint.io/terminx/eduke32/-/commit/8ea1a4d834b5ed8affc0cfbea9d00a3dcf75b66d> |
| `8b9f4345` | Makes `activate` overwrite its own opcode slot with `operateactivators` and a 0 argument, which fixes a crash after the 0.99 boss. <https://voidpoint.io/terminx/eduke32/-/commit/8b9f43458a568daf2f07d99592416a520bd23ff4> |
| `ed97e5d0` | Corrects detection of the 3D Realms par time in `definelevelname`, which 0.99 lacks. <https://voidpoint.io/terminx/eduke32/-/commit/ed97e5d0e7dbce26a91f8c81bea91541489a79e2> |

## Executable layout

`DUKE3D.EXE` is a DOS/4GW-bound Watcom program like 1.0, but laid out more simply. The
outer MZ's `e_lfanew` (`0x3c`) is valid and points straight at the `LE` header at file
offset `0x2a50` (10832). There is no separate stub, so the data-page offset is relative to
the start of the file. There are two objects, not three.

| | 0.99 | 1.0 (for comparison) |
|---|---|---|
| MZ stub (file offset) | `0` (none) | `0x26654` |
| LE header (file offset) | `0x2a50` | `0x290fc` |
| Data pages | `0x36a00` (file-relative) | `0x39000` (stub-relative) |
| Pages | `0x9a` (last page `0x543` bytes) | `0xa5` |
| Entry point | `0x70b6c` | `0x988b4` |
| Fixups | type 7: 24605 | type 7: 25695 |

| Object | Base | Size | File pages | Contents |
|---|---|---|---|---|
| 1 | `0x10000` | `0x89b1f` | 138 | code |
| 2 | `0xa0000` | `0x1a71a0` | 16 | data and BSS (initial ESP `0x2471a0`, the top of the object) |

All fixups are internal.

## Disassembly method

The 1.0 method (`docs/v10-v11/reference.md`), with the same tools under `~/Projects/ref/`:

1. `tools/le_flatten.py DUKE3D.EXE 2a50 ghidra/beta099.bin 0` gives one raw image based
   at `0x10000`, `0x2371a0` bytes, MD5 `9af9643821946cb992613f44f7322661`.
2. It is imported into the Ghidra project `ghidra/duke13` as `beta099.bin` (next to
   `sw10.bin`), raw x86:LE:32 at `0x10000`:
   `analyzeHeadless ghidra duke13 -import beta099.bin -processor x86:LE:32:default
   -loader BinaryLoader -loader-baseAddr 0x10000 -preScript SetupLE.java 70b6c 99b1f a0000
   -postScript ApplyWatcall.java`. `SetupLE.java` now takes an optional third argument,
   the data object's base, because the beta's data starts at `0xa0000`, not `0xb0000`
   (the default, so the earlier imports are unchanged). Analysis finds 1228 functions,
   and `ApplyWatcall` gives all of them the `__watcall` prototype. The log is
   `ghidra/import_beta099.log`.
3. `tools/fmatch.py` against `sw10.bin` (1290 functions): 742 exact, 210 by order, 338
   unmatched (`ghidra/fmap_beta099.txt`). The engine and most library code match
   exactly. The game code is linked in a different order (actors first, at `0x11000`), so
   most game rows below were found by strings, call graphs and data use, then checked in
   the decompile.
4. Decompiles are in `ghidra/dc_beta099/`, the function list with callees in
   `ghidra/funcs_beta099.txt`.

## Address map

The 1.0 address is given for comparison. "Call position" means the function is at the
same place in domovethings' call sequence as its 1.0 counterpart.

### Functions

| Name | Module | 0.99 | 1.0 | How identified |
|---|---|---|---|---|
| getpackets | game | `0x1cac0` | `0x106d0` | packet-type switch on `packbuf[0]` (`0xf5530`); packet 5 calls newgame and enterlevel; called by the main loop and playback |
| getinput | game | `0x3f3b0` | `0x43520` | fmatch order; fills `loc` (`0xb0540`–`0xb0549`) from the control state; only caller `0x1d300` |
| opendemoread | game | `0x2a270` | `0x1f1f0` | `demo_.dmo` string; fmatch exact |
| opendemowrite | game | `0x2a330` | `0x1f2b0` | `demo1.dmo` string; only caller enterlevel |
| record (the demo writer) | game | `0x2a3e0` | `0x1f360` | called by domovethings when `ud.recstat == 1`; copies `sync[]` into the record buffer |
| closedemowrite | game | `0x2a4d0` | `0x1f470` | fmatch exact; callers gameexit (`0x20930`) and the main loop's MODE_END exit |
| playback | game | `0x2a560` | `0x1f500` | only caller of opendemoread; demo-end `jnz` at `0x2a726` |
| domovethings | game | `0x2bab0` | `0x20ae0` | calls record, krand and processinput |
| processinput | game | `0x40230` | `0x441e0` | "KILLED BY" string (`0x40981`); fmatch order |
| movedummyplayers | game | `0x118f0` | `0x24c10` | call position; walks statnum 13 |
| movefta | game | `0x112b0` | `0x24360` | call position; walks statnum 2 |
| moveweapons | game | `0x13e00` | `0x27890` | call position; walks statnum 4 |
| movetransports | game | `0x14ad0` | `0x285f0` | call position; walks statnum 9 |
| moveplayers | game | `0x11aa0` | `0x24e30` | call position; walks statnum 10 |
| movefallers | game | `0x121c0` | `0x25770` | call position; walks statnum 12 |
| moveexplosions | game | `0x1b660` | `0x302a0` | call position; walks statnum 5 |
| moveeffectors | game | `0x15630` | `0x294a0` | call position; walks statnum 3 |
| moveactors | game | `0x19440` | `0x2db00` | call position; walks statnum 1 |
| movestandables | game | `0x12490` | `0x25aa0` | call position; walks statnum 6 |
| movefx | game | `0x11e90` | `0x252a0` | call position; walks statnum 11 |
| movecyclers | game | `0x11800` | `0x24af0` | call position; walks `cyclers[numcyclers]` |
| movesprite | game | `0x10b60` | `0x23c30` | calls setsprite, clipmove and changespritesect; callers moveweapons, moveactors, the CON `move` helper `0x305b0` and `0x10f10`. Unlike 1.0's, it does not call krand |
| execute | game | `0x34600` | `0x377f0` | called by exactly the five movers that run actor code; calls parse |
| parse | game | `0x30f80` | `0x34840` | recursive, and its only other caller is execute |
| cheats | game | `0x27820` | `0x1c630` | reads the cheat table at `0xa57de` (see "Cheats") |
| enterlevel | game | `0x45710` | `0x49e90` | copies `m_recstat` to `recstat`; calls opendemowrite |
| gameexit | game | `0x20930` | `0x14bc0` | prints its message, calls closedemowrite |
| loadpheader | game | `0x34ec0` | `0x38130` | `game0.sav` string; reads the header only |
| loadplayer | game | `0x34fb0` | `0x38220` | `game0.sav` string; mirrors saveplayer's reads |
| saveplayer | game | `0x35560` | `0x388d0` | `game0.sav` string; dfwrite sequence |
| krand | engine | `0x6151e` | `0x661e7` | the only use of `0x1a4e86d`; fmatch exact |
| kdfread | engine | `0x4db7f` | `0x7083b` | fmatch exact |
| dfwrite | engine | `0x4dfa9` | `0x70c65` | fmatch exact |
| compress | engine | `0x4e225` | `0x70ee1` | fmatch exact |
| uncompress | engine | `0x4e401` | `0x710bd` | fmatch exact |

domovethings (`0x2bab0`) is shorter than 1.0's. In order:
- counters, then the input copy from the per-player FIFO (`0xdbed8`, 8 × 10 bytes per
  slot) into `sync[]`
- the earthquake countdown and the sync-check byte (`randomseed`'s low byte)
- record, when `ud.recstat == 1`
- when not paused, krand and movedummyplayers
- for each player: `0x4ae20` (1.0 `0x4fb10`), then processinput and checksectors
  (`0x4b880`) when not paused
- when not paused: movefta, moveweapons, movetransports, moveplayers, movefallers,
  moveexplosions, moveeffectors, moveactors, movestandables, doanimations (`0x46160`),
  movefx
- every other tic, animatewalls (`0x46580`) and movecyclers
- `0x2b850`, which corrects the predicted position (1.0 `0x1faf0`)

So the mover order is 1.0's, with the same moveexplosions, moveeffectors, moveactors
sequence. 1.0 also calls `pan3dsound` (`0x51ed0`) every other tic, and the beta doesn't.

### Globals and constants

| Name | 0.99 | 1.0 | Notes |
|---|---|---|---|
| `keyw[]` (CON keywords) | `0xa5868` | `0xc6788` | pointer table, starting with `definelevelname`. A pointer scan gives about 93 entries, ending at `globalsound`. Task 3 dumps the table |
| `which_demo` | `0xa5866` | `0xc6783` | byte, initial value 1; wraps from 4 back to 1 in playback |
| `randomseed` | `0x210c30` | `0x22aaf0` | operand of krand's `imul` |
| `sprite[]` | `0x18ce5c` | `0x1a6d1c` | 4096 × 44 bytes |
| `ps[]` | `0xf0f38` | `0x112ab0` | **8 × `0x33f` bytes** (1.0: 8 × `0x34e`); saveplayer writes `(ps, 0x33f, 8)` |
| `po[]` | `0xf0eb8` | | 8 × 16 bytes |
| `hittype[]` | `0xb8480` | | 2048 × 71 bytes |
| `ud.reccnt` | `0xf0cd8` | `0x130ebb` | first field read by opendemoread; compared with 0 in playback's demo-end test |
| `ud.recstat` / `ud.m_recstat` | `0xf0d0c` / `0xf0d1c` | | `/r` sets `m_recstat = 1`; playback sets it to 2; enterlevel copies it |
| `ud.multimode` | `0xf0cca` | `0x130e97` | short |
| `ud.volume_number`, `level_number`, `player_skill` | `0xf0d3c`, `0xf0d38`, `0xf0d34` | | |
| `sync[]` / `loc` | `0xe0f28` / `0xb0540` | | `sync[]` is read by processinput and record; playback writes its records into the input array `0xe0ed8` (8 × 10 bytes) and `0x2a9d0` queues them in the FIFO |
| `headspritestat[]` | `0x210c56` | `0x22ab10` | statnum 13 is `0x210c70` |
| `script[]` | `0xe1018` | `0x102f08` | 10240 longs (`0x2800`), as in 1.0 |
| `actorscrptr[]` | `0xec808` | | 4096 longs |
| `lockclock` / `totalclock` | `0x108544` / `0x210c40` | | domovethings adds 4 to `lockclock` |
| `BYTEVERSION` | none | none | see below |
| Code cave | `0x99b20`, `0x4e0` bytes | `0xa2aa0`, `0x560` bytes | zero slack after the end of the code object (`0x99b1f`), to the end of its last file-backed page (`0x9a000`) |
| Log slack | `0x2471a0`, `0xe60` bytes | `0x2abaf0` | end of object 2, up to the page end `0x248000`. Room for 256 krandlog entries (`12 + 4 × 256` bytes). Not used at run time yet |

### player_struct (partial)

`ps[]` is 8 × `0x33f` bytes at `0xf0f38`. Only the fields this task needed were read. A
full map is left to later tasks.

| Offset | Size | Field | 1.0 | Evidence |
|---|---|---|---|---|
| `0x214`, `0x218`, `0x21c` | 4 each | `posx`, `posy`, `posz` | same | matched the player sprite in both STATE.BIN dumps |
| `0x270` | 2 | `ang` | `0x27c` | processinput adds `sync.avel` to it (`ps+0x270`) |
| `0x2a8`, `0x2aa` | 2 each | `fta`, `ftq` | | cheats set `fta = 100` and `ftq` to the quote |
| `0x2bc` | 2 | `i` (player sprite) | `0x2ca` | used to index `sprite[]` in movedummyplayers and the cheats |
| `0x2cc` | 2 | `cheat_phase` | `0x2da` | cheats |
| `0x2dc` | 1 | `gm` | `0x30c` | tested for MODE_MENU (1), MODE_EOL (8), MODE_TYPE (16), MODE_RESTART (32) and MODE_END (128) |

### BYTEVERSION

**The beta has none, as 1.0 has none. Ruling: `BYTEVERSION` is `9` for the 0.99 build.**

- getpackets packet 6 (name) copies the name from `packbuf[1]`, with no version byte.
- saveplayer's first write is the save name (`0x13` bytes). There is no version long.
- loadplayer and loadpheader read no version and check none.
- The "You cannot play Duke with different versions" string is not in the EXE.

`9` collides with nothing in `source/duke3d.h` (10, 12, 14, 103, 116, 27, 117). It only
matters for our own packets and our own save header, because neither DOS format carries
it. A DOS 0.99 save is told apart from a DOS 1.0 one by its layout (see "Save file"), as
`sg_check10` does for 1.0.

### Capture-patch notes

- **Demo-end pattern.** The `found demo` flag sits at `[esp+8]`, not `[esp+4]`. So the
  pattern is `cmp [reccnt],0 / jg / cmp byte [esp+8],0 / jnz END`. It matches exactly once,
  at `0x2a718`, with the `jnz` at `0x2a726`. `tools/v13d/mkcapture.py`'s regular
  expression needs that byte changed for 0.99.
- **The STATE.BIN dump** is `8 + 4096 × 44 + 0x33f` = 181,063 bytes. The 213-byte patch
  fits in the `0x4e0`-byte cave, and so would the 319-byte `krandlog` version.
- **krandlog's call-site scan** walks the flat image up to `0xa0000`, an offset that
  reaches address `0xb0000`. In the beta, data starts at `0xa0000`, so the scan must stop
  at the end of the code object (`0x9a000`). Otherwise it could patch data bytes that look
  like `call krand`.
- **mksave.** saveplayer (`0x35560`) has no `recstat` or `gm` check. It opens
  `game<n>.sav`, writes and closes it, and sets `ps[myconnectindex]+0x322 = 1`. So it can
  be called from a playback hook, as `tools/v13d/mksave.py` does for 1.0. The thumbnail
  is written from the buffer pointer at `0x17dd50`. domovethings for the hook is
  `0x2bab0`, called from playback at `0x2a830`.

## Formats

Everything here is read from opendemoread, opendemowrite, record, playback,
closedemowrite, enterlevel, the main loop (`0x29b70`), saveplayer, loadplayer,
loadpheader and getpackets. All multi-byte fields are little-endian.

### Compression

`compress` and `uncompress` are the 1996 LZW (both fmatch exactly against 1.0 and so
1.3D), and `kdfread`/`dfwrite` too. It was also checked on data. The demo recorded in
"DOS sanity check" decodes with `tests/v13d_save_decode.py`'s `uncompress` into exactly
its 541 records, and the block chain ends exactly at the end of the file (543 bytes).

### Demo file

**Identical to 1.0.** The header is 9 bytes, with no version:

| Offset | Size | Field | 0.99 address |
|---|---|---|---|
| 0 | 4 | `ud.reccnt` (records, all players) | `0xf0cd8` |
| 4 | 1 | `ud.volume_number` | `0xf0d3c` |
| 5 | 1 | `ud.level_number` | `0xf0d38` |
| 6 | 1 | `ud.player_skill` | `0xf0d34` |
| 7 | 2 | `ud.multimode` | `0xf0cca` |

- opendemoread calls `newgame(volume, level, skill)` straight after the header.
- opendemowrite writes a zero `reccnt` placeholder.
- record buffers up to 2520 records (`cmp 0x9d7`) and flushes them with
  `dfwrite(buf, multimode * 10, reccnt / multimode)`. The total goes in its own counter,
  `0xb0500`.
- closedemowrite flushes the rest, seeks to 0 and writes `0xb0500` as the record count.
- playback reads `min(reccnt, 2520) / multimode` groups per `kdfread` call and runs one
  domovethings per 4 `totalclock` ticks.

All of this is as in 1.0. The body is the same `kdfread` block chain, and the delta coding
restarts for every 2520-record call.

### Input record

**Identical to 1.0: 10 bytes, with no `horz`.** processinput reads `sync[]` at `0xe0f28`:

| Offset | Field | Evidence |
|---|---|---|
| 0 | `fvel` s16 | `posxv += fvel * …` (processinput, `ps+0x240`) |
| 2 | `svel` s16 | `posyv += svel * …`; tested with `fvel` for "not moving" |
| 4 | `avel` s16 | added to `ps.ang` (`ps+0x270`) |
| 6 | `bits` u32 | read as a long at `sync+6` |

getpackets packet 0 carries 4 flag bits per player (`fvel`, `svel`, `avel` as 2 bytes
each, `bits` as 4), as in 1.0. In the recorded demo, `fvel` stays within 0 to 1277,
`svel` within −79 to 905 and `avel` within −3 to 0, and `bits` shows only bit 2 (fire).

### Telling the demo headers apart (Review Focus 3)

- **0.99 from 1.0, or from 1.1: not possible.** The headers are byte-for-byte the same
  layout with no version. The 0.99 and 1.0 records are also the same layout, and 1.1
  differs only in the body. A 1.0 demo given to the 0.99 build plays and desyncs rather
  than being refused, as it would in DOS. The ledger records this as an accepted
  limitation.
- **0.99 from 1.3D, 1.4 or 1.5: yes,** exactly as for 1.0 (`docs/v10-v11/reference.md`,
  "Telling the demo headers apart"). A 1.3D header read as 0.99 gives a `multimode` of at
  least 256. A 1.4/1.5 header has its version byte where the volume goes. A 0.99 header
  read as 1.3D gives a `multimode` that is a multiple of 256. So the 1.0/1.1 build's
  existing checks in opendemoread (volume 0, level up to 10, skill up to 4,
  `1 <= multimode <= 8`) refuse both directions.

### Does a demo continue across a level change? No: one demo per level

- **Playback stops at the level end.** After each frame, playback tests `gm & MODE_EOL`
  (`0x2a8ab`) and returns 1 (`0x2a8b2` → `0x2a9b5`). The main loop then goes back to the
  title and the next demo.
- **Recording cannot cleanly span or reach a level end.**
  - The main loop's MODE_EOL path does not call closedemowrite. It calls vscrn
    (`0x431d0`), dobonus (`0x2c010`), then enterlevel (called at `0x2a179`). It ends the episode
    after E1L6 when `from_bonus == 0`.
  - closedemowrite is only called on MODE_END (`0x2a250`) and from gameexit (`0x2097c`).
  - enterlevel calls opendemowrite (`0x45956`) whenever `recstat == 1` and the entry is
    not a MODE_RESTART. That reopens `demo1.dmo` for writing, which truncates it, and
    leaks the previous handle.
  - opendemowrite resets the header total (`0xb0500`) but not the buffered count
    `ud.reccnt`. The new file's first block therefore also carries the old level's
    unflushed records, while its header count leaves them out.
  - So a recording that reaches an exit loses the level it just finished, and the file
    that's left is malformed. 1.0 has the same structure (enterlevel `0x49e90` opens, and
    there is no close at MODE_EOL).
- **A death restart stays in the same demo.** enterlevel skips opendemowrite for
  MODE_RESTART, and playback handles `gm & MODE_RESTART` by calling `enterlevel(MODE_DEMO)`
  and playing on. Unlike 1.0, the beta's processinput does not close the demo when the
  single player dies (1.0 `0x441e0` calls closedemowrite there).

So there are six playthrough demos, one per map, each started with `/l<n> /r`. A demo
that has to reach a map's exit can't be produced by the unmodified beta: the session would
have to be ended before the exit, or the scratch recording EXE patched to close the demo
at MODE_EOL. That choice is left to the recording task.

### Save file

**Every field is its own `dfwrite` LZW block, as in 1.0.** In write order (saveplayer
`0x35560`, loadplayer `0x34fb0` reads the same order):

| # | Field | 0.99 | 1.0 |
|---|---|---|---|
| 1 | `BYTEVERSION` | **absent** | absent |
| 2 | save name (19), slot stride `0x16` | yes (first field) | yes |
| 3 | `ud.volume_number`, `level_number`, `player_skill` (4 each) | yes | yes |
| 4 | thumbnail (160 × 100) | yes | yes |
| 5 | `numwalls` (2), `wall` (32 each) | `numwalls` walls | same |
| 6 | `numsectors` (2), `sector` (40 each) | `numsectors` sectors | same |
| 7 | `sprite[4096]` (44 each) | yes | yes |
| 8 | `headspritesect[1025]`, `prevspritesect`, `nextspritesect`, `headspritestat[1025]`, `prevspritestat`, `nextspritestat` (2 each) | yes | yes |
| 9 | `numcyclers` (2), `cyclers[256]` (12 each) | yes | yes |
| 10 | `ps[]` | **8 × `0x33f`** | 8 × `0x34e` |
| 11 | `po[]` (16 each) | 8 | 8 |
| 12 | `hittype[2048]` (71 each), raw | yes | yes |
| 13 | `numanimwalls` (2), `animwall` (3072 bytes) | yes | yes |
| 14 | `msx[2048]`, `msy[2048]` (4 each) | yes | yes |
| 15 | `spriteqloc` (2), `spriteq[64]` (2 each) | yes | yes |
| 16 | `mirrorcnt` (2), `mirrorwall[64]`, `mirrorsector[64]` (2 each) | yes | yes |
| 17 | `show2dsector[128]` | **absent** | yes |
| 18 | script bitmap (10240), `script[10240]` (4 each) | yes | yes |
| 19 | `actorscrptr[4096]` (4 each) | yes | yes |
| 20 | `lockclock` (4), `pskybits` (2), `pskyoff[256]` (2 each) | yes | yes |
| 21 | `animatecnt` (4), `animatesect[64]` (2), `animateptr[64]` (4), `animategoal[64]` (4), `animatevel[64]` (4) | yes | yes |
| 22 | trailing scalars | **3** (below) | 4 |

Trailing scalars: `earthquaketime` (1, `0x10aef1`, which domovethings counts down),
`ud.from_bonus` (2, `0xf0cc4`, which the main loop's end-of-episode test reads) and
`ud.respawn_monsters` (`0xf0d08`, which enterlevel copies from `m_respawn_monsters`
`0xf0d18`, the `/t` switch). There is no `respawn_items`.

`respawn_monsters` has a beta bug:
- **saveplayer writes it as 2 bytes** (`0x35a62`), but **loadplayer reads 4**
  (`0x354a6`). Each field is its own block, so the read takes the 2 decoded bytes plus 2
  stale bytes left in `kdfread`'s output buffer by an earlier block. The last block of 4
  or more bytes before it is `animatevel[]`'s, so the stale bytes are bytes 2–3 of
  `animatevel[0]` as stored.
- loadplayer then copies the value into `m_respawn_monsters` (`0xf0d18`).

A loader that matches the EXE must reproduce this, or the ruling must say why it doesn't.

After reading, loadplayer sets `gm = MODE_GAME`, zeroes `lockclock`, `totalclock`,
`ud.recstat` and the FIFO counters, and closes the file. loadpheader (`0x34ec0`) reads
the name, the three `ud` longs and the thumbnail, with no version.

A 0.99 save is told apart from a 1.0 save by layout:
- **`ps[]`:** the block decodes to 6648 bytes, against 1.0's 6768.
- **`show2dsector`:** there is no `show2dsector` block.
- **Trailing scalars:** there are three, against 1.0's four.

`sg_check10`'s walk of the 1.0 block sequence fails on the `ps[]` block of a 0.99 save,
and a 0.99 walk fails on a 1.0 save in the same place. That covers Review Focus 4.

### Script pointers in saves

**The same mixed scheme as 1.0.** The base is `script`'s run-time address. The code names
it as the immediate `0xe1018`, which DOS/4GW relocates.

- `script[]`: a bitmap marks every entry with `v >= base` (signed, no upper bound;
  `cmp …,0xe1018 / jl`). Those entries are stored as `v - base`, and loadplayer adds the
  base back.
- `actorscrptr[]`: **every** entry, zero included, is stored as `v - base`, so empty slots
  hold `-base`. loadplayer adds the base back to every entry.
- `hittype[]`: written **raw**, with no bitmap and no conversion. Any `temp_data` that
  holds a script pointer is a raw DOS address.
- `animateptr[]`: the first `animatecnt` entries are stored as offsets from `sector`
  (`0x20515c`).

A 0.99 loader takes the base from the save itself (the empty `actorscrptr[]` entries) and
translates the `hittype` pointers, as for 1.0. This hasn't been checked on a real save yet.

### Network packets

These are recorded only as far as getpackets (`0x1cac0`) shows them. Network play is out
of scope.
- Packet 0 has 4 flag bits per player, and writes into the input array `0xe0ed8`.
- Packet 6 (name) has no version byte; the name starts at byte 1.
- Packet 5 (new game) carries level, volume, skill, `monsters_off`, `respawn_monsters`
  and `coop`. The `warp` cheat builds it at `0x108584`.

### Cheats

The cheat table at `0xa57de` has **15 entries of 9 bytes**: `godly`, `ungodly`,
`warp##`, `ammo`, `view`, `weapons`, `unlock`, `cashman`, `items`, `rate`, `skill#`,
`clip`, `hyper`, `monsters`, `ending`. The 1.0/1.1 names (`cornholio`, `stuff`,
`scotty`, `kroz`, `cosmo`, `allen`) are not in the EXE. Entry starts with the D and N keys
(`0x21a8c0`, `0x21a8d1`). Only lowercase letters and the digits 0–8 can be typed. cheats
is `0x27820`.

| # | Code | Effect |
|---|---|---|
| 0 | godly | `god = 1` (`0xf0988`), quote 17 |
| 1 | ungodly | `god = 0`, quote 18 |
| 2 | warp## | volume and level from the two digits; MODE_RESTART (in multiplayer, sends packet 5) |
| 3 | ammo | addammo for weapons 1–9 to their maximum, quote 20 |
| 4 | view | toggles `ps+0x2c0` (third-person view), quote 22 |
| 5 | weapons | `gotweapon[1..9] = 1` (`ps+0x2fe`…), quote 21 |
| 6 | unlock | for every sector with lotag above 2, clears the lock bit (`0x4000`) and operates it; then operateforcefields; quote 4 |
| 7 | cashman | toggles `0xf098a` (read by checksectors) |
| 8 | items | the inventory set, by value as in 1.0's `items` (steroids 400, heat 1200, boots 200, shield 100, scuba 6400, holoduke 1600, jetpack 1600, first aid at max health, `got_access = 7`), quote 5 |
| 9 | rate | toggles `0xf0cf4` (read only by `0x20b70`, a display function) |
| 10 | skill# | skill from the digit; MODE_RESTART |
| 11 | clip | toggles `0xf098c` (read by processinput), quote 44/45 |
| 12 | hyper | steroids 399, heat 1200, quote 37 |
| 13 | monsters | cycles `0xb054e` 1–3; nothing reads it |
| 14 | ending | `level_number = 5`, MODE_EOL |

**Cheats can't be entered while a demo records or plays.** Entry needs
`numplayers < 2` and `ud.recstat == 0`. Recording has `recstat` 1 and playback has 2.

The cheat state is plain session globals (`god`, `cashman`, `clip`), and nothing in the
demo header carries it:
- checkcommandline (`0x29390`) zeroes `god` and `cashman` at startup.
- newgame and enterlevel never touch them, so a cheat stays on for the rest of the
  session.

So a `/l<n> /r` recording always starts with every cheat off. A god-mode playthrough would
need a patched scratch EXE (setting `0xf0988` at the start), and the same patch would be
needed for its playback.

## DOS sanity check (Task 2 step 5)

The beta's own `/r` recording plays back the same way in the beta. This was done in
scratch copies; nothing ran inside `~/Downloads/d3d_beta`.

1. **Two patched scratch EXEs, made with a scratch 0.99 copy of `mkcapture.py`.**
   - The recording EXE diverts gameexit's call to closedemowrite (`0x2097c`) to the cave.
     The cave does `pushad`, writes STATE.BIN, does `popad`, then jumps to closedemowrite,
     so the demo is still closed normally.
   - The playback EXE uses mkcapture's demo-end patch, with the `[esp+8]` pattern and
     `which_demo = 1`.
2. **Run setup.**
   - Each scratch directory holds the beta's GRP, CONs, `DUKE.RTS`, `DOS4GW.EXE` and its
     own `DUKE3D.CFG` (Sound Blaster at `220`, IRQ 5, DMA 3/7).
   - DOSBox-X 2026.10.01 runs with `cycles=max`, `core=dynamic`, `memsize=32`,
     `nosound=true`, and `sbtype=sb16` at `220`/5/3/7.
   - The beta first shows a "Press any key to continue" banner, before the startup
     messages.
3. **Recording:** `DUKE3D /l1 /r`, with input from DOSBox-X `AUTOTYPE -w 3 -p 0.15`:
   - `enter` for the banner
   - 60 × `up`, 8 × `lctrl`, 10 × `left`, 20 × `up`, 8 × `lctrl`
   - `f10` `y` to quit

   That gave `DEMO1.DMO`, 543 bytes: E1L1, skill 2, `multimode` 1, 541 records, 233 of
   them non-zero. Shots were fired: ammo went from 48 to 40.
4. **Playback:** `DUKE3D` with that demo, `enter` for the banner. STATE.BIN was written
   with `reccnt` 0.
5. **Comparison of the two dumps:**
   - `randomseed`: equal (`0x14cda0ef`).
   - The player sprite: equal (x −21669, y 11638, z −158786, health 100).
   - `ps[0].posx`/`posy`/`posz`/`ang`: equal.
   - All 4096 sprites, ignoring byte 19: equal except five SE 27 sprites (demo cameras,
     378–382). Their `xrepeat`/`yrepeat` (64 → 0) and `ang` differ. moveeffectors only
     aims SE 27 when `ud.recstat == 2` (`0x15630`), that is, in playback.
   - `ps[0]`: differs only at `0x2aa` (`ftq`, the quit prompt's quote), `0x2dc` (`gm`),
     `0x322` and `0x324` (display flags). These are menu and display state from the quit
     prompt.
6. The five Task 1 checksums were rechecked afterwards and all match.

The beta's demo playback is sound, so the spec's test method stands.
