# Duke Nukem 3D 0.99 Beta Build Variant — Design

Date: 2026-10-21
Status: Approved (brainstorming)

## Intent

The 1.3D, 1.1 and 1.0 work (`2026-10-19-v13d-variant-design.md`,
`2026-10-20-v10-v11-variant-design.md`) made this tree build each of those releases and play
their original content the way the DOS executables do. The user wants the same for the
**0.99 pre-release beta** (January 1996), the build that came a few weeks before shareware 1.0.

"Correct" is again defined by the original executable. The beta ships no demos, so the main
test is different from 1.0/1.1. **Success is an Episode 1 playthrough on the beta's own,
unmodified CON files that plays the same as it does in the DOS beta EXE** *(user)*. That is
checked with demos recorded from the DOS beta, which our build must play back in sync.

## What the beta is

`~/Downloads/d3d_beta/` (unpacked from `~/Downloads/d3d_beta.7z`). A note to the publisher
(`README.TXT`, Jan 5 1996) says: "This version is really stable for single player. We are
working the next week on net/modem and then we should be ready to release the game."

| | |
|---|---|
| `DUKE3D.EXE` reports | `Duke Nukem 3D Pre-Release "Beta" version .99` |
| `DUKE3D.EXE` | 851,779 bytes, Jan 4 1996, Watcom C/C++32, DOS/4GW (LE at offset 10832) |
| `DUKE3D.GRP` | 9,690,241 bytes, 166 entries: six maps `E1L1`–`E1L6`, the CONs, 7 MIDIs, no demos |
| `GAME.CON` / `DEFS.CON` / `USER.CON` | 2,422 / 780 / 572 lines, byte-identical to the copies inside the GRP |
| Changed lines vs 1.0 (whitespace-insensitive) | `GAME.CON` 423, `DEFS.CON` 49, `USER.CON` 383 |

Found in the EXE: the shareware order screens, `/r` ("Record a demo"), `demo1.dmo`,
`game0.sav` and the save/load menus. So the beta can record demos and has saves.

Old CON keywords the beta scripts use: `activate` once, `shadeto` twice, and
`definelevelname` with a single time (`definelevelname 0 0 E1L1.map 02:00 HOLLYWOOD
HOLOCAUST`), with no 3D Realms par time. `endoflevel`, `ifplayersl` and `rotatespritea` aren't
used.

The 0.99→1.0 CON step (about 850 changed lines) is larger than 1.0→1.1. Expect a longer
ledger than 1.0's.

## Goals

1. `DUKE3D_VERSION` accepts `0.99`, valid only with `DUKE3D_SHAREWARE=ON`, like 1.0 and 1.1.
   It's a `VOLUMEONE` build.
2. The beta's `GAME.CON`, `DEFS.CON` and `USER.CON` compile and run unmodified.
3. **E1 plays as in the DOS beta** *(user)*. All six maps can be played through on the beta
   data, with the beta's menus and screens and none of the later content that is hardcoded in
   C. The check is a set of demos recorded with the DOS beta (see Demos), each played back in
   sync by our build.
4. Saves written by the DOS beta load in our 0.99 build *(user: "saves can stay")*.

**Crashes in the beta are fixed, not copied** *(user)*. If the DOS beta crashes or hangs
somewhere in E1, for example after the boss (EDuke32 `8b9f4345`), our 0.99 build fixes it.
The fix is guarded to 0.99 and gets a ledger entry and a ruling that mark it as a departure
from the EXE. No DOS demo can check past such a point. A demo stops before it, and the stretch
after it is checked by playing it in our build.

## Non-goals

- **Multiplayer** *(user)*. The beta's net/modem code was unfinished. No network test, no
  co-op or deathmatch checks, and no `fakedomovethings` work for 0.99. *My call:* if the
  version handshake is cheap to keep distinct, it should stay distinct so a 0.99 build can't
  join a 1.0+ build. That gets no test of its own.
- Registered content, other episodes, and any 0.99 data other than what's in
  `~/Downloads/d3d_beta/`.
