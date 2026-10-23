# Remaining Source Comments Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task with the user's previously selected native execution method. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Explain the remaining complex source paths without changing their behavior.

**Architecture:** Reconcile the landed input work with the isolated SDL split first. Then make seven focused comment-only passes with separate commits, comparing each pass against the combined implementation. Preserve the current source organization and public interfaces.

**Tech Stack:** C, CMake/CTest, SDL2, Build/Polymost, existing version-specific data fixtures.

**Spec:** `docs/superpowers/specs/2026-10-23-source-comment-coverage-design.md`.

## Global Constraints

- After branch reconciliation, source changes must be comments only.
- Preserve signatures, executable tokens, preprocessor directives, arithmetic, data layouts, file formats and platform/version behavior.
- Preserve existing explanations, credits and library notices; do not blanket-annotate vendored `dbopl.c`.
- No new features, source movement, formatting sweep, bug fixes or tests that count comments.
- Work only in `/Users/raynorpat/.codex/worktrees/e4fe/duke3d`; preserve the original checkout and its untracked files.
- Use existing native execution; keep work committed and isolated. Merging back to main is a separate action.
- Run games and tests in build-tree scratch directories, never in the source data directories.

## Review Focus

1. Save references: distinguish serialized byte offsets, DOS addresses and live native pointers; verify codec, hardening and real-save fixtures in Task 1.
2. Animation delta frames: distinguish run/skip/dump encodings, frame numbering and borrowed frame storage; Task 2 requires exact token checks and records the lack of a dedicated ANM runtime test.
3. Input lifetime: distinguish frame polling, tick consumption, queued presses and immediate camera presentation; exercise the landed input tests in Tasks 0 and 3.
4. Renderer arithmetic: state coordinate spaces, reciprocal interpolation and endpoint conventions accurately; exercise existing image checks in Task 4.
5. Shared state: distinguish packet sequence numbers from ring slots, editor selection from map data, and callback buffers from owned audio storage; exercise the owning checks in Tasks 5–7.

## File and responsibility map

| Pass | Files | Main responsibility |
| --- | --- | --- |
| Save compatibility | `source/savegame.c`, `source/savegame.h` | Record codecs, compressed-block layout checks and reference reconstruction |
| Animation | `source/animlib.c`, `source/animlib.h`, `source/_animlib.h` | ANM pages, delta operations, frame and buffer lifetime |
| Input/configuration | `source/inputaim.c/.h`, `source/control.c/.h`, `source/gamepad.c/.h`, `source/keyboard.c`, `source/config.c` | Fractional aiming, device state, input consumption and persisted settings |
| Input boundaries | Selected blocks in `source/player.c`, `source/game.c`, `source/menues.c`, `source/premap.c`, `source/sector.c`, `source/build/sdl_input.c`, `source/build/sdl_main.c` | Explain the input call chain and reset sites where the landed code meets simulation and presentation |
| Renderer | `source/build/polymost_soft.c`; selected shared math in `polymost.c` | Polygon setup, scan conversion, depth and perspective texture stepping |
| Network | `source/build/network.c` | Packet queues, reliability bookkeeping, native UDP transport |
| Editor | `source/astub.c`, `source/build/build.c` | Selection, tag operations, alignment, geometry editing and mode transitions |
| Audio | `source/audiolib/multivoc.c`, `mv_mix.c`, `mvreverb.c`, `pitch.c`, `dsl.c`, `midi.c`, `al_midi.c`, `oplemu.c`, `music.c`, `fx_man.c` | Mixing, sample cursors, voice lifetime, callback buffers and MIDI/OPL timing |

Header edits are limited to missing caller contracts. Do not duplicate a useful
header explanation above every implementation or revisit already documented
menu/weapon/CON routines unless the landed input behavior makes a comment stale.

