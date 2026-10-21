# 1.0 and 1.1 Shareware Reference Material

This file records the reference material for the 1.0 and 1.1 shareware build variants
(spec `docs/superpowers/specs/2026-10-20-v10-v11-variant-design.md`). None of the
shareware files themselves are in this repository. Only their locations, checksums and
the values measured from them are.

## Data locations

| Edition | Local path | Origin |
|---|---|---|
| 1.0 shareware | `~/Downloads/DUKE3D10SW/` | `~/Downloads/duke3D1.0SW.zip` (MD5 `611d2a4a81c08aae1468915acc6a6000`), installed |
| 1.1 shareware | `~/Downloads/DUKE3D11SW/` | `~/Downloads/3dduke11SW.zip` (MD5 `e65baabb02e47202d26fe3c99bf9adc2`), installed |

Both zips hold the 3D Realms installer, not the game. They are installed by the steps below
and the installers are never run. The 1.3D shareware install in `~/Downloads/DUKE3D13SW/`
is a separate release and is not used here.

## Install steps

Run in a fresh scratch directory for each zip, not inside `~/Downloads`:

1. Unzip the zip. It contains `DUKE3DS._1` to `DUKE3DS._4`, plus `INSTALL.EXE` and
   `DUKE3DS.IFC`.
2. Concatenate the four volumes in order:
   `cat DUKE3DS._1 DUKE3DS._2 DUKE3DS._3 DUKE3DS._4 > all.bin`
3. Extract into the destination folder with `7zz x -o<dest> all.bin`.

The 1.1 concatenation reports 1,356 trailing bytes after the archive. This is expected and
can be ignored. Do not run `INSTALL.EXE` or any EXE from either release.

## Checksums

| Release | File | MD5 |
|---|---|---|
| 1.0 | DUKE3D.EXE | `1b365878a1761938654f175042c813e3` |
| 1.0 | DUKE3D.GRP | `1e57cf6272e8be0e746666700cc0ee96` |
| 1.0 | GAME.CON | `f84bdc1997eeb71b2c15524146ee268f` |
| 1.0 | DEFS.CON | `1d1cdc1f0faee6451a24fe2e0ccbbbba` |
| 1.0 | USER.CON | `b9b2b542064e50a860e47f8f2c8c70ca` |
| 1.0 | DEMO1.DMO | `d46c67c9841aa3292b504d6d477afe91` |
| 1.0 | DEMO2.DMO | `81a14c54220c1688464cdd243052b973` |
| 1.0 | DEMO3.DMO | `4a583870819445052951cd81c5039c65` |
| 1.1 | DUKE3D.EXE | `cc1cbb18837fefeb1ff1d82e7ed956cc` |
| 1.1 | DUKE3D.GRP | `9b0683a74c8bf36bf85631616385bec8` |
| 1.1 | GAME.CON | `0ff00e20eb240a0e6c683ef6b51ed2d5` |
| 1.1 | DEFS.CON | `735391453b78fc0ac6edcb29a9fce60e` |
| 1.1 | USER.CON | `c1824a0ae2a4e75c15fd391295b81c47` |
| 1.1 | DEMO1.DMO | `bc37a9eee9fb6dbedad7be1b41bf73ef` |
| 1.1 | DEMO2.DMO | `af7c381e5fbb08ce8e4067e3badebdfd` |
| 1.1 | DEMO3.DMO | `ece2713abc16b6818384a4af2365078c` |

## EDuke32 reference commits

These commits in the EDuke32 repository (`https://voidpoint.io/terminx/eduke32`) are the
best available map of the shareware CON differences. They are GPL.

| Commit | Scope | Meaning |
|---|---|---|
| `8ea1a4d8` | 0.99, 1.0, 1.1 | "Add support for shareware 0.99/1.0/1.1" (2013). Detects the version from the keywords a script uses, adds old keywords, and gates startup parameters and LIZTROOP and RPG smoke by version. <https://voidpoint.io/terminx/eduke32/-/commit/8ea1a4d834b5ed8affc0cfbea9d00a3dcf75b66d> |
| `8b9f4345` | 0.99 only | Makes `activate` overwrite its own opcode slot with `operateactivators`, which fixes a crash after the 0.99 boss. Kept for a later 0.99 project. <https://voidpoint.io/terminx/eduke32/-/commit/8b9f43458a568daf2f07d99592416a520bd23ff4> |
| `ed97e5d0` | 0.99 only | Corrects detection of the 3D Realms par time in `definelevelname`, which 0.99 lacks. Kept for a later 0.99 project. <https://voidpoint.io/terminx/eduke32/-/commit/ed97e5d0e7dbce26a91f8c81bea91541489a79e2> |

Only `8ea1a4d8` applies to 1.0 and 1.1. The two 0.99-only commits are listed here so they
are not lost, and no 1.0 or 1.1 work depends on them.

## Executable layout

Both EXEs are Watcom C/C++32 programs bound to DOS/4GW, laid out like 1.3D but at
different offsets. The `e_lfanew` at `0x3c` of the outer MZ is garbage (`0x9b40000`).
The real DOS/4G-bound image is an MZ stub at file offset `0x26654`, whose `e_lfanew`
(`0x2aa8`) points to an `LE` header at `0x290fc`. These three values are the same in
1.0 and 1.1. The LE data-page offset is relative to the stub, not to the start of the
file.

| | 1.0 | 1.1 |
|---|---|---|
| MZ stub (file offset) | `0x26654` | `0x26654` |
| LE header (file offset) | `0x290fc` | `0x290fc` |
| Data pages (stub-relative) | `0x39000` | `0x39200` |
| Pages | `0xa5` | `0xa8` |
| Entry point | `0x988b4` | `0x99994` |
| Fixups | type 7: 25695 | type 7: 25729, type 8: 2 |

