# Build Engine Source Split Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move existing Build engine code into the approved focused translation units with unchanged game/editor behavior and public interfaces.

**Architecture:** Make independently buildable extractions, starting with renames and the cache/filesystem split, followed by math, tiles, and display. Keep engine.c as the renderer/world coordinator and the sole ENGINE definition site. Keep polymost.c textually included there and expose only the small private interfaces listed below.

**Tech Stack:** Existing C, CMake/CTest, SDL2, classic software drawing, Polymost software and OpenGL 1.1/1.5/2.1 paths.

**Spec:** `docs/superpowers/specs/2026-10-21-engine-source-split-design.md` (updated 2026-10-22; user approved proceeding to this plan).

## Global Constraints

- This is a source-movement refactor: game and editor behavior, public function signatures, file formats, arithmetic, and version-specific logic must remain unchanged.
- There is no target percentage for the size of engine.c.
- Only engine.c defines ENGINE when including build.h.
- Keep engine.h, build.h, engine_protos.h, display.h, a.h, cache1d.h, and pragmas.h as the existing consumer-facing declaration surfaces.
- Preserve compiler definitions, platform source selection, linking, and polymost.c's HEADER_FILE_ONLY treatment. Keep ../videoopts.c and the existing GL support files in the target.
- Live video changes and their queue remain in source/game.c; the engine-free option helpers remain in source/videoopts.c and videoopts.h.
- No changes to game logic, audio, renderer features, networking protocols, configuration, save/demo formats, or supported platforms are in scope.
- Preserve the 19960104 beta revision and the pre-19960129 hitscan guards, as well as the later revision boundaries. Existing inline fixed-point operations in pragmas.h remain inline.
- Work only in `/Users/raynorpat/.codex/worktrees/e4fe/duke3d`. Source baseline: main `3d73ca1`, incorporated through `2460fc9`; spec revision: `86ec282`. Preserve the original checkout and its untracked files. Do not silently change the baseline during execution.
- Use existing tests first. This refactor follows baseline-green → moved-code-green; do not manufacture failures or add tests that assert filenames. New boundary assertions must pass the unmodified implementation before extraction.
- Run game/data tests in their build-tree scratch directories. Never launch the game from the original data folders. Keep evidence and generated files under ignored `build-split-*` directories.
- Keep commit subjects short and subsystem-prefixed, with no metadata. Leave work isolated for review; integration is a separate action.

## Review Focus

These are the five sensitive boundaries to check explicitly; each has an owning task and a concrete test below.

1. No tables.dat and repeated initengine: keep initialized math and early-version results without inventing a new reload/reset policy (Tasks 1, 4: engine_trace).
2. Cache-backed compressed reads: malformed blocks, endian reads, and older LZW variants must survive crossing files without duplicate cache state (Task 3: lzw_roundtrip, DOS saves and hardening).
3. Sprite moves/deletions and tile-backed camera views: retain list identities, allocated tile pointers, and restored view/renderer state (Tasks 1, 5, 6: engine_trace, render_e1l1_*_tile/tilt, render_gl).
4. Shader indices versus filtered RGB: clears, 2D colours, and palookup invalidation must target the same representation and upload at the same time (Task 6: render_gl, render_gl_filter).
5. A live renderer/mode change during fades: preserve the GL context lifetime, last frame, palette, and supersampling fallback rather than reinitializing resources in display.c (Task 6: render_gl_video, render_gl_supersample, SDL smoke).

## File Map

Paths below are relative to the worktree.

| Files | Final responsibility |
| --- | --- |
| source/build/engine.c | World/geometry/collision/map I/O, initialization coordinator, renderer state, sprite rasterization, rotatesprite queue/nextpage, textured map drawing, Polymost inclusion and private GL bridges |
| source/build/a.c → draw.c; mmulti.c → network.c | Rename complete implementations; retain a.h and public declarations |
| source/build/cache1d.c → cache.c + filesystem.c | Allocator/cache diagnostics versus GRP/file handles/LZW; retain cache1d.h |
| source/build/pragmas.c → math.c | Existing compiled helpers plus extracted math/tables; retain pragmas.h |
| source/build/tiles.c | Extract tile/voxel resources, animation and sprite bookkeeping |
| source/build/display.c | Extract view stack, pixel/text/line operations, palettes/brightness/fades |
| source/build/engine_internal.h | Private declarations actually needed by the extractions; no bulk renderer-state export |
| source/build/CMakeLists.txt | Compile each implementation exactly once |
| tests/engine_trace.c | Focused public math/sprite contract assertions before extraction |
| source/build/engine_protos.h, README.md, source/build/VENDORED.md | Update current location comments and document ownership/provenance |

