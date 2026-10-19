# 1.3D Reference Material

This file records the reference material for the 1.3D and 1.4 build variants (spec
`docs/superpowers/specs/2026-10-19-v13d-variant-design.md`). None of the 1.3D files
themselves are in this repository. Only their locations, checksums and the values
measured from them are.

## Data locations

| Edition | Local path | Origin |
|---|---|---|
| 1.3D full | `~/Downloads/DUKE3D/` | the user's copy of the retail release |
| 1.3D shareware | `~/Downloads/DUKE3D13SW/` | Internet Archive item `3dduke13SW`, `https://archive.org/download/3dduke13SW/emulator.zip` (MD5 `6331dc167d3c88a78816ed1e7cc54042`), unzipped |

The original shareware package `3dduke13SW.zip` (MD5 `04e4ca70b8a2d59ed56c451c5c1d5d39`)
contains only an installer. `emulator.zip` is the same release already installed.

The 1.3D `GAME.CON`, `DEFS.CON` and `USER.CON` are byte-identical between the two
editions.

## Checksums

| Edition | File | MD5 |
|---|---|---|
| full | DUKE3D.EXE | `13012cc50daf22c02dc6c226ebbd0c3f` |
| full | DUKE3D.GRP | `981125cb9237c19aa0237109958d2b50` |
| both | GAME.CON | `aac2c561d79ed37ffc04cfeff907ba42` |
| both | DEFS.CON | `4d7789f1542d69e473a89f0b21ec44e4` |
| both | USER.CON | `159a0931fed4f14cf2e41478f7240bbe` |
| full | DEMO1.DMO | `9cd0e93797f7ea31253c8a78da34faec` |
| full | DEMO2.DMO | `97797a7d7650d9b5ffb8c40582ece191` |
| sw | DUKE3D.EXE | `522be90658f0241fb7ebb8368138de0b` |
| sw | DUKE3D.GRP | `c03558e3a78d1c5356dc69b6134c5b55` |
| sw | DEMO1.DMO | `85f71bc53ef9f61244f59ad849cbce92` |
| sw | DEMO2.DMO | `bb04c6ce57b7777e5ff3dc9b09f971d3` |
| sw | DEMO3.DMO | `3fc4971bab61715b614adfc8d55e44df` |

## Pinned reference sources

These are cloned under `~/Projects/ref/gamesrc-ver-recreation/`:

| Repo | Commit | Contents |
|---|---|---|
| `github.com/gamesrc-ver-recreation/duke3d` | `fdb214ceea671341aa03d54d09fb7fe47940d3b7` | Duke3D 1.4/1.5 game code, guarded on `APPVER_DN3DREV` |
| `github.com/gamesrc-ver-recreation/build` | `88ffca1bdb8d93c731f6d02e62ae5119393bc73b` | Build engine revisions, guarded on `LIBVER_BUILDREV` |

Neither repo has a license file. The game code derives from the GPL 2003 release, and
the engine from Ken Silverman's BUILDLIC release. The variants port their differences
and don't copy files wholesale.

## Executable layout

Both 1.3D EXEs have the same layout. The `e_lfanew` at `0x3c` of the outer MZ is
garbage. The real DOS/4G-bound image is an MZ stub at file offset `0x352a4`, whose
`e_lfanew` (`0x2998`) points to an `LE` header at `0x37c3c`. The LE data-page offset
(`0x3cc00`) is relative to that stub, not to the start of the file.

| Object | Base | Full size | Shareware size | Contents |
|---|---|---|---|---|
| 1 | `0x10000` | `0x9a619` | `0x9a329` | code |
| 2 | `0xb0000` | `0xdd` | `0xdd` | data |
| 3 | `0xc0000` | `0x215400` | `0x215550` | data and BSS (stack at the top) |

Entry point: `0x9ef1c` (full), `0x9ec3c` (shareware). Fixups are all internal: type 7
(32-bit offset) and type 8 (32-bit self-relative).

## Disassembly method

Tools outside the tree, under `~/Projects/ref/`:

1. `tools/le_flatten.py <exe> 37c3c <out.bin> 352a4` loads the three objects at their
   link bases and applies the fixups. The result is one raw image based at `0x10000`.
2. The image is imported into the Ghidra 12.1.2 project `ghidra/duke13` as raw x86:LE:32
   at `0x10000`. `scripts/SetupLE.java` splits the code and data blocks and disassembles
   from the entry point. Analysis finds 1332 functions (full) and 1335 (shareware).