| Object | Base | 1.0 size | 1.1 size | Contents |
|---|---|---|---|---|
| 1 | `0x10000` | `0x92a9f` | `0x946ff` | code |
| 2 | `0xb0000` | `0x3d` | `0x9d` | data |
| 3 | `0xc0000` | `0x1ebaf0` | `0x1ebf50` | data and BSS (stack at the top) |

All fixups are internal.

## Disassembly method

This is the 1.3D method (`docs/v13d/reference.md`), using the same tools under
`~/Projects/ref/`:

1. `tools/le_flatten.py <exe> 290fc <out.bin> 26654` gives `ghidra/sw10.bin` and
   `ghidra/sw11.bin`. This is the same script as `tools/v13d/le_flatten.py` in this tree.
   Each output is one raw image based at `0x10000`. Their MD5s are
   `197cbd704bc66793521bee3de12bfbe4` (1.0) and `867e11b20fc9ec624e3274dd4f3e31f2` (1.1).
2. Each image is imported into the Ghidra project `ghidra/duke13` as raw
   x86:LE:32 at `0x10000`, with `-preScript SetupLE.java <entry> <code end>`
   (1.0: `988b4 a2a9f`, 1.1: `99994 a46ff`) and `-postScript ApplyWatcall.java`.
   Analysis finds 1290 functions (1.0) and 1291 (1.1). The import logs are
   `ghidra/import_sw10.log` and `ghidra/import_sw11.log`.
3. `tools/fmatch.py` matched each against the labeled 1.3D shareware program
   (`sw13.bin`, 1335 functions). For 1.0, 633 matched exactly, 394 by order and 308 not
   at all. For 1.1, 697 matched exactly, 434 by order and 204 not at all. Most of the
   game code does not match by bytes, so every game row below was identified by
   strings, call graphs and data use, then checked in the decompile. An fmatch result
   is only quoted as supporting evidence.
4. Decompiles are in `ghidra/dc_sw10/` and `ghidra/dc_sw11/` (out of tree). Function
   lists with callees are in `ghidra/funcs_sw10.txt` and `funcs_sw11.txt`.

## Address map

The 1.3D shareware address is given for reference. "Call position" means the function
sits at the same place in a known caller's call sequence as its 1.3D counterpart.

### Functions

| Name | Module | 1.0 | 1.1 | 1.3D sw | How identified |
|---|---|---|---|---|---|
| getpackets | game | `0x106d0` | `0x106d0` | `0x10660` | 1.1: "You cannot play Duke with different versions" string; 1.0 (no string): same packet-type switch on `packbuf[0]`, same 7 callers including playback |
| opendemoread | game | `0x1f1f0` | `0x1f5c0` | `0x20950` | `demo_.dmo` string; reads `ud.reccnt` first |
| opendemowrite | game | `0x1f2b0` | `0x1f680` | `0x20a70` | `demo1.dmo` string |
| record | game | `0x1f360` | `0x1f730` | `0x20bc0` | call position in domovethings; 1.0 fmatch exact |
| playback | game | `0x1f500` | `0x1f8d0` | `0x20d70` | only caller of opendemoread; holds the demo-end `jnz` used by capture patches (1.0 `0x1f69c`, 1.1 `0x1fa6c`) |
| domovethings | game | `0x20ae0` | `0x20ff0` | `0x22430` | only caller of processinput; calls record and krand |
| processinput | game | `0x441e0` | `0x44cf0` | `0x477c0` | "KILLED BY %s" string; fmatch order |
| movedummyplayers | game | `0x24c10` | `0x25250` | `0x26d10` | call position; walks statnum 13 |
| movefta | game | `0x24360` | `0x24970` | `0x26510` | call position; walks statnum 2 |
| moveweapons | game | `0x27890` | `0x27ed0` | `0x29ad0` | call position; walks statnum 4; calls execute and movesprite |
| movetransports | game | `0x285f0` | `0x28c60` | `0x2a830` | call position; walks statnum 9 |
| moveplayers | game | `0x24e30` | `0x25470` | `0x26f00` | call position; walks statnum 10; calls execute |
| movefallers | game | `0x25770` | `0x25d90` | `0x27750` | call position; walks statnum 12 |
| moveexplosions | game | `0x302a0` | `0x30a80` | `0x2e270` | walks statnum 5 only; calls execute; size matches (4241 / 4252 vs 4132) |
| moveactors | game | `0x2db00` | `0x2e1f0` | `0x2b560` | walks statnum 1 first; calls execute and movesprite |
| moveeffectors | game | `0x294a0` | `0x29b30` | `0x2f2a0` | walks statnum 3 first, then 1 and 10, as in 1.3D |
| movestandables | game | `0x25aa0` | `0x260d0` | `0x27a80` | call position; walks statnum 6; calls execute |
| movefx | game | `0x252a0` | `0x258c0` | `0x272c0` | call position; walks statnum 11 |
| movecyclers | game | `0x24af0` | `0x25130` | `0x26c00` | call position; size 282 vs 272 |
| movesprite | game | `0x23c30` | `0x24270` | `0x25b30` | same callees (setsprite, clipmove, krand, changespritesect) and callers (moveweapons, moveactors); fmatch order |
| parse | game | `0x34840` | `0x350a0` | `0x370a0` | recursive, and its only other caller is execute; fmatch order |
| execute | game | `0x377f0` | `0x38110` | `0x3a3b0` | calls parse; called by the five movers that run actor code; fmatch order |
| loadpheader | game | `0x38130` | `0x38600` | `0x3a870` | `game0.sav` string; reads the header only, as in 1.3D |
| loadplayer | game | `0x38220` | `0x38750` | `0x3a9a0` | `game0.sav` string; mirrors saveplayer's reads |
| saveplayer | game | `0x388d0` | `0x38f60` | `0x3baf0` | `game0.sav` string; dfwrite sequence |
| krand | engine | `0x661e7` | `0x67a71` | `0x6bf61` | the only use of the constant `0x1a4e86d` |
| kdfread | engine | `0x7083b` | `0x7213b` | `0x767a6` | fmatch exact (1.0) and order (1.1); loadplayer's reader |
| dfwrite | engine | `0x70c65` | `0x72565` | `0x76bd0` | fmatch exact (1.0) and order (1.1); saveplayer's writer |
| compress | engine | `0x70ee1` | `0x727e1` | `0x76e4c` | fmatch exact (both) |
| uncompress | engine | `0x710bd` | `0x729bd` | `0x77028` | fmatch exact (both) |