- The Build editor of that era.
- A byte-exact executable, period audio, or the setup/config formats (`SETUP.EXE`,
  `DUKE3D.CFG`).
- Matching the shipped `DUKE3D.CFG` in the beta folder. It's someone's local config.

## Anti-goals (failure even if the 0.99 build works)

- **Any change in behavior for the 1.5, 1.4, 1.3D, 1.1 or 1.0 builds, other than real bug
  fixes.** This is the rule from earlier variants. Every 0.99 difference is guarded. A real
  bug in shared code is fixed for all versions, with a comment line next to the fix and a
  mention in the commit message. All existing tests must still pass: the 1.0/1.1/1.3D demo,
  save and network tests in all six existing build configs.
- **Any beta data in the repository**: CONs, GRP, EXE, recorded demos, saves. Tests read them
  from local paths. Decompiled excerpts and comparison notes may be committed, as before.
- **A second copy of the source tree.** It's `#if` guards in the same files.
- **Editing the CONs to make them work.** Goal 2 says unmodified.

## Reference material

- **The beta EXE** is the ground truth. Flatten and import it into the Ghidra project in
  `~/Projects/ref/ghidra` as `beta099.bin`, next to `sw10.bin`/`sw11.bin`.
- **The 1.0 build in this tree** is the closest working reference. 0.99 is built as a step
  back from it.
- **EDuke32's 0.99 support** (all GPL, pointed out by the user, listed in the 1.0/1.1 spec):
  - `8ea1a4d8`: version detection from keywords, `shadeto`, `activate`, the startup
    parameters by version, the LIZTROOP and RPG smoke gates.
  - `8b9f4345`: `activate` overwrites its own opcode slot with `operateactivators` and a 0
    argument. This fixes a crash after the 0.99 boss.
  - `ed97e5d0`: `definelevelname` par-time detection. 0.99 has no 3D Realms time.

  These are hypotheses to confirm against the EXE. EDuke32 aims to run the scripts, not to
  match the EXE tic for tic.
- **Existing tooling:** `tools/v13d/mkcapture.py` (per-version tables, now covering 1.0/1.1),
  `gendemo.py`, `startnear.py`, `mksave.py`, the save decoders, the `tools/gh` Ghidra runner,
  and the 1.0/1.1 ledger and notes in `docs/superpowers/plans/2026-10-20-v10-v11-variant.md`
  and `docs/v10-v11/`. They need 0.99 tables.

## Design

### Build option

- `DUKE3D_VERSION=0.99`, shareware only. Its configure error and message follow the 1.0/1.1
  ones.
- Guards nest as before: 0.99 is 1.0-style unless it differs further. The revision-date
  scheme in `cmake/CompileOptions.cmake` is extended down with a 0.99 date.
- `BYTEVERSION`: taken from the EXE if it has one, as for 1.0. The first Ghidra pass checks
  whether it collides with another version's. That matters for save refusal: a 0.99 save must
  not load in a 1.0 build, or the other way round. If the values collide, the save
  layout check that 1.0 uses (`sg_check10`) or an equivalent tells them apart.

### Finding the differences

The 1.0/1.1 method, against 1.0 instead of 1.3D:

1. Ghidra import, as above.
2. CON compiler and interpreter: compare the beta CONs with 1.0's. Make the 0.99 compiler and
   interpreter match the beta EXE: opcode numbering, `activate`, `shadeto`, the single-time
   `definelevelname`, the startup parameters, and any keyword 0.99 lacks or does differently.
3. Engine: compare the game-state engine functions (clipmove, getzrange, hitscan, inside,
   cansee, neartag, the math tables) against this tree's 1.0 engine.
4. Game modules (game, actors, player, premap, sector, gamedef), function by function against
   the EXE, using the `docs/v10-v11/notes/1.0-*.md` reports as a map. Read-only comparison
   agents per module are fine. Findings go into a differences ledger in the plan.
5. When a demo desyncs, find the first diverging tic and work back from it.

