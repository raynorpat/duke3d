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
volume, level, skill and a 2-byte field (5 reads, against 10 in 1.3D).

The ruling is in `docs/v10-v11/rulings.md`.

### Capture-patch notes

`tools/v13d/mkcapture.py`'s 1.3D patch is 319 bytes with `krandlog`. Its
`cave not empty` assert checks `0x600` bytes. In 1.0 only `0x560` bytes of zero slack
remain before the code object's last page ends. A 1.0 capture tool must check at most
`0x560` bytes, or use another cave. The playback demo-end pattern
(`cmp [reccnt],0 / jg / cmp byte [esp+4],0 / jnz`) matches exactly once in each EXE.
