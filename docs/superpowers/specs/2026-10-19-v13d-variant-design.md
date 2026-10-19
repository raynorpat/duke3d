# Duke Nukem 3D 1.3D (and 1.4) Build Variants — Design

Date: 2026-10-19
Status: Approved (brainstorming)

## Intent

This tree is the 1.5 Atomic Edition source. The user wants a compile-time option that
builds a **1.3D** game from the same tree, so the original 1.3D retail and shareware
releases can be played: their CON scripts, demos, saves and multiplayer. 1.3D mods are the
eventual goal. The first target is the original 1.3D content itself.

"Correct" is defined by the original executables. The main test is that the original 1.3D
demos play back **in sync**. That requires 1.3D game logic, engine logic and RNG use to
match the DOS executables exactly, not approximately.

## Goals

1. A CMake option builds a 1.3D variant. Combined with the existing `DUKE3D_SHAREWARE`
   option, it builds 1.3D shareware. The same option also builds a **1.4** (Atomic
   Edition 1.4) variant, which the 1.5→1.4 step below produces anyway. For 1.4, only
   the build and the CON scripts are checked: the user has no 1.4 data to test demos
   against.
2. The original 1.3D `GAME.CON`, `DEFS.CON` and `USER.CON` compile and run unmodified.
   They are identical between the full and shareware releases.
3. All five original 1.3D demos play through in sync: `DEMO1.DMO` and `DEMO2.DMO` from the
   full release, and `DEMO1.DMO`–`DEMO3.DMO` from shareware.
4. Single player plays correctly on 1.3D data: three episodes (one for shareware), 1.3D
   menus and screens, and no Atomic-only content (Episode 4, the Plutopak logo and
   sprites, Atomic-only actors or weapon behavior hardcoded in C).
5. Saves written by the original DOS 1.3D executable (full and shareware) load in the
   matching 1.3D build.
6. Multiplayer between instances of our own 1.3D build, over the existing UDP transport
   in `source/build/mmulti.c`.

## Non-goals

- Network play against the original DOS executable (IPX, modem, `COMMIT.EXE`).
- Third-party 1.3D mods beyond what works as a result of the goals above.
- The 1.3D Build editor.
- A byte-exact reproduction of the 1.3D executable, or of its audio (Apogee Sound System
  1.1) and setup/config file formats.

## Anti-goals (failure even if the 1.3D build works)