In 1.0 and 1.1, domovethings calls the movers in this order: moveexplosions,
moveeffectors, moveactors. 1.3D calls moveexplosions, moveactors, moveeffectors. The
source order in actors.c also differs: moveeffectors, moveactors, moveexplosions in 1.0
and 1.1, against moveactors, moveexplosions, moveeffectors in 1.3D. The rest of
domovethings' call sequence after krand is the same as 1.3D, with 19 calls in each.

### Globals and constants

| Name | 1.0 | 1.1 | 1.3D sw | Notes |
|---|---|---|---|---|
| `keyw[]` (CON keywords) | `0xc6788` | `0xc6c90` | `0xc7830` | pointer table, starting with `definelevelname`; used by parsecommand and its helpers. A pointer scan gives about 93 (1.0) and 94 (1.1) entries. Task 3 dumps the table and settles the count |
| `which_demo` | `0xc6783` | `0xc6c8d` | `0xc782e` | byte, initial value 1; loaded by `mov al,[..]` just before playback calls opendemoread |
| `randomseed` | `0x22aaf0` | `0x22ae04` | `0x251ae8` | operand of krand's `imul` |
| `sprite[]` | `0x1a6d1c` | `0x1a7030` | `0x1cdd10` | 4096 × 44 bytes; setsprite operands (fmatch exact) and saveplayer |
| `ps[]` | `0x112ab0` | `0x1162e0` | `0x11be80` | **8 × `0x34e` bytes** (1.3D: 16 × `0x552`); saveplayer writes `(ps, 0x34e, 8)` |
| `ud.reccnt` | `0x130ebb` | `0x130562` | `0x14df5a` | first field read by opendemoread; compared with 0 in playback's demo-end test |
| `headspritestat[]` | `0x22ab10` | `0x22ae24` | `0x251b08` | from the saveplayer write order; used above to tell the movers apart |
| `script[]` | `0x102f08` | `0x107388` | `0x106450` | 10240 longs (`0x2800`; 1.3D: 20000) |
| `BYTEVERSION` | none | `12` (`0x0c`) | `14` (`0x0e`) | see below |
| Code cave | `0xa2aa0`, `0x560` bytes | `0xa4700`, `0x900` bytes | `0xaa330`, `0xcd0` bytes | zero slack after the end of the code object, to the end of its last file-backed page |
| Log slack | `0x2abaf0` | `0x2abf50` | `0x2d5550` | end of object 3, used for the capture `log` key |

### BYTEVERSION

| Edition | Value | Evidence |
|---|---|---|
| 1.0 | none | getpackets packet 6 (name) copies the name from `packbuf[1]` with no version byte. saveplayer's first write is the name (`0x13` bytes), with no version long. loadplayer and loadpheader read no version and check none. The "You cannot play Duke with different versions" string is not in the EXE. |
| 1.1 | 12 (`0x0c`) | getpackets `0x10dc1`: `mov al,[packbuf+1]` and `cmp eax,0xc` at `0x10dc6`. saveplayer `0x38f73`: `mov edx,0xc`, stored as the first long. loadplayer `0x3879d` and loadpheader `0x3864b`: `cmp dword [esp..],0xc`. |
| 1.3D sw | 14 (`0x0e`) | getpackets `0x11038`, saveplayer `0x3bb03` (for comparison) |

The 1.0 and 1.1 demo headers carry no version either. opendemoread reads `reccnt`,
volume, level, skill and `ud.multimode` (5 reads, against 10 in 1.3D; see "Formats").

The ruling is in `docs/v10-v11/rulings.md`.

### Capture-patch notes

`tools/v13d/mkcapture.py`'s 1.3D patch is 319 bytes with `krandlog`. Its
`cave not empty` assert checks `0x600` bytes. In 1.0 only `0x560` bytes of zero slack
remain before the code object's last page ends. A 1.0 capture tool must check at most
`0x560` bytes, or use another cave. The playback demo-end pattern
(`cmp [reccnt],0 / jg / cmp byte [esp+4],0 / jnz`) matches exactly once in each EXE.

## Formats

Everything here is read from `opendemoread`, `opendemowrite`, `record`, `playback`,
`closedemowrite`, `saveplayer`, `loadplayer`, `loadpheader` and `getpackets` (addresses
in the Address map, plus `closedemowrite` at 1.0 `0x1f470` and 1.1 `0x1f840`). The 1.3D
formats they are compared with are in `docs/v13d/reference.md`, "Formats". All
multi-byte fields are little-endian. The ledger rows are F11-001 to F11-006 and F10-001
to F10-004.

### Compression

`compress` and `uncompress` are the 1996 LZW of E-012 in both releases (E11-014: both
fmatch exactly against 1.3D sw). They are also checked on data: the first `kdfread` block
of each original demo decodes with `tests/v13d_save_decode.py`'s `uncompress` to the
16380 bytes its block header announces (1638 records), and every demo's block chain
decodes to exactly `reccnt` records and ends exactly at the end of the file:

