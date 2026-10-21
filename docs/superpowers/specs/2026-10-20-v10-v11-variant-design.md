# Duke Nukem 3D 1.0 and 1.1 Shareware Build Variants — Design

Date: 2026-10-20
Status: Approved (brainstorming)

## Intent

The 1.3D work (`2026-10-19-v13d-variant-design.md`) made this tree build a 1.3D game that
plays the original 1.3D retail and shareware content in sync with the DOS executables.
The user wants the same for the two releases before it: **shareware 1.0** (January 1996)
and **shareware 1.1** (February 1996). They want them built from the same tree, to the
same standard of correctness.

"Correct" is again defined by the original executables. The main test is that each
release's original demos play back **in sync**.

## Goals

The bar is the one 1.3D shareware met *(user's decision)*:

1. `DUKE3D_VERSION` accepts `1.0` and `1.1`. Both are valid only with
   `DUKE3D_SHAREWARE=ON`. No full 1.0 or 1.1 release exists *(user)*.
2. Each version's original `GAME.CON`, `DEFS.CON` and `USER.CON` compile and run
   unmodified.
3. All three original demos of each version play through in sync: `DEMO1.DMO`–`DEMO3.DMO`
   from 1.0, and the different `DEMO1.DMO`–`DEMO3.DMO` from 1.1.
4. Single player plays correctly on each version's data. That means Episode 1, that
   version's menus and screens, and none of the later content hardcoded in C.
5. Saves written by the original DOS executable load in the matching build.
6. Multiplayer between instances of our own 1.0 build, and between instances of our own
   1.1 build, over the existing UDP transport.

## Non-goals

- The 0.99 beta (`~/Downloads/d3d_beta.7z`), for now *(user)*.
- Full (registered) 1.0 or 1.1. Neither exists.
- Network play against the original DOS executables.
- The Build editor of that era.
- A byte-exact executable, or matching the period audio library and setup/config formats.
- Third-party mods beyond what works as a result of the goals above.

## Anti-goals (failure even if the 1.0/1.1 builds work)

- **Any change in behavior for the 1.5, 1.4 or 1.3D builds, other than real bug fixes.**
  This is the 1.3D rule, extended to every version that already exists. Every 1.0/1.1
  difference is guarded. A real bug in shared code found along the way is fixed for all
  versions, as in 1.3D, with a comment line next to the fix and a mention in the commit
  message. All existing tests, including the 1.3D demo, save and network tests, must still
  pass.
- **Any 1.0 or 1.1 data in the repository.** The CONs, GRPs, demos, saves and executables
  stay out of the tree. Tests read them from local paths.
- **A second copy of the source tree.** These are variants selected with `#if` blocks in
  the same files.

## Reference material

All of it is outside the repo.

- **Original installers:** `~/Downloads/duke3D1.0SW.zip` and `~/Downloads/3dduke11SW.zip`.
  Each holds the 3D Realms installer (`INSTALL.EXE`, `DUKE3DS.IFC`,
  `DUKE3DS._1`–`._4`). Concatenating the four volumes in order gives a self-extracting
  ZIP that 7-Zip (`7zz x`) unpacks directly. The 1.1 concatenation has 1,356 bytes of
  trailing data, which can be ignored.
- **Installed copies** *(my call)*: unpack to `~/Downloads/DUKE3D10SW/` and
  `~/Downloads/DUKE3D11SW/`, next to `~/Downloads/DUKE3D13SW/`. Contents (sizes in bytes):

  | | 1.0 | 1.1 |
  |---|---|---|
  | `DUKE3D.EXE` reports | "Unregistered Shareware v1.0" | "Unregistered Shareware v1.1" |
  | `DUKE3D.EXE` | 1,066,391 | 1,076,339 |
  | `DUKE3D.GRP` | 10,429,258 | 10,442,980 |
  | `GAME.CON` lines | 2,481 | 2,520 |
  | Demos | 3 | 3 (different from 1.0's) |

  Both executables are Watcom C/C++32 with DOS/4GW, the same toolchain as 1.3D.
- **How far apart they are** (whitespace-insensitive diff, changed lines):

  | | GAME.CON | DEFS.CON | USER.CON |
  |---|---|---|---|
  | 1.0 vs 1.1 | 79 | 6 | 114 |
  | 1.1 vs 1.3D | ~2,040 | ~280 | ~410 |

  1.1→1.3D is a large step: the 1.3D `GAME.CON` is 3,581 lines. 1.0→1.1 is small.
- **Demo headers** differ from 1.3D again. 1.0 `DEMO1.DMO` starts
  `08 13 00 00 00 00 02 01 00 93 1a fc`. 1.3D shareware `DEMO1.DMO` starts
  `74 08 00 00 00 01 04 00 01 00 00 00`.
- **No source reference exists for these versions.** gamesrc-ver-recreation's `duke3d`
  covers 1.4 and later. Its `build` repo's oldest engine is the 1.3D editor's (around
  19960320–19960419). Its notes mention that a 19960213 `ENGINE.OBJ` was used along the
  way, which is close to 1.1, but that object isn't in the repo. The 1.0 and 1.1
  executables are the primary references. The 1.3D build in this tree, the recreation
  engine revisions and `extras/` are supporting evidence.
- **EDuke32 commit `8ea1a4d8`** (2013, "Add support for shareware 0.99/1.0/1.1"; GPL;
  pointed out by the user):
  <https://voidpoint.io/terminx/eduke32/-/commit/8ea1a4d834b5ed8affc0cfbea9d00a3dcf75b66d>.
  This is the best available map of the CON-level differences:
  - It detects the version from which keywords a script uses.
  - It adds the old keywords `shadeto`, `endoflevel`, `ifplayersl`, `activate` and
    `rotatespritea`.
  - It gates the startup parameters by version. For scripts older than 1.1 these are
    item respawn time, player friction and blast radii. Gravity, freezer ammo and
    shrinker/expander ammo are newer than 1.1.
  - It gates the LIZTROOP spawn case and RPG smoke.

  Two follow-up commits, also pointed out by the user, fix that commit's
  0.99 handling.
  - `8b9f4345`
    (<https://voidpoint.io/terminx/eduke32/-/commit/8b9f43458a568daf2f07d99592416a520bd23ff4>)
    makes `activate` overwrite its own opcode slot with `operateactivators` and a 0
    argument, which fixes a crash after the 0.99 boss.
  - `ed97e5d0`
    (<https://voidpoint.io/terminx/eduke32/-/commit/ed97e5d0e7dbce26a91f8c81bea91541489a79e2>)
    corrects how the 3D Realms par time in `definelevelname` is detected. 0.99 lacks it.
  Both touch only 0.99 paths. They stay listed here for a later 0.99 project *(user)*. Checked against the scripts: neither 1.0 nor 1.1 uses
  `activate` or `rotatespritea`, both give both times in `definelevelname`, both use
  `shadeto` (3 times) and `endoflevel` (once), and only 1.1 uses `ifplayersl` (twice).
  Two of the commit's other claims still need checking against the executable: whether
  `shadeto` really compiles to nothing, and whether 1.0's `endoflevel` behaves exactly
  like `endofgame`.

  EDuke32 aims to run these scripts, not to match the executables tic for tic. Its
  behavior is a starting hypothesis for step 2 below: confirm each item against the
  executable, which stays the ground truth. It doesn't cover demos, saves, the engine or
  non-CON game logic.
- **Existing 1.3D research tooling to reuse:**
  - `~/Projects/ref/` holds the Ghidra project, `le_flatten.py`, the `__watcall`
    prototype, and the `tools/gh` runner, which works in parallel.
  - `tools/v13d/` in the tree holds `mkcapture.py` (demo-end state dumps from a patched
    scratch EXE under DOSBox-X with SB16), `gendemo.py` and `startnear.py`.
  - `tests/v13d_save_decode.py` and the 1.3D LZW decoder.
  - The 1.3D differences ledger in `docs/superpowers/plans/2026-10-19-v13d-variant.md`
    and the comparison reports in `docs/v13d/notes/`.
  - The rulings in `docs/v13d/rulings.md`.

  `mkcapture.py` and the save decoder carry 1.3D-specific addresses and layouts. They
  will need per-version tables.

## Design

### Build option

- `DUKE3D_VERSION` gains `1.0` and `1.1`. The configure step fails if either is used
  without `DUKE3D_SHAREWARE`. The existing message that "only 1.3D had a shareware
  release" is updated. The 1.0 and 1.1 builds are `VOLUMEONE`.
- **Guard nesting** *(my call)*. 1.1 is built as a step back from 1.3D, and 1.0 as a step
  back from 1.1. Code that differs in 1.3D is also 1.3D-style in 1.1 unless 1.1 differs
  further. Code that differs in 1.1 is also 1.1-style in 1.0 unless 1.0 differs further.
  This is the existing revision-date pattern (`cmake/CompileOptions.cmake`) extended
  downward with earlier dates, so guards read as `< <1.3D date>`, `< <1.1 date>`.
- `BYTEVERSION` for each version comes from its executable. It is not assumed to equal
  1.3D shareware's.

### Finding the differences

From the most to the least structured reference:

1. Flatten and import both executables into the Ghidra project, using the 1.3D
   workflow.
2. Compare the 1.1 CONs with the 1.3D CONs, starting from the EDuke32 commit's keyword
   and parameter list. Find every keyword and interpreter behavior the 1.1 scripts use. Then make the 1.1 compiler and interpreter match the 1.1
   executable, including the opcode numbering and any keyword 1.3D added or changed.
   Then do the same for 1.0 against 1.1.
3. Compare the engine functions that affect game state (clipmove, getzrange, hitscan,
   inside, cansee, neartag, sqrt/math tables, and so on) against this tree's 1.3D
   engine. Port the differences.
4. Compare the remaining game modules (game, actors, player, premap, sector, gamedef)
   for 1.3D→1.1, then 1.1→1.0, function by function against the executables. Use the
   1.3D module reports as a map. Read-only comparison agents per module are fine, as in
   1.3D. Their condensed findings go into a differences ledger in the plan.
5. When a demo desyncs, find the first diverging tic and work back from it.
6. Check the weapons and enemies the demos don't reach with generated demos played by
   both the DOS executable and our build, as in 1.3D Task 15.

**How exact:** the same as 1.3D. Steps 1–6 define the work. There is no audit of the
whole executable. Code that no test reaches may still differ slightly. That risk is
accepted.

### Demos

- Each build reads its version's DOS demo header layout, taken from the executable.
- Each build writes demos in its version's DOS format, as the 1.3D build does
  *(my call, following the 1.3D ruling)*. The DOS executable doesn't have to read them.
- The sync test is automatic: the demo-end state captured from the DOS executable (via
  a per-version `mkcapture.py`) is compared against `-demoverify` output. One test per
  demo: `demo_sync_v10_sw_<n>` and `demo_sync_v11_sw_<n>`.

### Saves

- Each build loads DOS saves from its own version. The save layout, header and
  compression scheme come from the executable. 1.0/1.1 may predate the 1996 LZW that
  1.3D used, or store script pointers differently. Confirm this early, because it decides
  how hard goal 5 is.
- Each build also writes saves. Its format follows the 1.3D precedent (the DOS layout of
  its version) where that is straightforward. Otherwise the builder decides, as long as
  its own saves load back.
- **Reference saves are scripted, not played by hand** *(user)*. The builder drives the
  original executables in DOSBox-X with scripted input. The input can be a recorded
  demo played into the game, keyboard injection, or a patched scratch EXE in the style of
  `mkcapture.py`. Each version gets a few states in `refsaves/` under its data dir: level
  start, active enemies, open doors or moving sectors, and some inventory. Their
  expected values go in a tests data file alongside the 1.3D one. The originals in
  `~/Downloads` stay untouched: work happens in scratch copies, and checksums are verified
  afterwards.

### Multiplayer

- This follows from demo sync, as in 1.3D. 1.0, 1.1, 1.3D and 1.5 builds must refuse to
  join each other. The existing `tests/net_v13d.cmake` pattern is reused.
- **If the `BYTEVERSION`s collide** *(my call)*: each build still uses its executable's
  value. If 1.0 and 1.1, or either of them and 1.3D shareware, turn out to share a value,
  a separate release check is added to the network handshake and to DOS save loading.
  That keeps mismatched builds from joining, and keeps a save from loading in the wrong
  release's build. Check this in the first Ghidra pass, because the network test, the
  save-refusal tests and the save codec all depend on it.

### Test data paths and tests

- New CMake cache variables `DUKE3D_V10_SW_DATA_DIR` and `DUKE3D_V11_SW_DATA_DIR`. Tests
  that need data are registered only when the matching variable is set, following the
  current 1.3D pattern.
- Tests registered in the matching build:
  - `con_compile_v10_sw` / `con_compile_v11_sw`: compiles the CONs with no errors or
    warnings, and compares the compiled script against the script stored in a DOS
    reference save, as `con_compile_v13d` does.
  - `game_boot_v10_sw` / `game_boot_v11_sw`.
  - `demo_sync_*`: three per version.
  - `dos_save_*`: one per reference save.
  - A network test per version.
- CI builds 1.0-sw and 1.1-sw on ubuntu, alongside the existing 1.4, 1.3D and 1.3D-sw
  jobs.

### Documentation

- `docs/v13d/reference.md` and the README gain the new versions, data variables and
  install steps, including the installer concatenation.
- Rulings and comparison notes for this work go in a new folder next to `docs/v13d/`.
  The builder picks the name. Decompiled excerpts may be committed there, as in 1.3D. No
  CON content or data may be committed.

## Sequencing

1.1 goes first, all the way to demo sync, then 1.0 *(my call)*. 1.0→1.1 is small enough
that 1.0 is mostly a short ledger once 1.1 is done.

The work happens on a new branch cut from `main`. Recent Polymost work on `main` doesn't
touch game logic, so it shouldn't conflict.

## Left to the builder

- Macro and revision-date names, and how they fit the existing `CompileOptions.cmake`
  scheme.
- How 1.0/1.1 demo and save formats are expressed in the existing per-version demo reader
  and save codec.
- The input-scripting method for the reference saves, and which states to capture.
- Order of porting and commit breakdown.
- Whether the docs folder is shared (`docs/v13d/` renamed or extended) or new.