Do not create math.h (which would shadow the C library header). Existing public headers stay named as above. Do not edit historical specs/plans or version-comparison reports to replace old source locations.

## Shared Verification Recipe

Use Bash for this recipe, from the worktree. `stage=before` records the unmodified-source baseline; repeat with `stage=after` in Task 7. Build directories and data arguments must be identical in both runs. The checked paths below exist locally as of planning; verify fingerprints through the existing tests before treating them as release references.

```bash
bash <<'BASH'
set -euo pipefail
stage=before
evidence="$PWD/build-split-evidence"
mkdir -p "$evidence"
data=(
  -DDUKE3D_V13D_DATA_DIR=/Users/raynorpat/Downloads/DUKE3D
  -DDUKE3D_V13D_SW_DATA_DIR=/Users/raynorpat/Downloads/DUKE3D13SW
  -DDUKE3D_V11_SW_DATA_DIR=/Users/raynorpat/Downloads/DUKE3D11SW
  -DDUKE3D_V10_SW_DATA_DIR=/Users/raynorpat/Downloads/DUKE3D10SW
  -DDUKE3D_V099_SW_DATA_DIR=/Users/raynorpat/Downloads/d3d_beta
  -DDUKE3D_DOS_SAVES_DIR=/Users/raynorpat/Downloads/pc-duke-nukem-3d-atomic-edition-savegame
)
for row in 'v15 1.5 OFF ON' 'v14 1.4 OFF ON' \
           'v13 1.3D OFF ON' 'v13sw 1.3D ON ON' \
           'v11sw 1.1 ON ON' 'v10sw 1.0 ON ON' 'v099sw 0.99 ON ON' \
           'v15-off 1.5 OFF OFF' 'v099sw-off 0.99 ON OFF'; do
  read -r id version shareware polymost <<< "$row"
  extra=(-DDUKE3D_GRP=)
  if [[ "$version" == 1.5 ]]; then
    extra+=(
      -DDUKE3D_GRP=/Users/raynorpat/Downloads/DUKE3D/DUKE3D.GRP
      -DDUKE3D_ATOMIC_GRP=/Users/raynorpat/duke3d-data/atomic15/DUKE3D.GRP
    )
  fi
  build="build-split-$id"
  cmake -S . -B "$build" -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DDUKE3D_FETCH_SDL=OFF \
    -DDUKE3D_VERSION="$version" -DDUKE3D_SHAREWARE="$shareware" \
    -DDUKE3D_POLYMOST="$polymost" -DDUKE3D_GL_TESTS=OFF \
    "${data[@]}" "${extra[@]}" > "$evidence/$stage-$id-configure.log" 2>&1
  cmake --build "$build" --parallel 8 > "$evidence/$stage-$id-build.log" 2>&1
  ctest --test-dir "$build" --show-only=json-v1 > "$evidence/$stage-$id-tests.json"
  ctest --test-dir "$build" --verbose --output-on-failure --parallel 1 \
    > "$evidence/$stage-$id-tests.log" 2>&1
  printf '%s %s passed\n' "$stage" "$id"
done
BASH
```

CTest runs serially to avoid network-port and scratch-file collisions. The executor runs long commands asynchronously, inspects logs and shares progress. A failed stage stops the loop; inspect and report the failure before moving source. Reuse the same recipe for the remaining rows after resolving the baseline disposition with the user; do not hide failures by disabling tests.

The recipe deliberately leaves DUKE3D_GRP empty for earlier releases, while each earlier build's data option registers its own release tests. The generic 1.5 renderer pins use the 1.3D GRP; DUKE3D_ATOMIC_GRP supplies matching Atomic data for DOS save checks. The separate GL suite keeps its Atomic fixture. The generic 1.5 boot test warns about the renderer fixture's release; also check the existing boot script with matching Atomic data, as in the initial baseline run. No original 1.4 data/capture has been established for this plan; its registered data-free/shared-layout save checks do not prove original 1.4 gameplay. Record that limitation.

Configure and build a separate 1.5 GL baseline, then run its four windowed tests:

```bash
cmake -S . -B build-split-gl -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DDUKE3D_VERSION=1.5 -DDUKE3D_SHAREWARE=OFF -DDUKE3D_POLYMOST=ON \
  -DDUKE3D_FETCH_SDL=OFF -DDUKE3D_GL_TESTS=ON \
  -DDUKE3D_GRP=/Users/raynorpat/duke3d-data/atomic15/DUKE3D.GRP
cmake --build build-split-gl --parallel 8
ctest --test-dir build-split-gl --verbose --output-on-failure --parallel 1 \
  -R '^render_gl($|_)' > build-split-evidence/before-gl-tests.log 2>&1
```

Repeat to `after-gl-tests.log` at the final gate. These four tests open real windows; do not force SDL's dummy driver. Record return-77 skips and unsupported tiers separately. Copy baseline rendering BMP/PNG outputs from the rendering tests to `build-split-evidence/before-images/<configuration>/` before subsequent runs overwrite them. Compare checksum lines and the same generated images after extraction, not whole logs with timings or paths. Retain existing test thresholds; do not re-pin outputs to make moves pass.

For the default 1.5 engine archive, snapshot defined external symbol names/types with `nm -g -U build-split-v15/source/build/libbuildengine.a` on macOS. Compare names/types, ignoring addresses and object headings, after extraction. Require every baseline external symbol to remain; newly external engine-private bridge names are the only intended additions. Use the platform equivalent on Linux/Windows.

### Task 1: Establish the Baseline and Pin Module Contracts

**Files:** Modify `tests/engine_trace.c`; existing CTest registration is unchanged. Evidence: ignored `build-split-evidence/`.

**Interfaces:** Consume only existing `build.h`/`engine_protos.h` public APIs and the existing `square()`/`check()` helpers. Produce assertions that protect later math and tile moves; introduce no production interface.

- [x] **Step 1:** Run the complete shared verification recipe, GL baseline, image and symbol snapshots on the unchanged production source. Record HEAD, compiler/CMake/SDL versions, registered test names, pass/failure/skip counts, and missing reference files. Decide whether any existing external DUKE3D_NET_PEERS builds have matching data before this run; freeze those arguments and executable identities for both runs, or record that only built-in protocol peers are exercised. Inspect actual results; this plan makes no claim they already pass.
- [x] **Step 2:** Before moving code, extend engine_trace with focused math assertions in its data-free working directory: after initengine, ksqrt(0/1/4) returns 0/1/2; getangle on the four unit axes returns 0/512/1024/1536; with randomseed reset to 0 and 1, the first krand returns 0 and 420 respectively. Repeat initengine and assert the same results. Keep the current slope/hitscan cases unchanged; they distinguish historical arithmetic beyond these simple roots.
- [x] **Step 3:** At the end of the existing trace cases, rebuild its two square sectors and initialize sprite lists. Insert two sprites into sector/status 0; check head/next/prev consistency. Move the first to sector 1 and status 3; assert its ID and the other sprite's list membership stay intact. setsprite at (1536,512,0) succeeds in sector 1. Delete that sprite, assert both lists exclude it, then insert into sector 1/status 3 and assert the freed ID is reused. Use public arrays and return values, not private allocator/layout assumptions.
- [x] **Step 4:** Build and run `ctest --test-dir build-split-<id> --output-on-failure -R '^engine_trace$'` in all nine configurations. Expected: every added assertion and every existing trace case passes before extraction. If a new assertion disagrees with the baseline, investigate it and correct the contract or report an existing issue; do not change production behavior in this task.
- [x] **Step 5:** Commit only the assertions: `tests: pin engine math and sprite contracts`.

### Task 2: Rename the Existing Drawing and Network Modules

**Files:** Rename `source/build/a.c` to `draw.c` and `mmulti.c` to `network.c`; modify `source/build/CMakeLists.txt` and current location comments in `engine_protos.h` and `tests/vline_counts.c`.

**Interfaces:** Consume/produce the same exported functions and globals. Keep `a.h`, cache1d.h and networking declarations unchanged; do not change the existing setgotpic declaration as an unrelated cleanup.