3. `scripts/ApplyWatcall.java` adds a `__watcall` prototype (EAX, EDX, EBX, ECX, then
   stack) as a program spec extension and applies it to every function. Ghidra has no
   Watcom convention, and without it the decompiler drops register arguments.
4. `tools/fmatch.py` matches shareware functions to full ones by byte signature, with
   absolute addresses and rel32 targets masked. It matched 997 exactly and 324 by
   order between exact matches; 11 were unmatched.
5. Game functions were found from string references (`tools/strrefs.py`; Watcom
   leaves filler bytes before aligned strings), the link order in `makefile.lnk`
   (game, actors, gamedef, global, menues, player, premap, sector, sounds, rts, config,
   engine, cache1d, mmulti, a, animlib), and call graphs. Engine functions were anchored
   by `processinput`'s call order against this tree's `processinput`. The gaps between
   anchors were then filled from the source order of the recreation's `ENGINE.C` at
   revision `19960419`, wherever the counts agreed.
6. Scripts `scripts/Decomp.java`, `CallSites.java`, `RefsTo.java` and `DumpFuncs.java`
   are run with `tools/gh <program> <script> args`.

## Address map

### Functions

| Name | Module | Full | Shareware | How identified |
|---|---|---|---|---|
| domovethings | game | `0x22170` | `0x22430` | strings / call graph; sw match: manual: only caller of record |
| record | game | `0x20960` | `0x20bc0` | strings / call graph; sw match: exact |
| playback | game | `0x20b10` | `0x20d70` | source order after closedemowrite; sw manual (sizes differ: 1087 vs 1172) |
| moveloop | game | `0x20f50` | `0x21210` | source order; sw manual |
| fakedomovethings | game | `0x211b0` | `0x21470` | source order; sw manual |
| opendemoread | game | `0x206f0` | `0x20950` | strings / call graph; sw match: exact |
| opendemowrite | game | `0x20810` | `0x20a70` | strings / call graph; sw match: exact |
| closedemowrite | game | `0x20a70` | `0x20cd0` | strings / call graph; sw match: exact |
| getpackets | game | `0x105d0` | `0x10660` | strings / call graph; sw match: order |
| processinput | game | `0x47760` | `0x477c0` | strings / call graph; sw match: order |
| moveplayers | game | `0x26cb0` | `0x26f00` | strings / call graph; sw match: exact |
| movedummyplayers | game | `0x26ac0` | `0x26d10` | strings / call graph; sw match: order |
| movefta | game | `0x262c0` | `0x26510` | strings / call graph; sw match: order |
| moveweapons | game | `0x29880` | `0x29ad0` | strings / call graph; sw match: order |
| movetransports | game | `0x2a5e0` | `0x2a830` | strings / call graph; sw match: order |
| movefallers | game | `0x27500` | `0x27750` | strings / call graph; sw match: order |
| moveexplosions | game | `0x2e020` | `0x2e270` | strings / call graph; sw match: order |
| moveactors | game | `0x2b310` | `0x2b560` | strings / call graph; sw match: order |
| moveeffectors | game | `0x2f050` | `0x2f2a0` | strings / call graph; sw match: order |
| movestandables | game | `0x27830` | `0x27a80` | strings / call graph; sw match: order |
| movefx | game | `0x27070` | `0x272c0` | strings / call graph; sw match: order |
| movecyclers | game | `0x269b0` | `0x26c00` | strings / call graph; sw match: exact |
| movesprite | game | `0x258e0` | `0x25b30` | strings / call graph; sw match: order |
| checksectors | game | `0x53df0` | `0x53b70` | strings / call graph; sw match: order |
| doanimations | game | `0x4e580` | `0x4e300` | strings / call graph; sw match: order |
| animatewalls | game | `0x4e9f0` | `0x4e770` | strings / call graph; sw match: exact |
| newgame | game | `0x4c920` | `0x4c650` | strings / call graph; sw match: order |
| parsecommand | game | `0x34410` | `0x34660` | strings / call graph; sw match: order |
| passone | game | `0x35a00` | `0x35c50` | strings / call graph; sw match: exact |
| loadefs | game | `0x35a30` | `0x35c80` | strings / call graph; sw match: order |
| parse | game | `0x36e50` | `0x370a0` | strings / call graph; sw match: order |
| execute | game | `0x3a160` | `0x3a3b0` | strings / call graph; sw match: order |
| loadpheader | game | `0x3a620` | `0x3a870` | strings / call graph; sw match: order |
| loadplayer | game | `0x3a750` | `0x3a9a0` | strings / call graph; sw match: order |
| saveplayer | game | `0x3b8a0` | `0x3baf0` | strings / call graph; sw match: order |
| krand | engine | `0x6c241` | `0x6bf61` | krand constant 0x1a4e86d; sw match: order |
| getzrange | engine | `0x6c255` | `0x6bf75` | processinput call order; sw match: exact |
| clipmove | engine | `0x68434` | `0x68154` | processinput call order; sw match: order |
| pushmove | engine | `0x69e63` | `0x69b83` | processinput call order; sw match: exact |
| hitscan | engine | `0x669d9` | `0x666f9` | processinput call order; sw match: exact |
| neartag | engine | `0x67b0e` | `0x6782e` | source order between anchors; sw match: exact |
| cansee | engine | `0x6676a` | `0x6648a` | source order between anchors; sw match: exact |
| updatesector | engine | `0x6a220` | `0x69f40` | processinput call order; sw match: exact |
| inside | engine | `0x61009` | `0x60d29` | decompiled; sw match: exact |
| getangle | engine | `0x610c8` | `0x60de8` | decompiled; sw match: exact |
| ksqrt | engine | `0x611b3` | `0x60ed3` | processinput call order; sw match: exact |
| setsprite | engine | `0x66143` | `0x65e63` | processinput call order; sw match: exact |
| insertsprite | engine | `0x66305` | `0x66025` | source order between anchors; sw match: exact |
| deletesprite | engine | `0x66452` | `0x66172` | processinput call order; sw match: exact |
| changespritesect | engine | `0x66614` | `0x66334` | processinput call order; sw match: order |
| nextsectorneighborz | engine | `0x666a2` | `0x663c2` | source order between anchors; sw match: exact |
| lintersect | engine | `0x68131` | `0x67e51` | source order between anchors; sw match: exact |
| dragpoint | engine | `0x68337` | `0x68057` | source order between anchors; sw match: exact |
| kdfread | engine | `0x76a86` | `0x767a6` | only caller pair of uncompress; sw match: offset |
| dfread | engine | `0x76ca3` | `0x769c3` | other caller of uncompress; sw match: offset |
| dfwrite | engine | `0x76eb0` | `0x76bd0` | only caller of compress; sw match: offset |
| compress | engine | `0x7712c` | `0x76e4c` | loads 9 and `0x200` (1996 LZW, E-012); sw match: offset |
| uncompress | engine | `0x77308` | `0x77028` | loads 9 and `0x200` (1996 LZW, E-012); sw match: offset |
| rotatepoint | engine | `0x6a2d8` | `0x69ff8` | decompiled; sw match: exact |
| getceilzofslope | engine | `0x74126` | `0x73e46` | processinput call order; sw match: order |
| getflorzofslope | engine | `0x74220` | `0x73f40` | processinput call order; sw match: order |
| getzsofslope | engine | `0x7431a` | `0x7403a` | decompiled; sw match: exact |
| kopen4load | cache1d | `0x767e5` | `0x76505` | strings / call graph; sw match: order |
| kread | cache1d | `0x768d5` | `0x765f5` | strings / call graph; sw match: order |
| fakedomovethingscorrect | game | `0x20ff0` | `0x212b0` | source order; called from domovethings |
| FindDistance2D | MACT386.LIB | `0x9a3a0` | | shift pattern (`>>5`, `>>7`, `>>2`, `>>6`), decompiled |
| FindDistance3D | MACT386.LIB | `0x9a3fc` | | shift pattern (`>>4`, `>>2`, `>>3`), decompiled |
| timerhandler | game | `0x101d0` | `0x101d0` | `inc [totalclock]; ret`, installed by inittimer |