- **Any change in behavior for the default 1.5 build, other than real bug fixes.** Every
  1.3D (and 1.4) difference is guarded so that the 1.5 build compiles to the same logic as
  before. The exception *(user's decision)* is a real bug in this port's shared code
  found along the way, for example a timing bug that desyncs demos. It is fixed for all
  versions, with a comment line next to the fix explaining it, and called out in its
  commit message.
- **Any 1.3D data in the repository.** The 1.3D CON scripts, GRPs, demos, saves and
  executables are not GPL. They stay out of the tree, and tests read them from local paths.
- **A second copy of the source tree.** 1.3D is a variant of this tree, selected with
  `#if` blocks in the same files.

## Context and dependencies

The 64-bit cleanup (`2026-10-19-64bit-cleanup-design.md`) splits 64-bit support into three
sub-projects. This work runs alongside them on its own branch. Each 1.3D goal is tested as
soon as the sub-project it depends on lands:

| 1.3D goal | Testable after |
|---|---|
| CON compile (goal 2) | Sub-project 1 |
| Single player (goal 4) | Sub-project 1 (core 64-bit; the game currently can't boot) |
| Demo sync (goal 3), multiplayer (goal 6) | Sub-project 2 (demos and network on 64-bit) |
| DOS saves (goal 5) | Sub-project 3 (loading DOS `.SAV` files) |

**Sequencing** *(my call)*. Sub-project 1 rewrites every `long` and changes how script
addresses are stored, so C changes written before it lands would conflict almost
everywhere. Until it lands, this work covers only research that needs no C changes in the
tree: disassembly, the list of differences, the CON analysis, and recreating the shareware
data and DOS reference saves. The C work then branches from the sub-project 1 branch and
rebases as it moves. The CON compile check is the first test to turn on.

**Interfaces with sub-projects 2 and 3** *(my call)*. Their specs aren't written yet. This
spec asks two things of them: the demo reader and the DOS save loader must allow the
header and struct layout to vary by game version. The 1.3D (and 1.4) layouts are then
plugged in by this project, so there is no second loader.

**Split of responsibility:** this spec covers only what is *different in 1.3D*. Generic
machinery stays in the 64-bit sub-projects: 64-bit-safe demo playback, the network
transport, and reading DOS-sized save structs. Where a 1.3D difference touches code those
sub-projects are rewriting, follow their conventions, such as fixed-width types and
`SCRIPT_OFF`/`SCRIPT_PTR` script offsets.

## Reference material

All of it is outside the repo:

- **Full 1.3D release:** `~/Downloads/DUKE3D/`. `DUKE3D.EXE` reports
  "Duke Nukem 3D Full Version v1.3D". It includes the 26,524,524-byte `DUKE3D.GRP`,
  the CONs, `DEMO1.DMO`, `DEMO2.DMO` and `DUKE.RTS`.
- **Shareware 1.3D release:** from the Internet Archive item `3dduke13SW`
  (`emulator.zip`, already installed). `DUKE3D.EXE` reports
  "Duke Nukem 3D Unregistered Shareware v1.3D". It includes an 11,035,779-byte GRP and
  three demos. The original `3dduke13SW.zip` (MD5 `04e4ca70b8a2d59ed56c451c5c1d5d39`)
  holds only an installer package. *Builder's housekeeping:* keep the installed copy at
  `~/Downloads/DUKE3D13SW/`. It is currently in a session scratch directory.
- **gamesrc-ver-recreation** (GitHub org `gamesrc-ver-recreation`):
  - `duke3d`: source reconstructed to match the **1.4** and 1.5 game EXEs, selected by
    revision macros (`GAMEVER.H`, `REVSEL`). The 1.5→1.4 differences are the first part
    of the 1.5→1.3D path.
  - `build`: Build engine sources matching the **1.3D Build editor's**
    `ENGINE.OBJ`/`CACHE1D.OBJ`. This is the closest available reference for the 1.3D game
    engine, which matters because engine functions (clipping, hitscan, `getzrange`, and so
    on) affect demo sync.
  - The repos have no license file. The game code derives from the GPL 2003 release, and
    the engine code from Ken Silverman's BUILDLIC release. Use them as references and
    port the differences, consistent with this tree's existing licenses.
- **`extras/` in this tree:** older pre-1.5 source files from the 2003 release
  (`BYTEVERSION 28`). These are useful evidence of earlier behavior, though the exact
  version is unknown.
- **The two 1.3D DOS executables:** the ground truth wherever the sources above disagree
  or have nothing. Disassembling them is expected.
- **CON diff:** the 1.3D CONs total about 5,300 lines, and the 1.5 CONs in `testdata/`
  total about 10,700. The 1.3D scripts use a subset of the commands, and some shared
  commands may behave differently in 1.3D.

## Decided (by the user)

- A compile-time option selects 1.3D. It is not a runtime switch.
- The scope is 1.3D retail and shareware content. The sync test is demo playback.
- Multiplayer is in scope only between our own builds.
- DOS 1.3D saves must load.
- 1.3D files stay out of tree and are read from a local path.
- The work runs side by side with the 64-bit sub-projects and is split as described above.

## Design

### Build option

- A new CMake cache variable, `DUKE3D_VERSION`, takes `1.3D`, `1.4` or `1.5` (default
  `1.5`) and adds a compile definition to the game target. This is a single version
  setting rather than one boolean per version, so versions can't be combined by mistake.
  *(My call.)* `DUKE3D_SHAREWARE` stays a separate option. With
  `DUKE3D_VERSION=1.3D`, it builds 1.3D shareware. 1.4 shareware is not a target.
- The 1.4 variant is built from the 1.5→1.4 differences in the recreation `duke3d` repo
  (see "Finding the differences" step 1). Its guards share the 1.3D work. Code that
  differs in 1.4 is also 1.4-style in 1.3D unless 1.3D differs further.
- In `source/duke3d.h`, the 1.3D full build is `VOLUMEALL` **without** `PLUTOPAK`.
  Today, `VOLUMEALL` always implies `PLUTOPAK`. 1.3D shareware is `VOLUMEONE`.
- `BYTEVERSION` takes the 1.3D values, taken from the executables. The full and shareware
  values may differ. The current `VOLUMEONE` value of 27 is not assumed to be the 1.3D
  shareware value.
- Every behavioral difference is a guarded block, `#if`/`#else` on the 1.3D macro, in
  place in the existing file. Gaps that `PLUTOPAK` already covers (Episode 4 and so on)
  reuse that macro.

### Finding the differences

Work from the most to the least structured reference:

1. Port the 1.5→1.4 game differences from the recreation `duke3d` repo.
2. Compare the 1.3D CONs with the 1.5 CONs. Find every compiler keyword and every
   interpreter behavior the 1.3D scripts depend on, and make the 1.3D build's compiler
   and interpreter match 1.3D for each one.
3. Compare this tree's engine (`source/build/engine.c`, `cache1d.c`) with the recreation
   `build` repo's 1.3D-editor revision. Port the game-relevant engine differences.
4. Confirm the remaining 1.4→1.3D game differences against the 1.3D executable,
   using `extras/` as supporting evidence.
5. When the demos desync, find the first diverging tic and work backward from it.
6. Check the weapons and enemies that no demo exercises against the executable
   individually.

**How exact** *(user's decision)*: steps 1–6 define the work. There is no
function-by-function comparison of the whole executable. Differences are ported from the
references, then the demos, the DOS saves and the targeted check in step 6 decide what is
left. Code no test reaches may still differ slightly from 1.3D. That risk is accepted.

The builder keeps a running list of every difference found, with its source, in the
implementation plan. It will be needed later for 1.3D mods.

### Demos

- 1.3D demo headers differ from 1.5. The 1.3D headers carry no version byte. Example first
  bytes: full `DEMO1.DMO` `77 1b 00 00 01 07 01 00`, shareware `DEMO1.DMO`
  `74 08 00 00 00 01 04 00`. The 1.3D build reads the 1.3D header layout.
- Demos recorded by our 1.3D build only need to play back in our own 1.3D build. Their
  format is the builder's choice, and the DOS executable doesn't need to read them
  *(user's decision)*.
- The sync test needs to be automatic and objective, for example by comparing game
  state at fixed tics against reference captures from the DOS executable running in
  DOSBox, or against a checksum log. The method is the builder's choice, but
  "watch it and see" is not enough.

### Saves

- The 1.3D build loads DOS 1.3D save files for the matching edition. Any layout
  differences from 1.5 (struct contents, globals saved, header) come from the executable.
- The 1.3D build **writes** saves in its own native format, the one sub-project 1
  defines. The DOS executable doesn't need to load them, so there's no DOS-layout writer
  and no DOSBox round-trip test *(user's decision)*.
- **Risk:** 1.3D may write raw DOS addresses where 1.5 writes script offsets, for
  example in actor script pointers, `temp_data` move/action/AI slots, or animation
  pointers. If so, loading them means turning a DOS address back into an offset. The
  script base address is either stored in the save or fixed per edition. Confirm this from
  the executable early, because it determines how hard goal 5 is.
- **Reference saves:** none exist yet. Create them by running the original 1.3D
  executables in DOSBox (install `dosbox-staging` with Homebrew). Cover several levels and
  states for both editions: active enemies, open doors and moving sectors, and inventory.

### Multiplayer

- The game logic is what demos already exercise, so multiplayer between our 1.3D builds
  mostly follows from demo sync.
- The 1.3D `BYTEVERSION` keeps a 1.3D build from joining a 1.5 build, and full from
  joining shareware.
- The packet format doesn't need to match the DOS executable.

### Test data paths and tests

- The CMake cache variables `DUKE3D_V13D_DATA_DIR` (full) and `DUKE3D_V13D_SW_DATA_DIR`
  (shareware) point at the local data. Tests that need data **skip** (CTest
  `SKIP_RETURN_CODE`) when the variable for their edition is unset or the folder is
  missing. Nothing fails just because the data isn't there, so CI stays green without it.
- 1.4 builds register a `con_compile` test against the in-repo 1.5 CONs in
  `testdata/`, which 1.4 must also compile. The 1.4 CONs were never released separately,
  as far as is known. CI builds 1.4 as well.
- Tests are registered only in builds with `DUKE3D_VERSION=1.3D`, for the matching edition:
  - `con_compile_v13d`: compiles the 1.3D CONs with no errors or warnings. It follows the
    pattern of the 64-bit spec's `con_compile`, but reads the scripts from the data dir.
  - `demo_sync_v13d_<edition>_<n>`: one per demo. Turns on after sub-project 2.
  - `dos_save_v13d_<edition>_<name>`: loads each reference save and checks a few known
    values. Turns on after sub-project 3.
- The default 1.5 build's existing tests must still pass unchanged.
- A CI job builds the 1.3D full, 1.3D shareware and 1.4 variants, so the guarded code keeps
  compiling.

## Left to the builder

- The C macro name for 1.3D, and how the existing `VOLUMEONE`/`VOLUMEALL`/`PLUTOPAK`
  logic is restructured. The four combinations must stay clear.
- The disassembly tool and workflow.
- The order in which differences are ported, and how to break them into commits.
- The demo sync verification method, within the requirement above.
- The specific DOS saves to create.
- Whether the 1.3D build's binary or output names differ from the 1.5 build's.
- README updates documenting the new option and data variables.
