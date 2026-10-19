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
