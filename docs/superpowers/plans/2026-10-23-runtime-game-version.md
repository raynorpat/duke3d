# Runtime Game Version Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build one executable that selects Duke release behavior by base-GRP fingerprint, requires overrides for unknown GRPs, blocks shareware custom maps, and resolves issue #1 with justified rendering references.

**Architecture:** A dependency-free profile library supplies immutable release metadata, fingerprints and one process-wide selection to the game and engine. Convert existing release branches in place, retain explicit historical file encodings, and select the profile before release-dependent initialization. Test all supported releases from one build.

**Tech Stack:** C99, CMake 3.24+, SDL2, CTest, existing Python 3 fixture/decoder tools; no new dependencies.

**Spec:** `docs/superpowers/specs/2026-10-23-runtime-game-version-design.md` (approved 2026-10-23).

## Global Constraints

- Preserve all seven currently supported profiles and their established tests.
- Unknown GRPs must require an override; there is no default release fallback.
- Any selected shareware profile enforces shareware restrictions, even for an unknown/modified archive.
- Overrides cannot select a full profile for a recognized shareware archive; reject that conflict.
- Selection cannot change later through add-ons, scripts, a demo, a save, a network peer or config values.
- Preserve existing release-specific saves, demos, network formats, arithmetic and random-call sequences.
- Keep the current C99, CMake and SDL2 foundation, existing resource lookup and case-insensitive base filename handling.
- Non-version options, including renderers, controls and the optional route bot, remain build options.
- Do not commit proprietary GRPs, scripts, saves or demos.
- No new release support, historical editor emulation, new base-path option, launcher or mid-process release switching.
- Leave unrelated untracked `__pycache__` directories alone. Follow subsystem-prefixed commit messages without metadata.

## Review Focus

- An override pair must be consumed before global/legacy argument consumers; its value must never become a bare-map request (Tasks 2, 9).
- Header/capacity changes must not alter DOS block sizes, script serialization or native-input transport layout (Tasks 3, 7).
- A loose/add-on replacement named like a stock map, or a CON redirect, must not bypass shareware restrictions (Task 9).
- A test helper bypassing game `main()` must initialize the correct profile before scripts, engine math or defaults (Tasks 3–11).
- Stale cached 1.3D rendering defaults must not silently become Atomic expectations, and unknown GRPs must not inherit stock references (Task 11).

## File ownership and interfaces

- Create `source/gameprofile.h` / `source/gameprofile.c`: immutable metadata, size/CRC fingerprints, streamed file inspection, profile resolution and process selection. Link a `duke3d_profile` static target from `buildengine`; no dependency on game objects or SDL headers.
- Create `source/gameversion.h` / `source/gameversion.c`: early override/help parsing only; no mounting, engine startup or gameplay.
- Retain numeric `GAMEVER_*` labels in `source/gamever.h`. Convert declarations and limits in `source/duke3d.h`, `source/names.h`, `source/soundefs.h` and `source/global.c`; convert behavior in its existing files.
- Keep codecs in `source/savegame.c` / `source/savegame.h`, historical LZW in `source/build/filesystem.c`, and game-level map policy in `source/premap.c`.
- Create `tests/profile_test_support.h`: profile selection for test entry points. Create focused detection/CLI/map tests named below; adapt existing tests rather than duplicating their DOS expectations.
- Create `tests/render_references.cmake`: verified fingerprint-specific rendering references and registration metadata. It is not a second runtime detection implementation.
- Update `CMakeLists.txt`, `cmake/CompileOptions.cmake`, `source/CMakeLists.txt`, `source/build/CMakeLists.txt`, `tests/CMakeLists.txt`, `.github/workflows/build.yml`, `README.md` and `source/build/VENDORED.md` only as required by this conversion.

Public profile interface to use consistently:

