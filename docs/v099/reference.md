# 0.99 Beta Reference Material

This file records the reference material for the 0.99 pre-release beta build variant
(spec `docs/superpowers/specs/2026-10-21-v099-variant-design.md`). None of the beta files
are in this repository. Only their location, checksums and the values measured from them
are. Later tasks add the "Address map", "Executable layout" and "Formats" sections.

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