- [x] **Step 1:** Use Git renames, replace the two CMake source entries, and change only now-misleading location/end-of-file comments. Leave historical upstream references intact.
- [x] **Step 2:** Rebuild `build-split-v15`, `build-split-v099sw`, and `build-split-v15-off`, including both executables and all test targets. Expected: no duplicate or missing implementations.
- [x] **Step 3:** Run vline_counts, pragmas_semantics and available render_e1l1/render_maskorder tests on v15; run `ctest --test-dir build-split-<id> --output-on-failure -R '^net_'` for v13/v13sw/v11sw/v10sw after rebuilding them. On v15 use `ctest --test-dir build-split-v15 --output-on-failure -R '^(vline_counts|pragmas_semantics|render_e1l1.*|render_maskorder)$'`. Preserve the beta's lack of a network test requirement. Expected: original checksums and handshake results.
- [x] **Step 4:** Inspect move-aware diff and commit: `engine: name drawing and network modules`.

### Task 3: Separate Cache Allocation from Filesystem and LZW

**Files:** Replace `source/build/cache1d.c` with `cache.c` and `filesystem.c`; modify the CMake source list, engine_protos.h location headings, and stale active comments in `source/savegame.c` and `tests/v13d_save_decode.py` only.

**Interfaces:** Both modules use the existing cache1d.h declarations unchanged. Filesystem consumes `void allocache(intptr_t *, int32_t, unsigned char *)`; cache owns its existing diagnostics and allocator state. No new cross-module API is needed.

- [x] **Step 1:** Move the allocator description, MAXCACHEOBJECTS, cactype, allocator state and initcache/allocache/suckcache/agecache/reportandexit into cache.c. Retain the existing externally visible cache globals and the static cachesize there.
- [x] **Step 2:** Move MAXGROUPFILES/MAXOPENFILES and everything from their GRP/file-handle state through the LZW implementation into filesystem.c. Keep file tables, LZW buffers/locks, helper functions, USE_PHYSICSFS branches, endian handling, and all BUILD_ENGINEREV branches together. Duplicate only required include/license preambles; remove the old compiled file.
- [x] **Step 3:** Replace the single CMake entry with cache.c and filesystem.c. Build all nine configurations, then run their lzw_roundtrip tests and registered `lzw_dos_*`, `dos_save_*`, `save_refuse_*`, and `save_hardening_*` tests with `ctest --test-dir build-split-<id> --output-on-failure -R '^(lzw_roundtrip|lzw_dos_.*|dos_save_.*|save_refuse_.*|save_hardening_.*)$'`. Expected: malformed inputs survive and release-specific reference saves/readbacks match baseline.
- [x] **Step 4:** Compare allocator/file/LZW external symbol names with baseline and inspect moved bodies. Commit: `engine: separate cache and filesystem code`.

### Task 4: Extract Math Helpers and Table Loading

**Files:** Rename `source/build/pragmas.c` to `math.c`; extract from engine.c; create `source/build/engine_internal.h`; update CMake.

**Interfaces:** Retain all public pragmas.h functions and `int ksqrt(int32_t)`, `int getangle(int32_t,int32_t)`, `void rotatepoint(int32_t,int32_t,int32_t,int32_t,short,int32_t *,int32_t *)`, `int krand(void)`. Add private:

```c
void engine_loadtables(void);
int32_t engine_render_sqrt(uint32_t value);
int32_t engine_recip(int32_t value);
extern short radarang[1280];
extern int32_t pow2long[32];
int32_t getclipmask(int i1, int i2, int i3, int i4);
```

Reuse display.h's existing pow2char/textfont/smalltextfont declarations. Add `extern char britable[16][64];` to the private header for the loader/display dependency. No public math.h. getclipmask already has a public declaration in engine_protos.h; the matching private declaration lets engine.c call the moved implementation without importing that entire legacy header.