```c
typedef enum {
    GAME_099SW, GAME_10SW, GAME_11SW, GAME_13DSW,
    GAME_13D, GAME_14, GAME_15, GAME_PROFILE_COUNT
} GameProfileId;
typedef struct {
    GameProfileId id;
    const char *token, *name;
    int32_t gamever, enginerev;
    unsigned char byteversion, shareware, plutopak;
} GameProfile;
typedef struct {
    uint64_t size;
    uint32_t crc;
    const char *name;
    const GameProfile *profile; /* NULL for recognized unsupported Mac data */
    int shareware;
} GameGrp;
extern const GameProfile *game_profile;
const GameProfile *game_profile_get(GameProfileId id);
const GameProfile *game_profile_find(const char *token); /* case insensitive */
const GameGrp *game_grp_find(uint64_t size, uint32_t crc); /* NULL if unknown */
uint32_t grpcrc32(uint32_t crc, const unsigned char *buf, size_t count);
int game_grp_inspect(const char *path, uint64_t *size, uint32_t *crc,
                     char *error, size_t error_size); /* 1 success, 0 error */
typedef enum { GAME_RESOLVE_OK, GAME_RESOLVE_NEEDS_OVERRIDE,
               GAME_RESOLVE_UNSUPPORTED, GAME_RESOLVE_SHAREWARE_CONFLICT }
    GameResolveResult;
GameResolveResult game_profile_resolve(const GameGrp *detected,
    const GameProfile *forced, const GameProfile **selected);
int game_profile_select(const GameProfile *profile); /* 1 success, 0 conflict */
```

All returned descriptor pointers are immutable and stable. Resolution is pure;
selection accepts the first profile and repeat selection of the same descriptor,
but rejects a different selection. In the final product `game_profile` begins
NULL and every entry point selects explicitly. Test profile variations run in
separate CTest processes rather than resetting production state.

## Conversion sequence and common checks

Tasks 3–8 are a coupled source migration. Keep existing version builds usable
while converting: a temporary `source/gameprofile_legacy.c` initializes the
profile from existing `DUKE3D_GAMEVER`/`VOLUMEONE` settings through
`void game_profile_select_build_default(void)`. Compile this small bridge with
the existing game/compile option interfaces, without new version defines.
Call it at existing game/test entry points until Task 9 replaces game startup.
Converted branches use `game_profile` fields, never the legacy macros. Remove
the bridge and every caller in Task 10. Do not expose runtime overrides while
unconverted release branches remain. This is a migration aid, not shipped
fallback behavior.

Before execution use the worktree skill to create/detect isolation. Use a fresh
`build-runtime` directory for the final configuration. Capture baseline fixture
paths and commands in an ignored work log; do not hardcode personal paths into
tracked tests. Existing inputs are `DUKE3D_GRP`, `DUKE3D_ATOMIC_GRP`,
`DUKE3D_DOS_SAVES_DIR`, `DUKE3D_V13D_DATA_DIR`,
`DUKE3D_V13D_SW_DATA_DIR`, `DUKE3D_V11_SW_DATA_DIR`,
`DUKE3D_V10_SW_DATA_DIR` and `DUKE3D_V099_SW_DATA_DIR`.

For each implementation task: run its new/extended assertions on the preceding
code and observe a specific missing/incorrect behavior; implement; rebuild and
run the same checks; then commit the task's explicit files. A compile failure
proves a missing interface only, not a behavioral regression. Retain previous
baseline comparisons through each conversion. Use isolated test output paths
and existing SDL dummy-driver properties. Do not parallelize mutations of the
same test fixture/config directory.

### Task 1: Capture release baselines and investigate issue #1

**Files:** Read existing release tests and rendering harnesses; create `docs/render-reference-investigation.md`. Store captures and logs in ignored build directories.

**Interfaces:** Consumes current build options and supplied fixture paths; produces a baseline/fixture inventory and evidence identifying the rendering differences. No runtime API yet.

- [ ] Record commit and toolchain, verify each available GRP's size/CRC, and list available DOS demos/saves. Configure the seven legacy combinations and capture existing CTest results; distinguish missing fixtures, existing failures and new failures. Read `docs/superpowers/plans/2026-10-23-input-improvements.md` for reported pre-existing 1.1 network failures and independently check any relevant failure before classifying it.
- [ ] Reproduce issue #1 with 1.3D and Atomic using clean reference caches and the unchanged pins. Run `ctest --test-dir <baseline-build> -R '^render_(e1l1(_tilt)?|classic2d_.*)$' --output-on-failure`; preserve complete pass/failure output and captures. Reported Atomic observations are `99d777cd`, `67030c2c` and map hashes `b08ff175`, `3378e21d`, `e5dc3af1`, `d99dddf4`, `ddec2065`; do not install them as expectations yet.
- [ ] Compare relevant MAP/ART/palette inputs from both archives and inspect rendered differences. Render both archives under the same engine revision as a control, then their matching revisions. Document whether each difference is asset-dependent, intended historical engine behavior or a defect. If unresolved, keep investigating during Task 11; do not bless unexplained pins.
- [ ] Commit the evidence document: `tests: document release rendering baseline differences`. Do not change rendering references in this task.