The existing native build matrix is `build-split-v15`, `build-split-v14`,
`build-split-v13`, `build-split-v13sw`, `build-split-v11sw`,
`build-split-v10sw`, `build-split-v099sw`, `build-split-v15-off`, and
`build-split-v099sw-off`; `build-split-gl` supplies the real GL checks.
For Task 0, regenerate each using `cmake -S . -B <build>` to retain its recorded
fixture/options cache, then run `cmake --build <build> --parallel 4` and
`ctest --test-dir <build> --output-on-failure` for the nine version builds.
Run the four `render_gl` tests separately in `build-split-gl`. Inspect test
inventories before running; if a build/cache is missing, reconstruct its
version/edition/data options from the existing project instructions and recorded
parent evidence, rather than silently testing a default configuration.

## Shared verification for comment passes

Fold the following into each pass; do not create a standalone tooling project.

- [x] Before editing, save the affected source bytes and actual preprocessed owning units under an ignored `build-comment-coverage-evidence/` directory. Record HEAD, compiler commands and relevant test results.
- [x] Compare significant C tokens before/after, preserving string/character literals and preprocessor directives. Do not use a regex that simply strips comment-looking text out of strings.
- [x] Compare actual preprocessed token streams using `compile_commands.json` in `build-split-v15`, `build-split-v11sw`, and `build-split-v099sw-off`. Extend to 1.0/1.3D/1.4 when documenting a branch absent from those three configurations. Include the containing translation unit for included `.c` files.
- [x] Normalize only diagnostic source-line differences. If edited code uses `__LINE__` for functional behavior, retain that value instead of normalizing it away. Expected result: identical executable tokens for every affected unit/configuration.
- [x] Run `git diff --check`, rebuild the owning targets, run that pass's existing checks, and review each comment against all applicable conditional implementations.
- [x] Commit only the pass's intended paths with its short subsystem-prefixed message. Save evidence outside tracked source files.

Existing ignored comparison scripts from previous passes may be adapted if
available; the required checks above must also be reproducible without them.
Never treat build-tree presence alone as evidence that current sources passed.

## Task 0: Reconcile the landed input work

**Files:** The intersection of the input commit and this worktree's changes,
especially the SDL implementation, engine/view interfaces, game input consumers,
and `tests/CMakeLists.txt`. Preserve new `source/inputaim.c/.h` and all input tests.

**Interfaces:** Preserve main's `CONTROL_PollDevices`, `CONTROL_EndInput`,
`CONTROL_ConsumeLook`, `CONTROL_GetView`, queue/acknowledgement hooks and
`INPUT_AddLook`/`INPUT_AddRate`/`INPUT_ConsumeLook`; preserve the split SDL backend.

- [x] Re-read both tips before executing. Recorded tips are worktree `4057429` and main `d84c766`, common ancestor `55cbcaa`. If either has advanced, inspect and record the new changes before proceeding.
- [x] Capture the two parents' test inventories and available baseline results. Merge main into this isolated worktree, leaving the original checkout untouched.
- [x] Resolve `sdl_driver.c`'s modify/delete overlap by carrying its input changes into the appropriate split SDL files and private header. Keep `sdl_driver.c` removed, compile each split source once, and retain `plat_unix`/`plat_win32` names.
- [x] Resolve shared engine/game changes by preserving frame polling, fractional rendering and reset behavior alongside the previous comments. Retain all new tests and their CMake registrations; do not resolve conflicts by selecting either whole parent file.
- [x] Regenerate and build the existing version configurations, including GL-enabled and Polymost-off builds. Run their full existing suites and the four real GL tests. Use the newly discovered test inventory rather than expecting the old count of 98.
- [x] Explicitly run `input_polling`, `input_aim`, `joystick_polling`, `input_render`, `gamepad`, `input_defaults`, SDL smoke tests, and the affected render/demo tests. Require no regressions against the parents; diagnose integration failures before starting comments.
- [x] Recheck the previously observed 1.0/1.1 network connection failures against current baselines; their old names alone do not justify waiving a changed result.
- [x] Commit the reconciliation as `platform: reconcile landed input with split SDL backend`. Capture this verified combined HEAD as the implementation baseline for Tasks 1–7.