| Demo | Header bytes | `reccnt` | Volume / level | Skill | `multimode` | First block (stored → decoded) | Blocks |
|---|---|---|---|---|---|---|---|
| 1.0 DEMO1 | `08 13 00 00 00 00 02 01 00` | 4872 | 0 / 0 | 2 | 1 | 6803 → 16380 | 4 |
| 1.0 DEMO2 | `bf 11 00 00 00 01 02 01 00` | 4543 | 0 / 1 | 2 | 1 | 5975 → 16380 | 4 |
| 1.0 DEMO3 | `37 0f 00 00 00 02 02 01 00` | 3895 | 0 / 2 | 2 | 1 | 7103 → 16380 | 3 |
| 1.1 DEMO1 | `75 0b 00 00 00 01 02 01 00` | 2933 | 0 / 1 | 2 | 1 | 4243 → 16380 | 3 |
| 1.1 DEMO2 | `6d 06 00 00 00 05 02 01 00` | 1645 | 0 / 5 | 2 | 1 | 5340 → 16380 | 2 |
| 1.1 DEMO3 | `fa 0f 00 00 00 00 02 01 00` | 4090 | 0 / 0 | 2 | 1 | 5837 → 16380 | 3 |

The 1.3D shareware demos store skill 4 at the same place; the 1.0 and 1.1 demos store 2.

### Demo file

The header is 9 bytes in both releases, with no version field (1.3D: 24 bytes):

| Offset | Size | Field | 1.1 | 1.0 |
|---|---|---|---|---|
| 0 | 4 | `ud.reccnt` (records, all players) | `0x130562` | `0x130ebb` |
| 4 | 1 | `ud.volume_number` | `0x1305ce` | `0x130f27` |
| 5 | 1 | `ud.level_number` | `0x1305ca` | `0x130f23` |
| 6 | 1 | `ud.player_skill` | `0x1305c6` | `0x130f1f` |
| 7 | 2 | `ud.multimode` | `0x13053e` | `0x130e97` |

The 1-byte fields are read into the low byte of 4-byte `ud` fields, as in 1.3D.
`ud.multimode` is identified by its use: `record` and `playback` size each record group
as `multimode * 10` and divide `reccnt` by it (1.1 `0x1fabb`–`0x1fae8`). The header has
no `m_coop`, `m_monsters_off` or `m_respawn_*` fields, so a multiplayer demo plays with
whatever those settings are. `opendemoread` calls `newgame(volume, level, skill)` straight
after the header (1.1 `0x49bf0`, 1.0 `0x49180`). `opendemowrite` writes a zero `reccnt`
placeholder, and `closedemowrite` flushes the last block, seeks to 0 and writes the total
record count, as in 1.3D. The two releases' demo code is the same apart from addresses
(normalized decompile diff of `record`, `playback` and `closedemowrite`).

The body is a chain of `kdfread` blocks, the same as 1.3D: `playback` reads
`min(reccnt, 2520) / multimode` groups of `multimode` records per `kdfread` call
(`0x1faa1`–`0x1fae8` in 1.1), and `record` flushes with `dfwrite` once 2520 records are
buffered. Each block is a 2-byte stored length, then an LZW block that decodes to at most
16380 bytes (whole records), delta-coded against the previous record.

`playback` runs one `domovethings` per 4 `totalclock` ticks, as in 1.3D. Its demo
counter (`which_demo`) wraps from 4 back to 1 in both releases (1.1 `0x1f8d0`:
`which_demo == 4`), not from 10 as in 1.3D. That is attract-mode behavior, not format;
it is left to the game rows.

### Input record

Both releases use a 10-byte record, but the fields differ in 1.0. The layouts come from
`processinput`'s reads of `sync[]` (1.1 `0x1072e8`, 1.0 `0x102eb8`) and from the
per-field packet decoding in `getpackets`:

| Offset | 1.1 (= 1.3D) | 1.0 |
|---|---|---|
| 0 | `avel` s8 (`0x46747`) | `fvel` s16 (`0x45fc9`) |
| 1 | `horz` s8 (`0x47276`: `horiz += horz >> 1`) | (high byte of `fvel`) |
| 2 | `fvel` s16 (`0x46b6b`) | `svel` s16 (`0x45fe5`) |
| 4 | `svel` s16 (`0x46b87`) | `avel` s16 (`0x45ba6`, added to `ang` at `0x45bc8`) |
| 6 | `bits` u32 (`0x44d3e`) | `bits` u32 (`0x4422e`) |

1.0 has no `horz` field at all. Its packet 0 carries 4 flag bits per player (fvel, svel,
avel as 2 bytes each, bits), where 1.1 carries 5 (fvel, svel, avel 1 byte, bits, horz).
The decoded 1.0 demos fit the 1.0 layout: `avel` stays within ±108 in all three, and
`fvel`/`svel` reach about ±4000 to ±5900. The 1.1 demos, read with the 1.1 layout, have
`avel` up to ±127 as a byte and `fvel` within ±3621.

### Telling the demo headers apart

- **1.0 from 1.1: not possible from the header.** The two layouts are identical and carry
  no version. Only the body differs (the record layout above), and nothing in it marks
  the release. Each build has to assume its own release, so a 1.0 demo played in the 1.1
  build (or the reverse) desyncs rather than being refused.
- **1.0 or 1.1 from 1.3D: yes, by `multimode`.** A 1.3D header read as 1.0/1.1 gives
  `multimode = m_coop | (low byte of 1.3D multimode << 8)`, which is at least 256 for any
  real demo (the 1.3D sw demos give `0x0100`). A 1.0/1.1 header read as 1.3D gives
  `multimode` = (high byte of `multimode`, 0) | (low byte of the first block length
  << 8), a multiple of 256 and so never 1 to 8 (1.0 DEMO1 gives `0x9300`, which is
  negative as the signed short the 1.3D build reads). So the range check
  `1 <= multimode <= 8` (`MAXPLAYERS` is 8 in both releases) refuses each in the other's
  build. The 1.3D build's existing check (`mm < 1 || mm > MAXPLAYERS`) already refuses
  all six 1.0/1.1 demos.