### Task 2: Pure profile detection, resolution and early argument parsing

**Files:** Create `source/gameprofile.h`, `source/gameprofile.c`, `source/gameversion.h`, `source/gameversion.c`, `tests/game_profile.c`, `tests/game_version_args.c`; modify root/test CMake registration.

**Interfaces:** Produces the public interfaces above and `int gameversion_parse_args(int *argc, char **argv, const GameProfile **forced, int *help, char *error, size_t error_size)` (1 success, 0 error). Parsing removes only recognized override/help arguments in place and retains all other argument order and final NULL.

- [ ] Add independent literal test cases for `(token, gamever, enginerev, byteversion, shareware, plutopak)`:
  `0.99/199601040/19960104/9/1/0`, `1.0/199601290/19960129/10/1/0`, `1.1/199602200/19960220/12/1/0`, `1.3d-shareware/199604190/19960419/14/1/0`, `1.3d/199604190/19960419/103/0/0`, `1.4/199610210/19960820/116/0/1`, `1.5/199610320/20000614/117/0/1`.
- [ ] Assert literal fingerprints from existing `knowngrps`: `9690241/02f18900`, `10429258/a28aa589`, `10442980/912e1e8d`, `11035779/983ad923`, `26524524/bbc9ce44`, `44348015/f514a6ac`, `44356548/fd3dcff1`. Assert Mac `10444391/c5f71561` is recognized shareware with no supported profile; same-size wrong CRC is unknown. Assert CRC of `123456789` is `cbf43926`, including split-buffer updates.
- [ ] Generate tiny valid/truncated/invalid GRPs in the test output directory. Assert full streamed inspection, unknown-size CRC, unreadable/malformed/truncated rejection, and every resolution outcome from the spec. Structural validation must bound entry counts, directory size and member extents against the actual file before the legacy mounter reads it; no data authenticity heuristics.
- [ ] Assert `-gameversion 1.3D-shareware` resolves correctly; missing/unknown/repeated values fail; removal preserves `/gaddon.grp`, `/l1`, `-map custom.map`, diagnostic paths and `-net` arguments exactly. Recognize existing `?`, `/?` plus `-help`/`--help` before data lookup. Run the two new test targets and observe failures.
- [ ] Implement the immutable tables, pure resolution and parser. Keep archive inspection separate from mounting; allow forced recognized mismatches except shareware-to-full. Initially link the new library only where needed; move the existing global `grpcrc32` body to it without leaving duplicate symbols and keep legacy `grpmismatch` working until Task 9.
- [ ] Run `cmake --build build-runtime --target game_profile game_version_args` and `ctest --test-dir build-runtime -R '^(game_profile|game_version_args)$' --output-on-failure`; expect all cases pass. Commit: `game: add runtime release profiles and override parsing`.

### Task 3: Shared declarations, capacities and explicit test selection

**Files:** Modify `source/duke3d.h`, `source/gamever.h`, `source/global.c`, `source/game.c`, `source/astub.c`, CMake targets and version-sensitive test entry points; create temporary `source/gameprofile_legacy.c` and `tests/profile_test_support.h`.

**Interfaces:** Produces `void game_profile_select_build_default(void)` during migration and test-only `int test_select_profile(int *argc, char **argv, const char *fallback_token)` which consumes `-test-gameversion TOKEN` before positional test arguments. Game-linked tests later call `game_init_release_defaults()` from Task 5 after selection.