`getzsofslope` exists in 1.3D, at `0x7431a`.

### Globals and constants

| Name | Full | Shareware | Notes |
|---|---|---|---|
| `keyw[]` (CON keywords) | `0xc76d8` | `0xc7830` | pointer table into obj3 strings |
| `script[]` | `0x106300` | `0x106450` | 20000 longs; save code converts in-range entries to offsets from this base |
| `actorscrptr[]` | `0x100f98` | `0x1010e8` | 4096 entries |
| `hittype[]` | `0xdd798` | `0xdd8e8` | 2048 entries × 71 bytes (packed) |
| `ps[]` | `0x11bd30` | `0x11be80` | 16 × `0x552` bytes |
| `sprite[]` | `0x1cdbc0` | `0x1cdd10` | 4096 × 44 bytes |
| `sector[]` | `0x245ec0` | `0x246010` | 1024 × 40 bytes |
| `wall[]` | `0x1fabc0` | `0x1fad10` | 8192 × 32 bytes |
| `numsectors` | `0x2529f8` | | |
| `sintable[]` | `0x1f9bc0` | | |
| `totalclock` | `0x2519a8` | `0x251af8` | |
| `randomseed` | `0x251998` | `0x251ae8` | `krand`: `seed = seed*0x1a4e86d + 1; return seed >> 16` |
| `ud.reccnt` | `0x14de0a` | `0x14df5a` | other `ud` fields are listed under Formats |
| `BYTEVERSION` | `103` (`0x67`) | `14` (`0x0e`) | compared in getpackets; written by saveplayer and checked by loadplayer/loadpheader |