### Save file

Every field is written with `dfwrite`, so every field is its own E-012 LZW block. In
write order (1.1 `saveplayer` `0x38f60`, 1.0 `0x388d0`; `loadplayer` reads in the same
order):

| # | Field | 1.1 | 1.0 | 1.3D sw |
|---|---|---|---|---|
| 1 | `BYTEVERSION` (4) | 12 (`0x38f73`) | **absent** | 14 |
| 2 | save name (19) | yes | yes (first field, `0x3890c`) | yes |
| 3 | `ud.volume_number`, `level_number`, `player_skill` (4 each) | yes | yes | yes |
| 4 | thumbnail (160 × 100) | yes | yes | yes |
| 5 | `numwalls` (2), `wall` (32 each) | 8192 walls (`0x3904f`) | **`numwalls` walls** (`0x3899a`) | 8192 |
| 6 | `numsectors` (2), `sector` (40 each) | 1024 sectors | **`numsectors` sectors** (`0x389c8`) | 1024 |
| 7 | `sprite[4096]` (44 each) | yes | yes | yes |
| 8 | `headspritesect[1025]`, `prevspritesect`, `nextspritesect`, `headspritestat[1025]`, `prevspritestat`, `nextspritestat` (2 each) | yes | yes | yes |
| 9 | `numcyclers` (2), `cyclers[256]` (12 each) | yes | yes | yes |
| 10 | `ps[]` | **8 × `0x34e`** | **8 × `0x34e`** | 16 × `0x552` |
| 11 | `po[]` (16 each) | **8** | **8** | 16 |
| 12 | `hittype[2048]` (71 each), raw | no | **here, raw, no bitmap** (`0x38ac8`) | no |
| 13 | `numanimwalls` (2), `animwall` (3072 bytes) | yes | yes | yes |
| 14 | `msx[2048]`, `msy[2048]` (4 each) | yes | yes | yes |
| 15 | `spriteqloc` (2), `spriteq[64]` (2 each) | yes | yes | yes |
| 16 | `mirrorcnt` (2), `mirrorwall[64]`, `mirrorsector[64]` (2 each) | yes | yes | yes |
| 17 | `show2dsector[128]` | yes | yes | yes |
| 18 | script bitmap, `script[]` (4 each) | **10240** | **10240** | 20000 |
| 19 | `actorscrptr[4096]` (4 each) | yes | yes | yes |
| 20 | hittype bitmap (2048), `hittype[2048]` (71 each) | yes | **no** (see 12) | yes |
| 21 | `lockclock` (4), `pskybits` (2), `pskyoff[256]` (2 each) | yes | yes | yes |
| 22 | `animatecnt` (4), `animatesect[64]` (2), `animateptr[64]` (4), `animategoal[64]` (4), `animatevel[64]` (4) | yes | yes | yes |
| 23 | trailing scalars | 7 (below) | 4 (below) | 12 |

Trailing scalars, in order:

- **1.1 (7):** `earthquaketime` (1, `0x131580`), `ud.from_bonus` (2, `0x130538`),
  `ud.respawn_monsters` (4, `0x130592`), `ud.respawn_items` (4, `0x130596`), `ud.god`
  (1, `0x1301f0`), `ud.auto_run` (**1**, `0x130532`), `ud.crosshair` (**1**, `0x1301f3`).
  There is no `secretlevel`, `respawn_inventory`, `monsters_off`, `last_level` or `eog`,
  and `auto_run` and `crosshair` are bytes (4 bytes in 1.3D).
- **1.0 (4):** `earthquaketime` (1, `0x130f68`), `ud.from_bonus` (2, `0x130e91`),
  `ud.respawn_monsters` (4, `0x130eeb`), `ud.respawn_items` (4, `0x130eef`).

How the 1.1 names were settled: `from_bonus` is set to `level_number + 1` on the secret
exit in `processinput` (`0x4524a`–`0x4526c`; 1.1 jumps to the fixed level 5, so it has
no `secretlevel`). `auto_run` is toggled by `gamefunc_AutoRun` (8) with quote 85+ in
`0x1d4f0` (`0x1e0ed`–`0x1e128`) and doubles the speed in `getinput` (`0x43f20`).
`crosshair` is toggled by `gamefunc_Toggle_Crosshair` (47) with quote 20+ in the same
function (`0x1e14b`–`0x1e186`).
`respawn_monsters` and `respawn_items` are packet 5 bytes 5 and 6, the bytes 1.3D's
packet 5 puts in the same two fields, and they have the same set of referencing
functions as their 1.3D counterparts (`getpackets`, `parse`, `execute`, `newgame`
setup, plus `moveactors` for `respawn_items`); `loadplayer` copies each into its `m_`
field. `god` is the byte that `0x1e530` clears with its neighbors and that
`moveplayers` reads; it also sits where 1.3D's `god` sits in the list. The 1.0 fields
sit at the same offsets from `ud.reccnt` as the 1.1 ones (−`0x2a`, +`0x30`, +`0x34`),
and 1.0 `earthquaketime` is the byte `domovethings` counts down (`0x20ae0`).

`loadpheader` reads the version (1.1 only, compared with 12), the name, the three `ud`
longs and the thumbnail, as in 1.3D (1.1 `0x38600`, 1.0 `0x38130`). The save name slot
stride is `0x16` in both.

### Script pointers in saves

**1.1: offsets, the 1.3D scheme.** `saveplayer` converts and `loadplayer` converts back:

- `script[]`: an entry `v` with `0x107388 <= v < 0x111388` (signed, `0x39270`/`0x39278`;
  that is `script` to `script + 0xa000`) is stored as `v - 0x107388` and marked in a
  10240-byte bitmap written before it.