- [ ] Extend profile tests to assert first selection succeeds, same selection is idempotent, conflicting selection fails and leaves the original pointer unchanged. Add native layout/capacity checks: packed `sizeof(input)==10`, `sizeof(sectortype)==40`, `sizeof(walltype)==32`, `sizeof(spritetype)==44`, player weapon-record storage at least 256 entries, script storage at least 20460 entries and all historical fields available together. Observe the missing-field/capacity failures.
- [ ] Make player/global declarations a superset. Introduce fixed allocation constants `GAME_SCRIPT_CAPACITY=20460` and `GAME_WEAPRECS_CAPACITY=256`; keep active limits distinct. Do not change DOS serialization lengths via allocation constants. Preserve the already fixed 10-byte native input layout. Replace native-state diagnostic comparisons affected by the player layout with existing explicit DOS codecs or field comparisons, preserving their original assertions.
- [ ] Add the temporary bridge and select before any converted code. Tests accepting multiple profiles run once per CTest process, consuming `-test-gameversion`; existing positional paths retain their meaning. Engine-only entry points use profile selection without game defaults. Editor selects `GAME_15` explicitly. No dependency from engine to game objects.
- [ ] Build all targets and run `game_profile`, `version_macros`, `savegame_codec`, `demo_writer` and engine tests in the baseline legacy configurations; expect prior results unchanged. Commit: `game: share release state declarations across profiles`.

### Task 4: Historical engine calculations and both LZW formats

**Files:** Modify `source/build/build.h`, `source/build/engine.c`, `source/build/math.c`, `source/build/filesystem.c`, `source/build/CMakeLists.txt`, `tests/engine_inside.c`, `tests/engine_trace.c`, `tests/lzw_roundtrip.c`, `tests/CMakeLists.txt`.

**Interfaces:** Consumes selected `game_profile->enginerev`; existing Build function signatures stay unchanged. Produces engine and compression functions valid for every profile in the same executable.

- [ ] Convert independent expected results in `engine_inside`/`engine_trace` into runtime per-profile cases and register all seven selections. Add golden compressed-byte and matching-decoder checks for each existing DOS LZW family, retaining round-trip tests. Use synthetic baseline vectors whose encodings distinguish the families; these codecs have no universal family tag, so do not require every wrong-family byte stream to be identifiable. Run early profile cases against the unconverted default engine and observe the known math/codec differences.
- [ ] Replace every engine revision guard with runtime branches. Where entire functions differ, keep narrowly named historical helpers behind existing public functions. Preserve overflow behavior and comparisons exactly. Select the revision before math tables initialize and retain the repeated-`initengine()` contracts. Compile both LZW implementations and dispatch from current revision.
- [ ] Run `ctest --test-dir build-runtime -R '^(engine_inside|engine_trace|lzw_roundtrip)(_|$)' --output-on-failure`, plus existing DOS LZW fixture tests where supplied; expect independent expected outputs for each profile. Rebuild editor and renderer targets. Commit: `engine: select historical math and compression at runtime`.

### Task 5: CON dialects, release defaults and input rules

**Files:** Modify `source/gamedef.c`, `source/global.c`, `source/control.c`, `source/names.h`, `source/soundefs.h`, `source/duke3d.h`, `source/funct.h`, `tests/con_compile.c`, `tests/con_runtime_v099.c`, `tests/input_polling.c`, `tests/game_helpers.c`, `tests/CMakeLists.txt`; create `tests/game_release.c`.

**Interfaces:** Produces `void game_init_release_defaults(void)`, `int game_freeze_weapon(void)`, `int game_remote_weapon(void)`, `int game_script_limit(void)`, `int game_sound_limit(void)`, `int game_weaprecs_limit(void)`, and `int game_rand(void)`; consumes active profile. Put shared declarations in `funct.h` (already included by game consumers).