## Task 1: Save compatibility

**Files:** `source/savegame.c`, narrowly `source/savegame.h`.

**Interfaces:** Keep `sg_*` record codecs, block validators,
`sg_walloffset` and `sg_animptr` unchanged.

- [x] Read the codecs and `docs/savegame-offsets.md`, including each version's player layouts and refusal rules.
- [x] Add missing summaries for nontrivial codec/validation helpers. Explain signed field widths, native stride versus serialized size, record counts, compressed-call boundaries, and ambiguous/invalid reference rejection.
- [x] Add selective notes around `sg_dfblocks`, `sg_call`, `sg_calls`, `sg_walloffset`, `sg_animptr`, `sg_checksave`, `sg_check10` and `sg_playerblocksize`; preserve existing detailed layout comments.
- [x] Apply the shared token/build checks. Run codec, real-save, hardening and refusal checks in the applicable version builds with `ctest --test-dir <build> --output-on-failure -R '^(savegame_codec|save_hardening.*|dos_save.*|save_refuse.*)$'`. Confirm the inventory contains the expected fixtures.
- [x] Commit as `game: document save compatibility contracts`.

## Task 2: Animation decoding

**Files:** `source/animlib.c`, narrowly `source/animlib.h` and `_animlib.h`.

**Interfaces:** Keep `ANIM_LoadAnim`, `ANIM_DrawFrame`, `ANIM_GetPalette`,
`ANIM_NumFrames` and `ANIM_FreeAnim` unchanged.

- [x] Trace `findpage` → `loadpage` → `renderframe`/`drawframe` and the public frame API.
- [x] Explain short/long run, skip and literal operations in `CPlayRunSkipDump`, signed-byte interpretation, page/record boundaries, frame numbering and sequential delta reconstruction.
- [x] Document who owns the input buffer, decoded image and palette pointers and what invalidates them, after checking the actual allocation/free paths.
- [x] Apply shared token/build checks; run the existing `game_boot`/`game_boot_*` checks. Record that these are startup checks, not dedicated ANM decoder coverage. If a clip is exercised manually, record its asset and observed playback separately; do not claim every compression opcode was exercised.
- [x] Commit as `game: explain ANM decoding and frame lifetime`.

## Task 3: Input and configuration

**Files:** Input/configuration and input-boundary rows in the file map; only the input call chain, reset sites and stale descriptions in the consumer files.

**Interfaces:** Preserve Q16 camera accumulation and existing public `INPUT_*`,
`CONTROL_*` and `GAMEPAD_*` contracts, plus synchronized game input formats.

- [x] Read main's landed input design and plan, then trace the combined tree from SDL polling through command consumption to local view presentation.
- [x] Document raw deltas versus sampled rates, microsecond integration, fractional remainders, signed cancellation, pitch-angle/horizon conversion and quantization at simulation boundaries.
- [x] Explain held/pressed/suppressed button state, ordered wheel steps, sequence acknowledgement, and focus/menu/level/camera/disconnect resets. Check each reset path before claiming a universal guarantee.
- [x] Add missing function summaries and configuration precedence/default-versus-saved-setting contracts; explain the boundary between local immediate aiming and serialized/demo/network input.
- [x] Apply shared token/build checks. Run `ctest --test-dir build-split-v15 --output-on-failure -R '^(input_polling|input_aim|joystick_polling|input_render|gamepad|input_defaults|player_prediction|sdl_driver_smoke.*)$'`, plus version-specific existing demo checks for edited consumers. Confirm required input tests are present after Task 0.
- [x] Commit as `input: explain polling, aiming and consumption state`.

## Task 4: Software Polymost arithmetic

**Files:** `source/build/polymost_soft.c` and only the shared setup/math it consumes in `polymost.c`.

**Interfaces:** Preserve `polymost_polygon`, `drawline2d`, `drawpoly` and
`polymost_soft_drawpoly`; retain textual inclusion and current renderer selection.