- `actorscrptr[]`: every non-zero entry is stored as an offset from `script`
  (`0x39304`–`0x39316`).
- `hittype[]`: for sprites `i < 2048` whose `actorscrptr[sprite[i].picnum]` is non-zero
  (`0x39385`), `temp_data[1]`, `[4]` and `[5]` (hittype `+0x33`, `+0x3f`, `+0x43`) are
  converted when inside `script[]`; bits 0, 1 and 2 of a 2048-byte bitmap mark them.
- `animateptr[]`: offsets from `sector` (`0x21f330`; `0x3959e`).

**1.0: mixed.**

- `script[]`: offsets with a bitmap, but the test is only `v >= 0x102f08` (signed, no
  upper bound: `cmp ..,0x102f08 / jl` at `0x38bc5`). Any word at or above the base is
  converted, which round-trips in DOS. Neither 1.0 CON file contains a 7-digit
  constant, so in practice only pointers are marked.
- `actorscrptr[]`: **every** entry, zero included, is stored as `v - 0x102f08`
  (`0x38c4a`–`0x38c58`), and `loadplayer` adds the base back to every entry. An empty
  slot is stored as `0xffefd0f8`.
- `hittype[]`: written **raw**, as item 12 above, with no bitmap and no conversion. So
  `temp_data[1]`, `[4]` and `[5]` hold **raw DOS addresses** inside 1.0's `script[]`
  (`0x102f08`–`0x10cf07`). 1.0 `execute` (`0x377f0`) uses `temp_data[4]` (`0xda3af`) as a
  pointer into the script (it reads `*(t[4] + 0x10)`), so these are live pointers. A 1.0
  loader must translate a value `v` in `[0x102f08, 0x10cf08)` to `v - 0x102f08` for the
  sprites whose picnum has an actor script, as the 1.1 saver does. Those sprites'
  other `temp_data` slots are left alone.
- `animateptr[]`: offsets from `sector` (`0x21f01c`; `0x38d1a`), as in 1.1.

### Network packets

Network play against the DOS executables is outside the spec, so these are recorded only
as far as `getpackets` shows them. Both releases' packets differ from 1.3D:

- 1.1 handles packet types 0, 1, 4, 5, 6, 7, 8, 17 and 250. 1.3D also handles 16 and
  255.
- Packet 0 (master to slaves) packs per-player flag bits (5 per player in 1.1, 4 in 1.0)
  and writes straight into a per-player input array (1.1 `0x107298`), not into the 1.3D
  move FIFO.
- Packet 5 (new game) has 7 data bytes in 1.1 (level, volume, skill, `monsters_off`,
  `respawn_monsters`, `respawn_items`, `coop`); 1.3D has 9 (adds `respawn_inventory` and
  `marker`). Packet 8 (new game with a user map) carries the same 7 bytes, but its bytes
  5 and 6 go to `respawn_items` and `respawn_monsters` (`0x10d49`–`0x10d6c`), the reverse
  of packet 5 (`0x10c7e`–`0x10c94`).
- Packet 6 (name) has `BYTEVERSION` at byte 1 in 1.1 (`0x10dc6`), names from byte 2. In
  1.0 the name starts at byte 1, with no version.

### player_struct (1.1)

`ps[]` is 8 × `0x34e` bytes at `0x1162e0`, byte-packed like 1.3D's `0x552`-byte struct
(`savegame.c` `sg_player`, `DUKE3D_GAMEVER < GAMEVER_14`). The map was derived by aligning
every fmatch-paired function of `sw11.bin` with `sw13.bin` and voting on the `ps` offset
pairs in matching instructions (processinput, shoot, doincrements, displayweapon,
resetplayerstats, resetweapons, resetinventory, cheatkeys and the rest). The fields that
have no 1.3D partner were then read at every use. 1.0 has the same `0x34e` stride; its map
was not checked. Field names are this tree's. "1.3D" is the 1.3D offset. Ledger rows quote
these names.

Compared with 1.3D, 1.1:
- has no `bobposx`/`bobposy`, `weaprecs[256]`, `weapreccnt` or `subweapon`
- has 10-entry `ammo_amount[]` and `gotweapon[]` (weapon 7 is the flamethrower; no freezer)
- moves `frag`/`fraggedself` after `fist_incs`
- shrinks `frag_ps` and `interface_toggle_flag` to bytes
- adds seven fields