- [ ] Add `game_release_<token>` assertions: freeze/remote slots are `10/9` before 1.3D and `9/10` afterward; script limits are `20000` before 1.4 and `20460` afterward; sound limits are `255/348/450` across pre-1.3D/1.3D/1.4+; weapon-record limits are `256` before 1.4 and `16` afterward. Assert beta skill/episode names, 1.0/1.1 spelling, later CON-defined defaults, keyword counts `93/94/98/112` and version-specific keyword substitutions. Preserve existing DOS-derived 1.0 random-sequence and input-scale assertions. Observe failures for profiles different from the compiled build.
- [ ] Initialize names, keyword substitutions and release globals explicitly after selection; compile all dialect branches in place. Preserve existing opcode numeric identities and compiler limits. Replace the conditional `rand()` macro with explicit `game_rand()` calls at its existing game call sites; use Watcom's existing sequence only for 1.0 and the prior libc path otherwise. Keep random call counts/order unchanged.
- [ ] Add runtime helpers for varying weapon/asset IDs and active limits. Do not substitute a runtime value into a C `case` label or file-scope initializer: convert such consumers in the owning task, using fixed numeric cases and runtime discrimination where required. Retain legacy symbols only until their remaining consumers migrate, then remove them in Task 10.
- [ ] Select and initialize release defaults in game-linked test mains before scripts/config/input. Run `ctest --test-dir build-runtime -R '^(game_release|con_compile|con_runtime_v099|input_polling|game_helpers)' --output-on-failure`; use real release CON compile checks when available. Commit: `game: select script dialects and input defaults at runtime`.

### Task 6: Player, actor, sector and level behavior

**Files:** Modify `source/player.c`, `source/actors.c`, `source/sector.c`, `source/premap.c`; adapt `tests/early_scenarios.c`, `tests/player_prediction.c`, `tests/player_death.c`, `tests/v099_player.c`, `tests/v099_actors.c`, `tests/v099_sector.c`, `tests/v099_render_camera.c`, `tests/CMakeLists.txt`.

**Interfaces:** Consumes profile fields and Task 5's helpers; existing gameplay function signatures stay unchanged. This task preserves historical behavior; Task 9 owns the new map prohibition.

- [ ] Make existing DOS-derived scenarios executable for their intended runtime profiles. Register early scenarios for 1.0/1.1 and beta scenarios for 0.99 regardless of build settings. Add boundary assertions for both meanings of weapon slots 9/10 and full/shareware inventory/weapon availability; run them against unconverted routines and observe wrong-release behavior.
- [ ] Convert release and edition branches in these four files. Use the selected revision for prediction as well as simulation. Keep operations/random calls in original order. Replace varying IDs and active limits with Task 5 interfaces, resolving dynamic case labels explicitly. Preserve level/music/inventory initialization order.
- [ ] Build and run `ctest --test-dir build-runtime -R '^(early_scenarios|player_prediction|player_death|v099_)' --output-on-failure`. Run available DOS demo synchronization cases as conversions permit; investigate the first divergent tic rather than changing references. Commit: `game: select historical gameplay behavior at runtime`.

### Task 7: Runtime DOS save layouts and validation

**Files:** Modify `source/savegame.c`, `source/savegame.h`, save/load portions of `source/menues.c`, save scanning in `source/config.c`, `tests/savegame_codec.c`, `tests/savegame_hardening.py`, `tests/dos_save.cmake`, `tests/dos_save_v13d.cmake`, `tests/CMakeLists.txt`.

**Interfaces:** Existing codec functions remain public. Add `const SaveGameLayout *sg_layout(void)` in `savegame.h`, with integer fields `player_size`, `dos_maxplayers`, `dos_maxhittypes`, `dos_maxtiles`, `dos_maxscript`, `player_dfsize`, `player_dfcount`, `player_block0`, `playerorig_dfsize`, `playerorig_dfcount` corresponding to current `SG_*` values. Fixed-size record constants remain constants; allocation capacity is never the on-disk count.

- [ ] Convert the existing independent player byte-offset tests into per-profile functions and register seven processes. Assert player encoded sizes `0x33f` (beta), `0x34e` (1.0/1.1), `0x552` (1.3D) and `911` (1.4/1.5); assert the original 8/16-player counts and original element-size/count LZW block grouping. Keep sentinel bytes past each record unchanged and fields absent on disk initialized as before. Observe wrong-layout failures.
- [ ] Retain each historical `sg_player` implementation under a narrow runtime dispatcher. Convert all version-varying `SG_*` consumers, save headers/scanning, script relocation and validation to `sg_layout()` and profile identity. Keep stack/static buffers at fixed maximum capacities; never introduce runtime sizes into static array declarations. Preserve early unversioned format checks and current limits on detectable mismatches.
- [ ] Adapt the codec CLI's `-test-gameversion` consumption without changing the remaining save path position. Retain independent Python DOS decoders unchanged unless their invocation needs a runtime token. Run codec and supplied DOS load/round-trip/refusal/hostile-save tests for each profile; decoded fields/block boundaries must match baseline. Commit: `save: select DOS layouts and validation from runtime profiles`.