- [x] Trace a polygon from shared setup into software rasterization. Identify coordinate spaces and the meanings/scales of depth, U and V terms from their producers and consumers.
- [x] Comment clipping, edge ordering, reciprocal depth, perspective reconstruction, texture addressing, transparent texels, Z-buffer conditions and scanline endpoints where currently unexplained.
- [x] Explain any original approximation or bit trick without describing it as a more accurate algorithm than it is. Preserve existing Ken Silverman explanations.
- [x] Compare raw tokens in both files and preprocessed owning `engine.c` units. Rebuild and run `ctest --test-dir build-split-v15 --output-on-failure -R '^(engine_trace|vline_counts|render_.*)$'` and `ctest --test-dir build-split-gl --output-on-failure -R '^render_gl($|_)'`. Require unchanged checked images and no substituted/skipped GL run.
- [x] Commit as `renderer: explain software Polymost interpolation`.

## Task 5: Networking

**Files:** `source/build/network.c`.

**Interfaces:** Keep packet API signatures, wire layout, protocol negotiation,
timeouts, queue capacity and native UDP behavior unchanged.

- [x] Trace `sendpacket` → `dosendpackets` → transport and `getpacket` → acknowledgement/resend handling, plus initialization and teardown.
- [x] Explain sequence counters versus low-byte ring indices, packed acknowledgement/error bits, backup-buffer wraparound, resend/timeout scheduling and packet ownership.
- [x] Add summaries for meaningful transport helpers and note Windows/POSIX socket differences where they affect callers; describe actual guarantees rather than promising universal reliable delivery.
- [x] Apply shared token/build checks. Run `ctest --test-dir build-split-v13 --output-on-failure -R '^net_'` and applicable early-version refusal tests. Compare any connection failures with Task 0's current baseline; do not repair unrelated networking behavior here.
- [x] Commit as `network: explain packet queues and retransmission`.

## Task 6: Editor operations

**Files:** `source/astub.c`, `source/build/build.c`.

**Interfaces:** Preserve editor extension hooks, map data structures,
selection/search globals, keyboard mappings and map I/O.

- [x] Trace `ExtLoadMap`, `ExtSaveMap`, caption/data hooks, `Keys2d`, `Keys3d` and the main editor input dispatch; identify editing operations with indirect or shared state.
- [x] Summarize nontrivial hooks and geometry/selection helpers. Explain highlighted versus searched objects, wall pairing, tag interpretation, sprite alignment and 2D/3D transitions.
- [x] Prioritize `GetWallZPeg`, `AlignWalls`, `AutoAlignWalls` and geometry-changing blocks; state texture/Z units and partner-wall invariants after checking the implementation.
- [x] Apply shared token/build checks and build `buildeditor`. Run `ctest --test-dir build-split-v15 --output-on-failure -R '^(editor_mode|build_cfg|engine_trace|render_classic2d_.*)$'`. These do not cover every interactive editing command; record that limit.
- [x] Commit as `editor: explain selection and geometry operations`.

## Task 7: Audio internals

**Files:** The audio row in the file map, narrowly any adjacent header needed to state an ownership contract.

**Interfaces:** Preserve `MV_*`, `DSL_*`, `MIDI_*`, music/effects wrappers,
SDL callback behavior and existing OPL interfaces.

- [x] Trace voice allocation → sample/block advancement → mixing → completion callback/free; separately trace MIDI scheduling into the OPL wrapper.
- [x] Fill genuinely empty function banners rather than adding duplicate headers. Explain fixed-point sample positions, rate/pitch conversion, lookup/clipping arithmetic, channel interleaving, reverb history and buffer/page transitions.
- [x] Document actual callback/main-thread ownership and shutdown ordering. Do not assert locking or callback guarantees that the implementation does not supply.
- [x] Keep `dbopl.c` and its imported implementation untouched; comment the local wrapper boundary where explanation is needed.
- [x] Apply shared token/build checks and run `ctest --test-dir build-split-v15 --output-on-failure -R '^(opl_music|dsl_shutdown|multivoc_lifecycle)$'`. Record platform/device coverage limits rather than claiming every audio backend was exercised.
- [x] Commit as `audio: explain mixing and callback lifetime`.