- [x] **Step 1:** Confirm Task 1's engine_trace contracts pass. Move the original compiled pragmas helpers unchanged, then the complete nsqrtasm/krecipasm/getclipmask implementations and their guarded assembly variants, square-root tables/init helpers, pow2char/pow2long, reciptable/fpuasm, radarang, tablesloaded, loadtables, getangle, ksqrt, rotatepoint, and krand. Keep radarang2 and renderer-specific aspect/voxel reciprocals in engine.c.
- [x] **Step 2:** Rename loadtables to private engine_loadtables and keep its one-time guard and tables.dat read order intact, including font/brightness reads. Call it at the original initialization point. In math.c retain the original local LOGICSQRT selection and all original arithmetic bodies.
- [x] **Step 3:** Implement the two private wrappers by delegating to the original nsqrtasm and krecipasm, with the same bit patterns/conversions. Engine rendering calls engine_render_sqrt/engine_recip; engine gameplay LOGICSQRT calls the existing public ksqrt. Do not alias LOGICSQRT to ksqrt inside math.c itself, which would recurse. Preserve existing externally defined nsqrtasm and other original symbols. Keep each assembly branch and its symbol-name macros with its implementation.
- [x] **Step 4:** Add only the header declarations needed by the moved definitions, include it after the existing platform/build prerequisites, and replace pragmas.c in CMake. No ENGINE define in math.c; do not touch inline pragmas.h arithmetic.
- [x] **Step 5:** Build all nine configurations. Run pragmas_semantics, engine_inside, engine_trace, game_helpers, the available per-release demo_sync tests and the 0.99 camera/episode checks using `ctest --test-dir build-split-<id> --output-on-failure -R '^(pragmas_semantics|engine_inside|engine_trace|game_helpers|demo_sync_.*|v099_render_camera|episode_end_v099_sw)$'`. Expected: repeated-init/absent-file assertions, overflow/hitscan/slope cases, demo states, and frame pins match baseline. The earliest and latest Polymost-off builds must link without GL.
- [x] **Step 6:** Compare move-aware diff/public symbols and commit: `engine: extract math and table helpers`.

### Task 5: Extract Tile Resources, Animation and Sprite Bookkeeping

**Files:** Create `source/build/tiles.c`; modify engine.c, engine_internal.h, CMake, and only polymost.c's animateoffs forward declaration.

**Interfaces:** Keep all existing public tile/sprite signatures. Add private `int animateoffs(short tilenum, short fakevar)` and `void engine_tiles_uninit(void)`. Private shared declarations:

```c
#if defined(USE_I386_ASM)
int setgotpic(int32_t value);
#else
void setgotpic(uint32_t value);
#endif
extern char picsiz[MAXTILES];
extern int32_t totalclocklock;
#ifdef SUPERBUILD
#define MAXVOXELS 512
#define MAXVOXMIPS 5
extern intptr_t voxoff[MAXVOXELS][MAXVOXMIPS];
extern int32_t voxlock[MAXVOXELS][MAXVOXMIPS];
#endif
```

Ensure engine.h's SUPERBUILD definition precedes using that part of the private header. Retain the existing global resource symbols at their original types even when only tiles.c uses them; existing build.h arrays are still defined by engine.c.

- [x] **Step 1:** Move BUILD_CACHEDEBUG, ART filename/handle state, pic/ART size and offset metadata, picsiz, voxel resource arrays, animateoffs, loadtile, allocatepermanenttile, loadpics, qloadkvx, copytilepiece, squarerotatetile, and all setsprite/initspritelists/insert/delete/change sector/status functions together. Keep animation's totalclocklock source and argument behavior. Move setgotpic and its complete guarded implementations/macros as tile-use bookkeeping, preserving its current branch-specific types. Remove polymost.c's now-redundant static animateoffs forward declaration so it uses the private header's declaration; do not alter its renderer implementation.
- [x] **Step 2:** Implement engine_tiles_uninit using exactly the original pic free/null and ART handle-close operations, and call it between transluc teardown and _uninitengine at the original uninitengine point. Do not add a new handle-reset or idempotence policy. Leave the existing initial tile-array clearing and voxel initialization loop in initengine, with private declarations as needed; do not reorder it around palette/table loading.
- [x] **Step 3:** Keep drawvox/drawsprite, loadvoxel/kloadvoxel's renderer-side placeholder, MAXXSIZ/MAXYSIZ/MAXZSIZ, lowrecip/distrecip, rasterization arrays, rotatesprite/flushperms/nextpage and lastageclock in engine.c. Add tiles.c to CMake and build the nine configurations.
- [x] **Step 4:** Run engine_trace's sprite contract assertions, game_helpers, available render_e1l1/tilt/tile and render_maskorder tests, v099_render_camera, and release boot/demo checks with `ctest --test-dir build-split-<id> --output-on-failure -R '^(engine_trace|game_helpers|render_e1l1.*|render_maskorder|v099_render_camera|game_boot.*|demo_sync_.*)$'`; run render_gl in the GL build for its camera view. Expected: list identities and reuse, resource-backed frames, and deterministic states match baseline.
- [x] **Step 5:** Inspect metadata ownership and uninitialization order and commit: `engine: extract tile and sprite management`.