### Task 8: Game control flow, menus, demos and networking

**Files:** Modify remaining release branches in `source/game.c`, `source/menues.c`, `source/config.c`; adapt `tests/demo_writer.c`, `tests/game_helpers.c`, `tests/gamepad.c`, `tests/v099_game.c`, `tests/v099_menus.c`, `tests/demo_sync_v13d.cmake`, `tests/net_v13d.cmake`, `tests/net_protocol_peer.c`, `tests/CMakeLists.txt`.

**Interfaces:** Consumes selected profile/defaults/codec interfaces; preserves existing public game/demo/network functions. Produces all release behavior in one binary before automatic detection is enabled.

- [ ] Register demo writer/buffer-finalization and truncated-header tests by runtime profile; assert all protocol bytes independently (`9,10,12,14,103,116,117`). Preserve packed demo input conversion, player counts and block boundaries. Test matching peers with isolated data directories and wrong-profile peers refusing with existing protocol checks. Run failing alternate-profile cases before conversion.
- [ ] Convert game/menu/config edition branches, banners, command-line dialects, episode screens, demo read/write and network code to runtime profile conditions. Keep old demo formats explicit; do not use the selected profile's label to alter a file header and pretend conversion occurred. Preserve existing diagnostic, boot-test and route-bot hooks, conditional only on their non-version feature flags.
- [ ] Replace compile-time expected branches in affected tests with independently chosen runtime expectations. Avoid deriving expected protocol values from the production table. Run helper/menu/demo/network checks and available original DOS demo synchronization tests; expect baseline results per release. Commit: `game: select menus demos and network behavior at runtime`.

### Task 9: Automatic startup selection and shareware map enforcement

**Files:** Modify `source/game.c`, `source/config.c`, `source/premap.c`, `source/menues.c`, `source/funct.h`, `source/build/engine.c` only if needed for base-only map lookup; create `tests/game_version_cli.cmake`, `tests/shareware_maps.c`, `tests/shareware_maps.cmake`; update test registration.

**Interfaces:** Consumes Tasks 2–8. Add `int game_map_allowed(int volume, int level, const char *map_name, int custom)` for the central game policy, and `int game_load_campaign_map(char *filename, int32_t *x, int32_t *y, int32_t *z, short *ang, short *sector)` for profile-aware map loading. Place implementations in `premap.c`, declarations in `funct.h`; full profiles delegate normal lookup unchanged.

- [ ] Add real-executable CLI checks for help without GRP; unknown valid GRP with/without override; bad/duplicate arguments; malformed GRP even with override; case-insensitive `DUKE3D.GRP`; supported fingerprint detection using local fixtures; recognized-shareware/full conflict; and `/g` archives not changing the selected profile. Assert errors precede CON compilation/network/engine startup, include path/size/CRC where inspected, and exit nonzero. On macOS, cover an executable launched under a temporary `Contents/MacOS/` path: detection must follow the existing bundle working-directory resolution. Observe existing startup failures against the new contract.
- [ ] Add shareware policy tests for all four profiles: valid campaign slots/names accepted; extra episodes, user-file slots, noncampaign names, `-map`/bare-map requests, network map requests, demo/save custom-map markers and CON redirects rejected. Generate tiny structurally valid base/loose/add-on maps with distinguishable coordinates; assert the loading boundary reads the base version of a stock-named map, while full profiles load the override. Keep non-map asset overrides working. Use original GRPs only in optional integration cases.
- [ ] Parse/consume the early option before `_platform_init` captures global arguments. For help return without data. Preserve `_platform_init`'s bundle-directory handling in `source/build/sdl_main.c` before resolving the base path; its platform work must stay release-independent. Then fingerprint/validate the base before mounting, select exactly once, initialize defaults, print the correct banner and proceed with engine/game startup. Remove `OWNGRP`, `knowngrps`, `checkgrp` and `grpmismatch` from `game.c` once their replacement tests cover the contract. Do not mount add-ons before the base archive.
- [ ] Derive each shareware campaign allowlist from the existing release's documented/shipped map set, checking 0.99 separately. Reject custom selection at entry points and again at the game load boundary. Use the existing base-only lookup suffix on a bounded local filename copy, avoiding mutation of persistent names. Guard save/demo/network custom markers before applying their custom-map selection. Do not add a global map ban to the Build editor.
- [ ] Run `ctest --test-dir build-runtime -R '^(game_version_cli|shareware_maps|game_boot|demo_|net_)' --output-on-failure`, plus beta campaign boot/end tests. Expect explicit rejection and base-only map bytes for shareware, unchanged full custom-map support. Commit: `game: detect base GRP releases and enforce shareware maps`.