## Completion and handoff

- [x] Review the complete diff from Task 0's combined baseline: every later source edit must be a comment, and every claimed contract must match all relevant branches.
- [x] Use the requesting-code-review skill for a fresh factual review of the completed comment passes; resolve inaccurate wording before handoff.
- [x] Run the final combined token/preprocessor comparison and `git diff --check`. Run the full current v15 suite once at the end; do not repeat the full version matrix for every comment pass.
- [x] Record actual test counts, baseline failures, review corrections, coverage limitations and commit IDs in ignored evidence. Confirm a clean worktree.
- [x] Report completion and leave main untouched. Any later merge to main must preserve both the landed input behavior and these commits.


## Execution record — 2026-10-23

| Task | Commit | Observed checks |
| --- | --- | --- |
| Input/SDL reconciliation | `44a483b` | 575 passes; four synchronization failures reproduced on unchanged main; 13 focused input/SDL checks passed |
| Save compatibility | `6232e74` | 84 focused checks passed across six versions |
| ANM decoding | `bd6243f` | Five available startup checks passed; six versions built/preprocessed |
| Input/configuration | `f4ae439` | 16 input, SDL and representative demo checks passed |
| Software Polymost | `3c13674` | 17 native rendering/engine checks and four real GL checks passed |
| Networking | `89563a4` | All 23 connection/refusal checks passed |
| Editor | `2f141ca` | All 10 focused editor/configuration/geometry-render checks passed |
| Audio | `fe6fb6a` | All three OPL, shutdown and mixer lifecycle checks passed |

After reconciliation, the seven source passes changed comments only. The final
combined check matched executable tokens for all 27 edited source/header files
and passed 169 preprocessing comparisons, including owning engine units for the
included renderer files. The final full 1.5 suite passed all 102 tests.

The initial 1.3D connection failure was a socket bind error from an occupied
configured port. The identical harness passed on fresh ports against both main
and the combined tree; Task 5 subsequently passed the configured-port tests too.
The four early-version baseline failures are `net_connect_v11_sw`,
`net_connect_v11_sw_coop`, `net_connect_v10_sw` and `net_connect_v10_sw_coop`.
Each also failed with synchronization errors on an unchanged main snapshot.
They were not repaired as part of a commenting pass.

Verification uses executable-token equality rather than artificial failing
comment-presence tests. There is no dedicated ANM opcode runtime test, 1.4 has no
registered startup fixture, and editor checks do not cover every interactive
command. Validation ran natively on macOS with dummy-audio lifecycle checks and
real GL rendering; Windows execution was not performed. Imported `dbopl.c`
remains byte-identical to the combined baseline. Evidence is retained under the
ignored `build-comment-coverage-evidence/` directory.


The independent final review checked every added comment hunk and the SDL
reconciliation boundaries. Two factual wording corrections removed a claim
about sprite-preview shading and clarified a pitch-bend semitone offset.
No implementation changes were required. `player_prediction` is registered only
for the early versions here: both 1.0 and 1.1 passed it in Task 0's full matrix;
it was not part of Task 3's 1.5 test count.

The review wording corrections are committed as `e0a8558`. Post-correction
executable-token and preprocessing comparisons passed again; no deferred review
findings remain.

The review deliberately left inherited mixer accounting, editor geometry/Z-test
quirks and protocol behavior unchanged, since these passes preserve behavior.
Malformed ANM cases, exhaustive interactive editing and untested platforms/devices
remain coverage limits. Older splits and comments predating this plan were
already reviewed separately. Final test execution and token evidence were
verified by the implementer; the independent reviewer inspected that evidence
and the source rather than rerunning the suites.

All work remains in the managed isolated worktree. Main and the user's ongoing
work in the original checkout were preserved.
