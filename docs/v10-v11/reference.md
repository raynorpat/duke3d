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