### Task 10: One build and a combined release test matrix

**Files:** Modify all CMake files named above, `.github/workflows/build.yml`, `tests/profile_test_support.h`, `tests/version_config_rejects.cmake`, `tests/boot_game.c`, `tests/con_compile_v13d.cmake`, `tests/v13d_check.cmake`, `tests/episode_end_v099.cmake`, `tests/routebot_record.cmake`, test registrations/entry points; delete `source/gameprofile_legacy.c` and `tests/version_macros.c` after replacement.

**Interfaces:** Final `game_profile` starts NULL. All executable/test/editor entry points select explicitly. Replace macro checks with `game_profile` tests; retain `version_config_rejects` name but change its contract to obsolete-option diagnostics. Add only optional `DUKE3D_V14_DATA_DIR` for a verified 1.4 fixture, since no current dedicated input covers that profile's complete boot.

- [ ] Add configure tests showing explicit obsolete settings (including `OFF`, `1.5` and values loaded from a reused cache) fail with migration guidance, while a clean no-version configure succeeds. Add a CTest registration check that supplied beta, 1.0, 1.1, 1.3D shareware/full, 1.4 and Atomic data register their distinct applicable cases together. Observe the old version-gated configuration failing these assertions.
- [ ] Remove version CMake options and compile definitions, every temporary bridge/caller, and remaining compile-time release selectors. Replace old runtime constants (`BYTEVERSION`, varying weapon IDs, active-size macros) at remaining consumers. Keep numeric release labels and unrelated platform/renderer/routebot guards. Initialize the editor explicitly to `GAME_15` before engine use; helpers that do not need data choose their test profile explicitly.
- [ ] Register test profile variations as separate processes with unique output directories. Mount/select real-data rendering/boot helpers by fingerprint before engine use; no-data helpers consume `-test-gameversion` before their existing positional arguments. Adapt external decoder/hardening commands to receive the fixture's expected profile rather than a removed build option. Preserve stock CON tests as explicit 1.4/1.5 cases and beta-only tests as explicit beta cases.
- [ ] Reduce CI to one unified build per existing OS, running all data-free profile cases. Update artifact naming and remove version-matrix build flags. In a fresh build run `cmake -S . -B build-runtime` with available fixture flags, `cmake --build build-runtime --parallel`, then `ctest --test-dir build-runtime --output-on-failure`; compare against the baseline inventory. Commit: `build: replace release variants with one runtime build`.

### Task 11: Verified rendering references for 1.3D and Atomic

**Files:** Modify `tests/render_e1l1.c`, `tests/render_classic2d.c`, `tests/render_gl.c`, `tests/render_horplus.c`, `tests/render_maskorder.c`, `tests/minihud.c`, `tests/CMakeLists.txt`, other existing render test mains as necessary; create `tests/render_references.cmake`, `tests/render_reference_config.cmake`; update `docs/render-reference-investigation.md`.

**Interfaces:** Rendering helpers inspect/select the GRP before `initengine()`. Produce `game_profile --identify PATH` as a test-tool mode that prints only the canonical fingerprint token or `unknown`/`unsupported` and errors on invalid files; configure-time reference selection invokes this built test tool through a CTest wrapper, so CMake does not implement separate CRC detection. The wrapper owns output/reference choice and invokes rendering executables with their existing checksum parameters.