### Task 6: Extract Display Operations through Private Polymost Bridges

**Files:** Create `source/build/display.c`; modify engine.c, engine_internal.h, and CMake. Keep polymost.c, sdl_driver.c, glext.c, glsubmit.c and source/videoopts.c implementations in their present ownership.

**Interfaces:** Keep public display/view/palette entry points and display.h unchanged. Add `void engine_loadpalette(void)` and, under `#if defined(POLYMOST) && defined(USE_OPENGL)`, these private engine.c bridges:

```c
int engine_gl_indexedtarget(void);       /* polymost_shaderdraw(0) && !glshcolor */
void engine_gl_2dcolor(int32_t col);      /* existing polymost_2dcolor(col) */
void engine_gl_lookupchanged(void);     /* existing glshlookupdirty = 1 */
void engine_gl_resetview(void);         /* existing glox1 = -1 */
```

Define bridges after polymost.c is included. Keep public setpolymost2dview in engine.c since it owns private GL viewport/fog state; display.c calls that existing API. No exported glshcolor/glshlookupdirty/glox1/gloy1/fog_is_on.

- [x] **Step 1:** Move loadpalette/initfastcolorlookup/getclosestcol/makepalookup/applybrightness/setbrightness/setpalettefade/getbasepalette/getfadedpalette and palette-only state to display.c. Rename only loadpalette to engine_loadpalette and call it at the same end-of-initengine point. Keep existing external palette/font/brightness symbols, static lookup scratch/basepal, and original preprocessor guards. Use a private 1024-byte palette conversion buffer in display.c instead of sharing engine.c's mirror scratch tempbuf; the mirror buffer stays local to engine.c.
- [x] **Step 2:** Move setgamemode, calcpixelaspect/setview/setaspect/setviewtotile/setviewback, drawline256, __printext256/printext256/printext256_noupdate, clearview/clearallviews, plotpixel/getpixel, and their view-stack buffers/bakumost/bakdmost. Keep queue and renderer/world state in engine.c; use existing display.h/build.h declarations plus only the missing private externs listed next.
- [x] **Step 3:** Add private declarations at original types for `int32_t viewoffset, viewingrangerecip, pixelaspect, stereopixelwidth, ostereopixelwidth; char *transluc, paletteloaded; char *globalpalwritten; int32_t globalpal; char kensmessage[128];`. These join Task 4/5 declarations; do not redeclare all renderer globals. Definitions have one owner; retain initializers and public symbol linkage. The loader writes display-owned fonts/britable through existing/private declarations. In display.c, under both POLYMOST and USE_OPENGL guards, include SDL_opengl.h, define GLSUB_WRAP, then include glsubmit.h and glext.h just as polymost.c currently does. These wrappers flush queued vertices before GL state changes; omitting them changes output even when calls look identical.
- [x] **Step 4:** At the exact old call sites, replace shader-target checks with engine_gl_indexedtarget, colours with engine_gl_2dcolor, lookup invalidation with engine_gl_lookupchanged, and only the clearallviews viewport-cache reset with engine_gl_resetview. Keep glViewport/scissor/clear commands, frame-drawn notifications, and relative call order unchanged. The indexed-target bridge must call polymost_shaderdraw even when filtering is active; a pure renderer/mode query loses its binding side effect. Preserve makepalookup's invalidation before its existing early returns.
- [x] **Step 5:** Add display.c to CMake and build all nine configurations plus build-split-gl. Run SDL smoke (including mode sizes, video fade, palette fade and screenshots), classic 2D/HUD and E1L1 checks, render-to-tile/tilt/mask ordering, all four GL tests serially, and registered demo_switch/net_videocycle checks. Use `ctest --test-dir build-split-v15 --output-on-failure -R '^(sdl_driver_smoke.*|render_.*|minihud_.*)$'`, the shared GL command, and `ctest --test-dir build-split-<id> --output-on-failure -R '^(demo_switch_.*|net_videocycle_.*)$'` where registered. Expected: no missing non-GL symbols; identical existing output pins, restored view state, index/RGB colour behavior, context/fade survival and fallbacks.
- [x] **Step 6:** Review frame bindings and pointer-width declarations separately from moved code. Commit: `engine: extract display and palette operations`.

### Task 7: Document Ownership and Complete the Regression Gate

**Files:** Modify README.md, source/build/VENDORED.md, and only stale current-location comments in engine_protos.h, engine.h, affected source/tests. Retain evidence under ignored build-split-evidence; add a concise execution-results section to this plan when implementing.