## Formats

### Save pointers (spec risk): answered

The 1.3D `saveplayer` (full `0x3b8a0`, shareware `0x3baf0`) does **not** write raw DOS
addresses. Before writing, it converts every pointer-valued field to an offset, writes
it, and converts it back afterwards, the same scheme as 1.5:

- `script[]` (20000 longs): every entry that falls inside `script[]` (full
  `0x106300`–`0x119b7f`) gets an offset from the `script` base. A 20000-byte bitmap
  written first marks which entries were converted.
- `actorscrptr[]` (4096): every non-zero entry is converted to an offset from `script`.
- `hittype[]` (2048 × 71 bytes): for sprites whose picnum has an actor script,
  `temp_data` slots 1, 4 and 5 (hittype offsets `+0x33`, `+0x3f`, `+0x43`) are converted
  when they point inside `script[]`. A 2048-byte bitmap is written first (bit 0 = t[1],
  bit 1 = t[4], bit 2 = t[5]).
- `animateptr[]` (64): converted to offsets from `sector[]` (full `0x245ec0`).

So loading a DOS save needs no knowledge of DOS addresses.

### BYTEVERSION

| Edition | Value | Where checked |
|---|---|---|
| full | 103 (`0x67`) | `getpackets` (`packbuf[1]`), first long of the save file (`saveplayer`/`loadplayer`/`loadpheader`) |
| shareware | 14 (`0x0e`) | the same places |

### Demo file

The header is 24 bytes, little-endian, packed. There is no version byte (`opendemoread`
`0x206f0`):

| Offset | Size | Field |
|---|---|---|
| 0 | 4 | `ud.reccnt` (tics) |
| 4 | 1 | `ud.volume_number` |
| 5 | 1 | `ud.level_number` |
| 6 | 1 | `ud.player_skill` |
| 7 | 1 | `ud.m_coop` |
| 8 | 2 | `ud.multimode` |
| 10 | 2 | `ud.m_monsters_off` |
| 12 | 4 | `ud.m_respawn_monsters` |
| 16 | 4 | `ud.m_respawn_items` |
| 20 | 4 | `ud.m_respawn_inventory` |

It is followed by `kdfread`-compressed blocks of up to 2520 `input` records, using the
1996 LZW variant (ledger E-012), not ours. Each record is 10 bytes (`avel` s8, `horz` s8,
`fvel` s16, `svel` s16, `bits` u32), interleaved per player. Then `newgame(volume, level, skill)` runs. The five original demos decode as:

| Demo | Tics | Episode/level (0-based) | Skill | Players |
|---|---|---|---|---|
| full DEMO1 | 7031 | 1 / 7 | 1 | 1 |
| full DEMO2 | 11269 | 2 / 2 | 2 | 1 |
| sw DEMO1 | 2164 | 0 / 1 | 4 | 1 |
| sw DEMO2 | 3325 | 0 / 5 | 4 | 1 |
| sw DEMO3 | 1582 | 0 / 0 | 4 | 1 |

`playback` (`0x20b10`) runs one `domovethings` per 4 `totalclock` ticks. The demo number
wraps from 10 back to 1.

### Save file (`saveplayer`, in write order)