- [ ] Complete Task 1's investigation before changing pins. If a renderer defect is demonstrated, add a failing minimal scenario to the existing renderer harness that isolates it; fix only that defect and retain any expected references still correct. Otherwise document measured input/output evidence for legitimate edition differences. Original issue observations are not sufficient evidence by themselves.
- [ ] Add wrapper/config tests for recognized 1.3D and Atomic, unknown modified data, renamed files, an explicit checksum override and a reused cache containing old 1.3D default values. Assert known editions use their own verified references; unknown data has no stock golden assertions and clearly reports that absence unless given explicit references. Introduce edition-scoped override cache keys and migrate an old nondefault explicit override only to the configured generic GRP case; old defaults must not override the new per-edition table. Document unavoidable ambiguity if an old explicit value exactly equaled its default.
- [ ] Register `render_e1l1_13d`, `render_e1l1_atomic`, tilt counterparts and `render_classic2d_<edition>_<size>` cases whenever both fixtures are provided. Retain 1.3D references verbatim except evidence-backed renderer fixes. Add Atomic references justified by the investigation, preserving HUD assertions and all existing resolutions (`320x200`, `640x400`, `640x480`, `1280x1024`, `480x640`, `1280x720`, `1600x900`). Keep floating-point Polymost expectations opt-in with their platform caveats.
- [ ] Run `ctest --test-dir build-runtime -R '^(render_reference_config|render_e1l1_|render_classic2d_)' --output-on-failure` with both fixtures and a clean cache, then the reused-cache regression. Expect both reference families asserted together and no unexplained changes. Commit: `tests: select verified rendering references by GRP edition`.

### Task 12: Documentation, full verification and review

**Files:** Modify `README.md`, `source/build/VENDORED.md`, and this plan's execution record. Keep historical specs/ledgers unchanged unless linking a directly relevant new explanation.

**Interfaces:** Final acceptance criteria are the approved spec and the baseline inventory; no new product interfaces.

- [ ] Update launch/build/test examples: one build; checksum selection; `-gameversion` tokens; strict unknown-GRP failure; supported editions; shareware map restrictions; editor policy; obsolete-cache migration; edition-specific rendering references. Record issue #1's established cause and fix without claiming unrun platform coverage.
- [ ] Audit source, tests, CI and current build docs with `rg -n 'DUKE3D_GAMEVER|BUILD_ENGINEREV|VOLUMEONE|VOLUMEALL|PLUTOPAK|DUKE3D_VERSION|DUKE3D_SHAREWARE|OWNGRP' source tests cmake .github CMakeLists.txt README.md`. Manually classify hits: fixed asset names/comments and explicit obsolete-option rejection may remain; no release-selecting defines/guards or build variants may remain. Inspect dynamic ID case labels, static initializers, `sizeof(script)`, `sizeof(ps)`, serialized input and every remaining `SG_*`/capacity consumer.
- [ ] Run fresh full build/CTest with all supplied fixtures; also configure/build/test once without commercial data and build the route-bot option. Run the editor/default-renderer smoke checks and optional GL tests in a usable context. Confirm no-data profile tests cover all seven profiles, same-executable matching/mismatching multiplayer checks run, stock shareware campaign boots, full custom maps work and shareware custom maps cannot load.
- [ ] Compare release demo final-state fields and decoded DOS saves against preserved independent references, not regenerated expectations. Investigate all new failures. Record missing fixtures, reproducible pre-existing failures and platform/hardware-only limits precisely. Do not mark an unavailable matrix row as passed.
- [ ] Perform a whole-branch code review with special attention to random-call ordering, encoding counts, early initialization, map-source restrictions and rendering evidence. Resolve findings and rerun only affected checks, then the final required suite if changes affect shared behavior. Commit documentation: `game: document runtime release selection and verification`.

## Plan self-review

Spec coverage: detection/override/unsupported data (Tasks 2, 9); runtime declarations
and limits (3, 5); engine/compression (4); gameplay (5, 6, 8); DOS codecs and
compatibility (7, 8, 12); shareware maps (9); unified build/editor/CI/test matrix
(10); issue #1/references (1, 11); documentation and final acceptance (12).
Review Focus cases each have explicit owning tests above. No implementation
has been performed as part of writing this plan.

## Execution record

Executed natively in `feature/runtime-game-version`, separate managed worktree.
Baseline `58bdcc3`; all twelve tasks committed and verified in sequence.
Final fresh review found one archive-boundary issue, fixed with RED→GREEN
regressions and the full acceptance matrix.
The [runtime verification report](../../runtime-release-verification.md) records
fixture coverage, baseline exceptions and final review results.
