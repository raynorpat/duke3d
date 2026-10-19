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
| rotatepoint | engine | `0x6a2d8` | `0x69ff8` | decompiled; sw match: exact |
| getceilzofslope | engine | `0x74126` | `0x73e46` | processinput call order; sw match: order |
| getflorzofslope | engine | `0x74220` | `0x73f40` | processinput call order; sw match: order |
| getzsofslope | engine | `0x7431a` | `0x7403a` | decompiled; sw match: exact |
| kopen4load | cache1d | `0x767e5` | `0x76505` | strings / call graph; sw match: order |
| kread | cache1d | `0x768d5` | `0x765f5` | strings / call graph; sw match: order |
| fakedomovethingscorrect | game | `0x20ff0` | `0x212b0` | source order; called from domovethings |
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