**Interfaces:** Final CMake list contains draw.c/cache.c/filesystem.c/math.c/tiles.c/display.c/network.c/engine.c plus the unchanged support/backend sources and ../videoopts.c. All existing public symbols/declaration surfaces remain available.

- [x] **Step 1:** Document final module responsibilities and upstream filename mapping without rewriting historical provenance. Confirm no active build path still compiles a.c, cache1d.c, pragmas.c or mmulti.c; polymost.c remains HEADER_FILE_ONLY and included once by engine.c. Inspect compile_commands.json for the actual compiler inputs, not just filenames in the source tree.
- [x] **Step 2:** Repeat the shared recipe with stage=after, the GL run, and image/symbol comparisons. Compare normalized registered test names to the baseline; no existing tests may disappear. Report exact pass/failure/skip totals per configuration and matching unpinned checksums on the same compiler/platform. Keep any baseline failures distinct from regressions.
- [x] **Step 3:** Exercise the registered protocol peers first. For DUKE3D_NET_PEERS, use only the external executable/data pairs frozen in Task 1; compare those same inputs before/after rather than adding peers after the baseline. Do not claim beta multiplayer, original 1.4 DOS validation, or unavailable GL tiers were checked. Existing optional routebot code is retained; if validating its enabled build, use a separate 0.99 build with DUKE3D_ROUTEBOT=ON and run its existing tests without creating new DOS references.
- [x] **Step 4:** Run `git diff --check` and review `git diff --color-moved=plain 3d73ca1 -- source/build` plus the test/docs diffs. Require each changed body line to be a boundary/linkage/buffer substitution described above. Audit BUILD_ENGINEREV guards, initializers, signedness, intptr_t, licensing and duplicate storage. The original checkout remains untouched.
- [x] **Step 5:** Record tested platforms and the remaining Linux/Windows CI requirement. Obtain whole-branch code review through the execution workflow chosen by the user, address findings within scope, and rerun the owning checks for any resulting changes. Commit documentation/results: `engine: document extracted module ownership`.

## Plan Review and Execution Handoff

The plan changes no implementation until reviewed. Recommended execution: **Native**, because the extraction tasks share a small evolving private header, must proceed in order, and can be verified at each checkpoint. Subagent-driven execution is available if the user prefers fresh task implementers and reviews at each boundary; no agents have been dispatched while planning.

After plan review and execution-method selection, use the corresponding required execution skill. Complete all seven tasks and the final review/verification gate before offering integration of the isolated worktree.

## Execution record

Execution method: Native, selected by the user on 2026-10-22. All seven tasks
are complete. Each extraction passed its named checks; the final full matrix
ran on source snapshot `0f14ffc`. Local raw evidence, the independent review,
comparison scripts/results and the execution ledger are retained in ignored
`build-split-evidence/`.

| Configuration | Before passed/total | After passed/total | Skipped |
|---|---:|---:|---:|
| v15 | 97/97 | 97/97 | 0 |
| v14 | 29/29 | 29/29 | 0 |
| v13 | 52/52 | 52/52 | 0 |
| v13sw | 51/51 | 51/51 | 0 |
| v11sw | 51/53 | 51/53 | 0 |
| v10sw | 50/52 | 50/52 | 0 |
| v099sw | 52/52 | 52/52 | 0 |
| v15-off | 92/92 | 92/92 | 0 |
| v099sw-off | 52/52 | 52/52 | 0 |
| Real OpenGL | 4/4 | 4/4 | 0 |

Final aggregate: 530/534 passed, four unchanged baseline failures, zero skips.
CTest retains its failure status for the two affected configurations; the
comparison gate accepts only their exact existing names and initialized-peer
Out Of Sync signatures. No new failure was observed. An additional boot check
using matching Atomic data also passed.

All nine registered test-name sets are unchanged. Compiler manifests confirm
each split implementation is compiled once, no retired source is compiled,
and polymost.c remains included by engine.c rather than compiled separately.
All 654 original defined external symbol names/types remain, with 11 planned
private helper/table additions. All 257 saved renderer images are byte-for-byte
identical: v15 42, v15-off 38, GL 177. Recorded checksums, including the unpinned
software Polymost outputs, match baseline on this compiler/platform.