Every field below is written with `dfwrite`, so every field is an LZW block in the 1996
variant (ledger E-012), including the header fields. A full save starts `08 00 04 00 00 00 67 00 00 00`:
block length 8, then the block's 4-byte header (length 4, not compressed) and `BYTEVERSION` 103. A
shareware save has `0e` in place of `67` (checked on the shareware reference saves).

1. `BYTEVERSION` (4)
2. save name (19). There is **no `ud.multimode`** field, unlike 1.5.
3. `ud.volume_number`, `ud.level_number`, `ud.player_skill` (4 each)
4. thumbnail (160 × 100)
5. `numwalls` (2), `wall[8192]` (32 each), `numsectors` (2), `sector[1024]` (40 each),
   `sprite[4096]` (44 each)
6. `headspritesect[1025]`, `prevspritesect[4096]`, `nextspritesect[4096]`,
   `headspritestat[1025]`, `prevspritestat[4096]`, `nextspritestat[4096]` (2 each)
7. `numcyclers` (2), `cyclers[256]` (12 each)
8. `ps[16]` (`0x552` each), `po[16]` (16 each)
9. `numanimwalls` (2), `animwall` (3072 bytes)
10. `msx[2048]`, `msy[2048]` (4 each)
11. `spriteqloc` (2), `spriteq[64]` (2 each). There is **no `spriteqamount`**; the queue
    is a fixed 64.
12. `mirrorcnt` (2), `mirrorwall[64]`, `mirrorsector[64]` (2 each)
13. `show2dsector[128]` (1 each). There is **no `actortype[]`, `boardfilename` or clouds**.
14. script bitmap (20000), `script[20000]` (4 each, offsets)
15. `actorscrptr[4096]` (4 each, offsets)
16. hittype bitmap (2048), `hittype[2048]` (71 each)
17. `lockclock` (4), `pskybits` (2), `pskyoff[256]` (2 each)
18. `animatecnt` (4), `animatesect[64]` (2), `animateptr[64]` (4, offsets from
    `sector`), `animategoal[64]` (4), `animatevel[64]` (4)
19. Twelve scalars, in the same order as the first twelve in 1.5 after `animatevel`:
    `earthquaketime` (1), `ud.from_bonus` (2), `ud.secretlevel` (2),
    `ud.respawn_monsters` (4), `ud.respawn_items` (4), `ud.respawn_inventory` (4),
    `ud.god` (1), `ud.auto_run` (4), `ud.crosshair` (4), `ud.monsters_off` (4),
    `ud.last_level` (2), `ud.eog` (1). These names come from matching the 1.5 order. The
    file ends there: there is **no `coop`, `marker`, `ffire`, `camsprite`, `connecthead`,
    `connectpoint2`, `numplayersprites`, `frags`, `randomseed`, `global_random` or
    `parallaxyscale`**.

The shareware save has the same layout, with its own addresses and `BYTEVERSION`.

## Comparison reports

The per-module EXE-vs-source comparison reports behind the ledger's C and G13 rows are
kept out of tree in `~/Projects/ref/notes/1.3d-<module>.md` (con-compiler,
con-interpreter, game, actors, player, premap, sector). They hold the exact decompile
and assembly line references. Each module's 1.3D (full EXE) address range:

| Module | Range |
|---|---|
| game.c | `0x101d0`–`0x248df` |
| actors.c | `0x248e0`–`0x339af` (`getincangle` at `0x339b0`) |
| gamedef.c | `0x339b0`–`0x3a43f` |
| menues.c | `0x3a440`–`0x41c6f` |
| player.c | `0x41d40`–`0x4ad49` |
| premap.c | `0x4ad50`–`0x4debd` |
| sector.c | `0x4dec0`–`0x54bf0` |
| sounds.c | `0x54c30`– |

## Demo reference capture

`tests/data/v13d_demo_refs.cmake` holds the final state of the five original demos.
The values are captured without the debugger. A scratch copy of `DUKE3D.EXE` is patched
so that when demo playback ends, it writes `STATE.BIN` (`reccnt`, `randomseed`,
`sprite[4096]`) and hangs:

1. `python3 -I tools/v13d/le_flatten.py <DUKE3D.EXE> 37c3c <edition>13.bin 352a4` makes
   the flattened image that the patcher searches.