How exact: the same as before. Steps 1–5 and the demos define the work. Code that no demo
reaches may still differ slightly. That risk is accepted.

### Demos

The beta has no demos, so we make them *(user agreed to recorded demos standing in for
originals)*:

- **Recorded by the DOS beta EXE**, never by our build, under DOSBox-X with an emulated SB16.
  The beta's own `/r` is the recording path. How the input is driven is left to the builder:
  scripted key injection, a replayed input stream, or a patched scratch EXE.
- **Coverage:** at least one demo per E1 map that plays from the start to that map's exit,
  six in all, including the boss. Add shorter scenario demos for weapons, enemies and items
  the playthroughs don't reach, as in 1.0/1.1. The demo after the boss must cover the
  `activate`/`operateactivators` path from `8b9f4345`, up to any crash point (see Goals).
- **Skill and cheats** *(user agreed to the proposal)*: the playthroughs use normal skill with
  no cheats. God mode or other cheats are allowed where a scripted run keeps dying. Scenario
  demos then cover damage, death and the other skills' enemy behavior that the cheat runs
  skip.
- **Reference** *(my call)*: the DOS beta playing back its own recording is the ground truth,
  and the capture hook goes on the playback path. If a recording doesn't play back the same
  way in DOS (for example, it no longer reaches the exit), it's re-recorded or cut short,
  never used as a reference.
- **Storage:** outside the repo, in `~/Downloads/d3d_beta/refdemos/`. The recording scripts
  and their input files go in the repo under `tools/`, so the demos can be regenerated.
- **Check:** the demo-end state captured from the DOS beta (a 0.99 table for
  `mkcapture.py`) is compared against our `-demoverify` output, as before. One ctest per demo:
  `demo_sync_v099_sw_<name>`.
- Our build reads the beta's demo header layout, taken from the EXE, and writes demos in that
  format, as the 1.0/1.1 builds do.

Whether a demo can span a level change in the beta decides whether "one demo per map" means
one recording per map or one long one. Check this before writing the recording script.

### Saves

- The 0.99 build loads saves written by the DOS beta. The layout, header and compression come
  from the EXE. Confirm early whether it matches 1.0's layout, because that decides how much
  of the 1.0 save codec is reusable.
- It writes saves in the beta's DOS layout where that is straightforward. Otherwise it writes
  a layout that loads back in our build.
- **Reference saves are scripted** *(as in 1.0/1.1)*: a few states in
  `~/Downloads/d3d_beta/refsaves/` (level start, active enemies, moving sectors, some
  inventory), with expected values in a tests data file next to the 1.0/1.1 ones. One ctest
  per save: `dos_save_v099_sw_<n>`.

### Test data paths and tests

- A new CMake cache variable, `DUKE3D_V099_SW_DATA_DIR`. Tests that need data are registered
  only when it's set.
- Tests: `con_compile_v099_sw` (no errors or warnings, compiled script matches the one stored
  in a DOS reference save), `game_boot_v099_sw`, the `demo_sync_v099_sw_*` tests and the
  `dos_save_v099_sw_*` tests.
- A `build-v099sw` build directory joins the six existing ones. CI builds 0.99-sw on ubuntu
  next to the 1.0-sw and 1.1-sw jobs.

### Documentation

- README and the reference docs gain 0.99: what it is, `DUKE3D_V099_SW_DATA_DIR`, how to
  unpack it, and how to regenerate the reference demos.
- Rulings and comparison notes go in `docs/v099/`, next to `docs/v10-v11/` *(my call)*.

## Sequencing

One branch cut from `main`. Order *(my call)*: Ghidra import and first-pass facts
(`BYTEVERSION`, demo header, save layout, whether a demo spans levels); then the CON compiler
and interpreter until `con_compile` passes; then the E1L1 recording and the sync work level by
level; then saves; then docs and CI.

## Left to the builder

- Macro and revision-date names.
- The input-scripting method for recording demos and saves.
- The scenario demos beyond the six playthroughs.
- Commit breakdown and porting order within a module.