Independent whole-branch review (`3d73ca1..0f14ffc`) found no Critical or
Important issue in supported configurations. Its body comparisons confirmed
preserved arithmetic, state ownership, initialization order and GL side
effects. The one Minor and all four declined areas are recorded below; no
production fix pass was needed. The reviewer read evidence before the last
rows/artifact comparisons finished; the final comparator subsequently checked
those completed gates. Working-tree `git diff --check` passed; whole-range
`git diff --check 3d73ca1..0f14ffc` exits 2 for the deferred whitespace warnings.

Baseline: v15 97/97; v14 29/29; v13 52/52; v13sw 51/51; v11sw 51/53;
v10sw 50/52; v099sw 52/52; v15-off 92/92; v099sw-off 52/52; GL 4/4.
The four failures are pre-existing `net_connect_v11_sw`,
`net_connect_v11_sw_coop`, `net_connect_v10_sw`, and
`net_connect_v10_sw_coop`, whose initialized peers report Out Of Sync.
The 1.1 failures reproduced before source changes in a separate retry. These
checks remain enabled; a new failure name or failure signature is rejected.

The matrix command above now corrects two baseline-recipe issues found in
execution: use the 1.3D renderer fixture for its existing checksum pins, and
initialize the optional flag array for macOS Bash 3.2's nounset behavior.
No output checksums or tolerances were changed. Atomic data remains in use for
DOS saves, the GL suite and the additional matching-release boot check.

Existing public declarations and symbols are retained. The new private helper
and table symbols are implementation dependencies, declared only in
`engine_internal.h`. No new public header is introduced.

Platforms: local macOS arm64. Linux/Windows validation remains the existing CI
matrix requirement; original 1.4 DOS captures and beta multiplayer are not
claimed. Built-in protocol peers are tested; no external peer binaries are
configured. Existing routebot source remains unchanged.

Source baseline: `3d73ca1`; completed source snapshot: `0f14ffc`. During
execution the original checkout advanced independently to `a999f4e` via
`e223d62`. This work did not write to or reset that checkout; its pre-existing
untracked `tests/__pycache__/` remains. Integrating newer main changes is a
separate step and has not been claimed as validated here.

Rulings made during execution, in ledger order (including the final review's
four declined areas):

1. Use the approved baseline-green to extraction-green process for this
   behavior-preserving move. Cost if wrong: an uncovered dependency must be
   caught by the regression suite or independent review.
2. Correct the renderer fixture to 1.3D while retaining Atomic data for its
   separate save/GL checks. Cost if wrong: misleading fixture validation;
   existing checksums and data fingerprints verify the choice. Initial fixture
   errors remain in evidence; no checksum or tolerance was changed.
3. Keep the four baseline network failures enabled and reject new names or
   signatures. Cost if wrong: another defect in an already failing path might
   escape; unchanged networking bodies and move review reduce that risk.
4. Defer root-cause repairs of those same four desyncs to their own task.
   Cost if wrong: a second fault could remain hidden behind the baseline desync.
5. Do not certify unsupported Watcom/DOS/I386 runtime or calling conventions.
   In particular, getclipmask's Watcom register annotation now lives in math.c
   while callers see the private prototype. Current CMake does not select
   that compiler/path. Cost if wrong: legacy builds may need ABI/declaration
   work before they run correctly; retained assembly text proves no runtime
   compatibility.
6. Do not certify the dormant PhysicsFS configuration. Cost if wrong: an
   optional-build integration fault remains undetected until it is enabled
   and tested, despite preserving its implementation.
7. Preserve existing repeated-init allocation and sprite-list boundary
   policies. Cost if wrong: existing resource or invalid-input defects remain;
   supported initialization and sprite-list contracts have baseline assertions.
8. Make the Bash 3.2 optional-array recipe compatible with nounset by using
   `extra=(-DDUKE3D_GRP=)`. Cost if wrong: an unintended default fixture option;
   configured paths and pinned outputs verify the corrected invocation.
9. After the actual full final run, use the evidence comparator as the
   task-completion command rather than repeat the same suite solely inside
   its bookkeeping wrapper. Cost if wrong: stale evidence could be accepted;
   the comparator requires all nine final rows and unchanged source/tests
   relative to the tested source snapshot.

Deferred minor: the whole-range whitespace check reports inherited whitespace
in moved math/tiles implementations and EOF blank lines in cache.c/display.c.
These have no runtime effect; no formatting churn is included. The range check
must not be described as clean.