| Offset | Size | Field | 1.3D | Evidence (1.1) |
|---|---|---|---|---|
| `0x000`–`0x22f` | | `zoom` … `invdisptime` | same | same offsets as 1.3D; `posx/posy/posz` `0x214/0x218/0x21c` in every function |
| `0x230` | 4 | `oposx` | `0x238` | processinput `0x45b3b`; xvel bob uses it in place of `bobposx` (`0x45ad5`) |
| `0x234` | 4 | `oposy` | `0x23c` | `0x45b47` |
| `0x238` | 4 | `oposz` | `0x240` | `0x45b53` |
| `0x23c` | 4 | `pyoff` | `0x244` | `0x45b59` |
| `0x240` | 4 | `opyoff` | `0x248` | `0x45b5f` |
| `0x244` | 4 | `posxv` | `0x24c` | |
| `0x248` | 4 | `posyv` | `0x250` | |
| `0x24c` | 4 | `poszv` | `0x254` | |
| `0x250` | 4 | `last_pissed_time` | `0x258` | doincrements `0x447af` |
| `0x254` | 4 | `truefz` | `0x25c` | processinput `0x44e0c` |
| `0x258` | 4 | `truecz` | `0x260` | `0x44e1b` |
| `0x25c` | 4 | `player_par` | `0x264` | doincrements |
| `0x260` | 4 | `visibility` | `0x268` | |
| `0x264` | 4 | `bobcounter` | `0x26c` | |
| `0x268` | 4 | `weapon_sway` | `0x270` | |
| `0x26c` | 4 | `pals_time` | `0x274` | quickkill `0x3f310` |
| `0x270` | 4 | `burn_time` (1.1 only) | — | set to 120 when THROWFLAME hits a player (moveweapons `0x28702`); counted down in processinput (`0x4517a`, +48 `extra_extra8` per tic); zeroed under water (`0x45bdc`) and by resetplayerstats (`0x48368`) |
| `0x274` | 4 | `randomflamex` | `0x278` | only write: `krand()%120+100` when a burn starts (`0x286e7`); never read, like 1.3D's |
| `0x278` | 4 | `crack_time` | `0x27c` | doincrements (`= 777`) |
| `0x27c` | 2 | `ang` | `0x280` | |
| `0x27e` | 2 | `oang` | `0x282` | |
| `0x280` | 2 | `angvel` | `0x284` | `0x46778` |
| `0x282` | 2 | `cursectnum` | `0x286` | |
| `0x284` | 2 | `look_ang` | `0x288` | |
| `0x286` | 2 | `last_extra` | `0x28a` | |
| `0x288` | 20 | `ammo_amount[10]` | `0x28c` (11) | weapon `k` at `0x288+2k` |
| `0x29c` | 2 | `wackedbyactor` | `0x2a2` | |
| `0x29e` | 2 | `curr_weapon` | `0x2a8` | |
| `0x2a0` | 2 | `last_weapon` | `0x2aa` | |
| `0x2a2` | 2 | `tipincs` | `0x2ac` | doincrements; FEM tipping (G11-S15) |
| `0x2a4` | 2 | `horizoff` | `0x2ae` | |
| `0x2a6` | 2 | `holoduke_amount` | `0x2b0` | checkavailinven icon 3 |
| `0x2a8` | 2 | `newowner` | `0x2b2` | |
| `0x2aa` | 2 | `hurt_delay` | `0x2b4` | |
| `0x2ac` | 2 | `hbomb_hold_delay` | `0x2b6` | |
| `0x2ae` | 2 | `jumping_counter` | `0x2b8` | |
| `0x2b0` | 2 | `airleft` | `0x2ba` | |
| `0x2b2` | 2 | `knee_incs` | `0x2bc` | |
| `0x2b4` | 2 | `access_incs` | `0x2be` | |
| `0x2b6` | 2 | `fta` | `0x2c0` | |
| `0x2b8` | 2 | `ftq` | `0x2c2` | |
| `0x2ba` | 2 | `access_wallnum` | `0x2c4` | |
| `0x2bc` | 2 | `access_spritenum` | `0x2c6` | |
| `0x2be` | 2 | `kickback_pic` | `0x2c8` | |
| `0x2c0` | 2 | `got_access` | `0x2ca` | |
| `0x2c2` | 2 | `weapon_ang` | `0x2cc` | |
| `0x2c4` | 2 | `firstaid_amount` | `0x2ce` | checkavailinven icon 1 |
| `0x2c6` | 2 | `somethingonplayer` | `0x2d0` | |
| `0x2c8` | 2 | `on_crane` | `0x2d2` | |
| `0x2ca` | 2 | `i` | `0x2d4` | |
| `0x2cc` | 2 | `one_parallax_sectnum` | `0x2d6` | |
| `0x2ce` | 2 | `over_shoulder_on` | `0x2d8` | |
| `0x2d0` | 2 | `random_club_frame` | `0x2da` | |
| `0x2d2` | 2 | `fist_incs` | `0x2dc` | |
| `0x2d4` | 2 | `frag` | `0x2a4` | `ps[frag_ps].frag++` at death (`0x455b9`) |
| `0x2d6` | 2 | `fraggedself` | `0x2a6` | `0x456eb` |
| `0x2d8` | 2 | `one_eighty_count` | `0x2de` | |
| `0x2da` | 2 | `cheat_phase` | `0x2e0` | |
| `0x2dc` | 2 | `dummyplayersprite` | `0x2e2` | |
| `0x2de` | 2 | `extra_extra8` | `0x2e4` | |
| `0x2e0` | 2 | `quick_kick` | `0x2e6` | doincrements `0x449f5`; cheatkeys sets 24 |
| `0x2e2` | 2 | `heat_amount` | `0x2e8` | checkavailinven icon 5 |
| `0x2e4` | 2 | `actorsqu` | `0x2ea` | |
| `0x2e6` | 2 | `timebeforeexit` | `0x2ec` | |
| `0x2e8` | 2 | `customexitsound` | `0x2ee` | (no `weaprecs`, `weapreccnt` or `interface_toggle_flag` follow) |
| `0x2ea` | 2 | `rotscrnang` | `0x4f4` | |
| `0x2ec` | 2 | `dead_flag` | `0x4f6` | |
| `0x2ee` | 2 | `show_empty_weapon` | `0x4f8` | |
| `0x2f0` | 2 | `scuba_amount` | `0x4fa` | checkavailinven icon 6 |
| `0x2f2` | 2 | `jetpack_amount` | `0x4fc` | icon 4 |
| `0x2f4` | 2 | `steroids_amount` | `0x4fe` | icon 2 |
| `0x2f6` | 2 | `shield_amount` | `0x500` | |
| `0x2f8` | 2 | `spacesuit_amount` (1.1 only) | — | inventory item 5: CON `addinventory 5` (`0x36c80`), `items` cheat 1600 (`0x1d14a`), checkavailinven icon 7 (`0x237ac`), doincrements countdown (`0x449a5`) |
| `0x2fa` | 2 | `holoduke_on` | `0x502` | |
| `0x2fc` | 2 | `pycount` | `0x504` | |
| `0x2fe` | 2 | `weapon_pos` | `0x506` | |
| `0x300` | 2 | `transporter_hold` | `0x50a` | (`frag_ps` moved to `0x32e`) |
| `0x302` | 2 | `last_full_weapon` | `0x50c` | |
| `0x304` | 2 | `footprintshade` | `0x50e` | |
| `0x306` | 2 | `boot_amount` | `0x510` | checkavailinven icon 8 |
| `0x308` | 4 | `scream_voice` | `0x512` | |
| `0x30c` | 1 | `gm` | `0x516` | |
| `0x30d` | 1 | `on_warping_sector` | `0x517` | |
| `0x30e` | 1 | `footprintcount` | `0x518` | |
| `0x30f` | 1 | `hbomb_on` | `0x519` | |
| `0x310` | 1 | `jumping_toggle` | `0x51a` | |
| `0x311` | 1 | `on_sector_effector` (1.1 only) | — | set to 1 when SE0, SE6/14 or SE30 carries the player (moveeffectors `0x2a13e`, `0x2afaa`, `0x2d303`); cleared each processinput tic (`0x4513f`) and by resetplayerstats; read only by displayrooms (`0x165f6`), where it turns off view interpolation |
| `0x312` | 1 | `rapid_fire_hold` | `0x51b` | |
| `0x313` | 1 | `on_ground` | `0x51c` | |
| `0x314` | 13 | `name[13]` | `0x51d` | no reads, as in 1.3D |
| `0x321` | 1 | `inven_icon` | `0x52a` | checkavailinven (`0x23740`) |
| `0x322` | 1 | `buttonpalette` | `0x52b` | secret exit test (`0x45241`) |
| `0x323` | 1 | `jetpack_on` | `0x52c` | |
| `0x324` | 1 | `spritebridge` | `0x52d` | |
| `0x325` | 1 | `lastrandomspot` | `0x52e` | |
| `0x326` | 1 | `no_autoaim` (1.1 only) | — | aim returns -1 for a player shooter when set (`0x3f806`); also hides the crosshair (`0x154e1`). Only ever cleared (resetplayerstats in multiplayer, `0x48580`), so it is always 0 |
| `0x327` | 1 | `spacesuit_on` (1.1 only) | — | set by moveplayers in space (`0x2567b`); cleared when the suit runs out (`0x449c9`) or the player leaves ceiling space (`0x45f9b`); drawn by displaymasks (`0x41f24`) |
| `0x328` | 1 | `scuba_on` | `0x52f` | |
| `0x329` | 1 | `spacesuit_used` (1.1 only) | — | set when the suit runs out (`0x449d5`); then space hurts (`extra_extra8 += 32`, `0x256a0`); cleared by `addinventory 5` (`0x36ca2`) and resetplayerstats |
| `0x32a` | 1 | `footprintpal` | `0x530` | |
| `0x32b` | 1 | `heat_on` | `0x531` | |
| `0x32c` | 1 | `holster_weapon` | `0x532` | |
| `0x32d` | 1 | `falling_counter` | `0x533` | |
| `0x32e` | 1 | `frag_ps` (byte) | `0x508` (short) | death frag credit (`0x45594`); set by ifhitbyweapon, moveplayers, movetransports, CON `resetplayer` (`0x36803`) and resetpspritevars |
| `0x32f` | 1 | `aim_mode` | `0x534` | getinput, cheatkeys bit 23 (`0x507ac`), processinput (`0x44ebe`, `0x4725f`) |
| `0x330` | 10 | `gotweapon[10]` | `0x535` (11) | |
| `0x33a` | 1 | `refresh_inventory` | `0x540` | |
| `0x33b` | 4 | `palette` | `0x541` | setpal (`0x3f1e0`) |
| `0x33f` | 1 | `toggle_key_flag` | `0x545` | |
| `0x340` | 1 | `knuckle_incs` | `0x546` | |
| `0x341` | 1 | `select_dir` (1.1 only) | — | the field commented out of this tree's struct. operatesectors case 15 sets it on a path that is never taken (`0x4c654`, `1.1-sector.md`); processinput's SE17 choice block reads it (`0x453d8`) |
| `0x342` | 1 | `walking_snd_toggle` | `0x547` | |
| `0x343` | 1 | `interface_toggle_flag` (byte) | `0x4f2` (short) | cheatkeys key debounce (`0x507cc`–`0x507ea`) |
| `0x344` | 1 | `palookup` | `0x548` | |
| `0x345` | 1 | `hard_landing` | `0x549` | |
| `0x346` | 1 | `max_secret_rooms` | `0x54a` | |
| `0x347` | 1 | `secret_rooms` | `0x54b` | |
| `0x348` | 3 | `pals[3]` | `0x54c` | |
| `0x34b` | 1 | `max_actors_killed` | `0x54f` | |
| `0x34c` | 1 | `actors_killed` | `0x550` | |
| `0x34d` | 1 | `return_to_center` | `0x551` | |

Weapons (`ammo_amount`, `gotweapon`, `curr_weapon`) are numbered as in the 1.1 `DEFS.CON`:
- 0 KNEE, 1 PISTOL, 2 SHOTGUN, 3 CHAINGUN, 4 RPG, 5 HANDBOMB, 6 SHRINKER
- 7 FLAMETHROWER
- 8 TRIPBOMB, 9 HANDREMOTE

The weapon → projectile table at `0xc6f58` holds KNEE, SHOTSPARK1, SHOTGUN, CHAINGUN, RPG,
HEAVYHBOMB, SHRINKER, FLAMETHROWER (2548), HANDHOLDINGLASER and HANDREMOTE.

Inventory items (`inven_icon`, checkavailinven order):
- 1 first aid, 2 steroids, 3 holoduke, 4 jetpack, 5 heat
- 6 scuba
- 7 space suit
- 8 boots