2. `V13D_FLAT=<edition>13.bin python3 -I tools/v13d/mkcapture.py <full|sw> <original DUKE3D.EXE> <scratch dir>/DUKE3D.EXE <demo n> [callsite <hex>]`
   - redirects playback's demo-end branch (`cmp [reccnt],0 / jg / cmp byte [esp+4],0 / jnz END`)
     to a code cave in the zero-filled slack after object 1 (full `0xaa620`, shareware
     `0xaa330`);
   - sets the title loop's first demo (`which_demo`, full `0xc76d6`, shareware `0xc782e`).

   The cave finds `randomseed`'s runtime address from `krand`'s own operand, so it
   works wherever DOS/4G loads the image.
3. Copy that edition's GRP, CONs, `DUKE.RTS` and demos, plus a `DUKE3D.CFG` with a Sound
   Blaster set up (device 0), into the scratch directory. Run it headless:
   `SDL_VIDEODRIVER=dummy dosbox-x -conf <conf> -fastlaunch -nomenu`, with `cycles=max`,
   `[mixer] nosound=true`, `sbtype=sb16`, and an autoexec that runs `DUKE3D.EXE`. Without
   an emulated Sound Blaster the game stalls after "Could not detect FM chip".
4. Wait for `STATE.BIN` (180232 bytes), kill DOSBox-X, and run
   `python3 -I tools/v13d/statebin.py STATE.BIN <id>`. It refuses captures where `reccnt`
   isn't 0, because that would mean playback ran more than one tic in the final frame.

The captures are deterministic: two runs of shareware demo 3 produced byte-identical
`STATE.BIN` files. To bisect a desync, use `callsite <hex>` to divert any `call` (for
example, `domovethings`' call inside the tic loop) to the same cave.

## DOS reference saves (needs the user at the keyboard)

Task 7 step 3 needs saves made by actually playing, so a person does it. Run each
edition's original `DUKE3D.EXE` from a scratch copy in DOSBox-X with a window, using the
same config as above but without `SDL_VIDEODRIVER=dummy` and with `nosound=false` if
wanted. Save into these slots, then copy `GAME<n>.SAV` to `<data dir>/refsaves/`:

| Edition | Slot | Name | State |
|---|---|---|---|
| sw, full | 0 | `e1l1start` | E1L1 right after the level starts, without moving |
| sw, full | 1 | `e1l1fight` | E1L1 with awake enemies nearby and a door or elevator moving |
| sw, full | 2 | `inventory` | any E1 level holding several weapons, ammo types and inventory items |
| sw | 3 | `e1late` | a later E1 level (E1L4 or after) |
| full | 3 | `e2` | any E2 level |
| full | 4 | `e3` | any E3 level |
| full | — | `e1l1played` | extra: E1L1 after shooting, a barrel explosion and a broken wall (changed sector state) |

Full edition, all made (MD5 of the copy in `refsaves/`; status-bar values in
`tests/data/v13d_save_refs.cmake`). The decode of all six matched the status bar on every
field (`inventory` after allowing for a weapon switch the user made after saving), which confirms the F-007 `ps[]` offsets for health (via `ps.i`), `shield_amount`
+0x500, `curr_weapon`, `ammo_amount` and `steroids_amount` +0x4fe:

| Name | Level | MD5 |
|---|---|---|
| `e1l1played` | E1L1 | `1ef574835d951d675d16045f34af5899` |
| `e2` | E2L1 | `a4e4203a280398d8160eb0355d082735` |
| `e3` | E3L1 | `2c0ee5ca19b0a38f279c725406b7d032` |
| `e1l1start` | E1L1 | `dc61d7aa5d4817533565bea36f33a5eb` |
| `e1l1fight` | E1L1 | `b7e697dc5cd3f21612debbfb9de59220` |
| `inventory` | E1L1 | `b50cbf83c2f8ebb77cb27a44d57465b6` |

Shareware (copies in the shareware data dir's `refsaves/`; same `ps[]` layout, which the
status bars confirm):

| Name | Level | MD5 |
|---|---|---|
| `e1l1start` | E1L1 | `93cbfbe5cdca8ed494511e44d6910e27` |
| `inventory` | E1L1 | `0d88ef4374042a0947d275805de7f741` |

The expected values in `tests/data/v13d_save_refs.cmake` must be observed
**independently of the save layout**. Otherwise a layout mistake would be repeated in
both the reference and the loader under test, and the test would still pass. When
making each save, the user writes down from the screen: health, armor, current weapon,
its ammo, and episode/level. Copy the file to `<data dir>/refsaves/<name>.SAV`. Values
parsed from the file with the layout above (player sprite x/y) are only a supplement.
