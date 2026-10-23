# Build engine source split

Date: 2026-10-21
Updated: 2026-10-22
Status: Proposed for written-spec review; module split approved in conversation

## Intent and scope

Reorganize the existing Build engine into focused translation units, following
the user's reference diagram. The user approved the module split below.
This is a source-movement refactor: game and editor behavior, public function
signatures, file formats, arithmetic, and version-specific logic must remain
unchanged. There is no target percentage for the size of engine.c.

Work in the managed worktree at
`/Users/raynorpat/.codex/worktrees/e4fe/duke3d`, created from `v099` at
`2af4260f27424b656a8c44f7e5cadee5b19ab556` and now synchronized with `main`
through `3d73ca1` (2026-10-22). Leave the original checkout and its untracked
files alone. The worktree has detached HEAD.

## Merged baseline

The refactor starts from the merged implementation, not the original design
branches. These changes are existing behavior to preserve:

- Polymost's GL 1.1 immediate, GL 1.5 VBO, and GL 2.1 shader paths, including
  palette-index shading, translucency, brightness/fades, internal resolution,
  supersampling, and nearest/bilinear/trilinear filtering.
- The live VIDEO page, renderer and display-mode switches, resolution/vsync
  changes, and transition fades. Preserve the SDL backend's GL context/window
  lifetime, fallback behavior, palette preservation, and presentation of
  software frames through GL after a context has been created.
- The merged 0.99 beta and earlier shareware behavior, save/demo handling,
  edition checks, and expanded regression suites. Preserve the explicit
  32-bit wraparound in inside() and the beta-specific hitscan conditions.
- The current SDL2 gamepad support and input defaults. These remain outside
  the source-movement scope but are part of the baseline test suite.

The module split remains as approved. This update adds preservation constraints
and verification coverage; it does not reopen those completed features.

## Module ownership

| File | Responsibility and source |
| --- | --- |
| engine.c | Core world rendering, geometry, collision, map loading/saving, and engine initialization. Retain the existing textual inclusion of polymost.c. |
| display.c | Engine-level view setup, screen clearing and pixel/text operations, brightness, palette loading/lookups, and fades moved from engine.c. |
| draw.c | Low-level C scanline, column, and slab drawing routines, renamed from a.c. |
| tiles.c | ART and voxel loading, tile allocation/copy/rotation, tile animation offsets, and sprite position/list management moved from engine.c. |
| filesystem.c | GRP initialization and lookup, file handles/seeking/reading, endian-aware reads, and LZW compression and compressed reads/writes moved from cache1d.c. |
| cache.c | Cache initialization, allocation, eviction, aging, and cache diagnostics moved from cache1d.c. |
| math.c | Existing compiled helpers from pragmas.c plus square-root/reciprocal helpers and tables, getangle, rotatepoint, krand, and their initialization moved from engine.c. |
| network.c | Existing multiplayer implementation, renamed from mmulti.c. |

The SDL window, surface, OpenGL context, input, timer, presentation, and
screenshot backend remains in sdl_driver.c. Separating that backend further
is outside this refactor. Live video changes and their queue remain in
source/game.c; the engine-free option helpers remain in source/videoopts.c
and videoopts.h. Buildengine continues compiling ../videoopts.c. The existing
display.h backend interface, including mode,
fade, supersampling, and filtering entry points, keeps its current ownership.
The new display.c contains engine-level operations extracted from engine.c.

World and masked-sprite rasterization, rotated-sprite
rendering and its permanent-sprite replay queue, and the textured map renderer
remain in engine.c because they share renderer-local state. In particular,
nextpage stays with that queue. Tiles owns sprite bookkeeping, not the tightly
coupled world-sprite rasterizer.

The math module must preserve the distinction between the older logic square
root and the renderer square root. Geometry continues to use the same helper
and BUILD_ENGINEREV branch as before. Existing inline fixed-point operations
in pragmas.h remain inline. In particular, preserve the 19960104 beta revision
and the pre-19960129 hitscan guards, as well as the later revision boundaries;
do not collapse early shareware variants into one arithmetic implementation.

## Boundaries and shared state

Move function bodies and their module-local state together. Preserve source
formatting, comments, licensing notices, preprocessor branches, initialization
order, allocation ownership, and call order. Changes to bodies are limited to
what is necessary to cross the new translation-unit boundaries.

Keep engine.h, build.h, engine_protos.h, display.h, a.h, cache1d.h, and
pragmas.h as the existing consumer-facing declaration surfaces. File renames
do not require callers to adopt new public function or header names. Update
source-location comments where the movement makes them misleading; retain
historical upstream filenames in provenance documentation.

Only engine.c defines ENGINE when including build.h, preserving the existing
single definitions of the public engine globals. Extracted files see extern
declarations. Retain existing externally visible symbols and types; do not
turn previously static implementation state into a new public API.

Use a small private engine_internal.h for the declarations required between
engine modules. Functions and state used by only one module remain static.
Private cross-module declarations must be limited to actual dependencies.
Do not copy the renderer's entire state into a shared header or duplicate
storage. Keep module-specific ART file handles and palette lookup scratch
buffers private to their owning modules.

Engine initialization delegates math, palette, and tile setup/teardown where
necessary while preserving the existing order and repeat-call behavior.
Polymost remains included by engine.c. Where extracted display operations
need Polymost-local functionality, provide a narrow private bridge in
engine.c rather than extracting Polymost or exporting its local variables.
All such declarations and calls are guarded correctly for POLYMOST-off builds.

Display extraction must account for both shader output representations.
clearview and clearallviews currently distinguish palette-index drawing from
the filtered colour path through polymost_shaderdraw(0) and glshcolor. Keep
that distinction behind a private bridge rather than exposing glshcolor.
Likewise, makepalookup must still invalidate the shader lookup upload through
the existing glshlookupdirty behavior. Preserve the side effects and ordering
of shader frame binding, 2D setup, frame-drawn notifications, and palette
invalidation when replacing direct access with private bridges.

Framebuffer resources, filter/supersampling state, and GL entry-point loading
remain with polymost.c, sdl_driver.c, glext.c, and glsubmit.c as they are today.
The extraction must preserve frame contents across renderer switches, resizes,
palette changes, still-screen fades, and screenshots. No new context or
resource lifecycle is introduced by the module split.

## Build and documentation changes

Update source/build/CMakeLists.txt so each new .c file compiles exactly once
in buildengine. Remove the replaced a.c, cache1d.c, mmulti.c, and pragmas.c
entries; do not retain duplicate compiled implementations or introduce .c
includes for the new modules. Preserve compiler definitions, platform source
selection, linking, and polymost.c's HEADER_FILE_ONLY treatment.
Keep ../videoopts.c and the existing GL support files in the target.

Update the README source-layout description and source/build/VENDORED.md
with the new ownership and the mapping from upstream filenames. Update other
active build references if any exist; preserve historical plans and notes.
No changes to game logic, audio, renderer features, networking protocols,
configuration, save/demo formats, or supported platforms are in scope.

## Verification and acceptance

1. Build the synchronized, otherwise unmodified worktree and run its existing
   CTest suite before moving code. Record the baseline commands, configuration,
   and results.
   Keep generated files in ignored build directories or a task-owned temp
   directory. Never run the game in the user's original data directories;
   point tests at read-only data and let their scratch directories hold writes.
2. Move one ownership group at a time and rebuild to catch missing declarations,
   duplicate definitions, and dependencies on static state. Compile the game,
   editor, and test executables, not only the engine library.
3. Run the complete applicable existing suites before and after the refactor
   using the version/edition matrix below. Also build and test 1.5 and 0.99
   with DUKE3D_POLYMOST=OFF, covering modern and earliest revision guards
   without GL. Use available local release data to enable rendering, boot,
   save, demo-sync, and multiplayer tests where supported.
4. Frame checksums and deterministic demo/gameplay results must match their
   baseline or existing pins. Exercise existing opt-in GL tests when a working
   graphical context is available, comparing the same configuration before and
   after. Report unavailable data, contexts, and platforms explicitly.
5. Review the diff with move detection and inspect changed function bodies.
   Confirm the new modules are separate compiled translation units, that no
   implementation is duplicated, and that public symbols/types and version
   guards are preserved. Run git diff --check.

### Version and edition matrix

| DUKE3D_VERSION | DUKE3D_SHAREWARE | Release-data option |
| --- | --- | --- |
| 1.5 | OFF | DUKE3D_GRP; DUKE3D_ATOMIC_GRP and DUKE3D_DOS_SAVES_DIR for DOS saves |
| 1.4 | OFF | DUKE3D_GRP for compatible render tests; DUKE3D_DOS_SAVES_DIR for the shared-layout hardening check |
| 1.3D | OFF | DUKE3D_V13D_DATA_DIR |
| 1.3D | ON | DUKE3D_V13D_SW_DATA_DIR |
| 1.1 | ON | DUKE3D_V11_SW_DATA_DIR |
| 1.0 | ON | DUKE3D_V10_SW_DATA_DIR |
| 0.99 | ON | DUKE3D_V099_SW_DATA_DIR |

These are the seven supported combinations in CMakeLists.txt and the current
CI matrix. Preserve configure-time rejection of unsupported combinations.
The protocol_27 peer represents a historical 1.5 shareware handshake, not an
additional supported build configuration. A 1.4 save hardening pass against
the shared layout is not evidence of an original 1.4 DOS capture.

### Regression coverage to retain

- Run the data-free suite in each configuration, including version macros,
  pragmas_semantics, engine_inside, engine_trace, vline_counts, lzw_roundtrip,
  savegame_codec, demo_writer, gamepad, input_defaults where registered,
  video_opts, build_cfg, editor_mode, and the SDL driver smoke tests. Do not
  limit validation to tests whose source files changed.
- With DUKE3D_GRP set to suitable data, retain classic E1L1 and 2D/HUD checksum
  checks, minihud checks, mask ordering, and Polymost software/solid,
  render-to-tile, and tilt checks. Keep the existing pins and tolerances.
- With suitable data and DUKE3D_GL_TESTS=ON, run render_gl, render_gl_video,
  render_gl_supersample, and render_gl_filter in a real GL context. Together
  they cover drawing tiers, palette/fades and captures, live renderer/mode
  switching, supersampling, filtering, and fallback behavior. Run these
  serially to avoid window/focus and output-file interference. An exit code
  of 77 is a skipped path, not a passing GL check.
- For 1.3D, 1.1, and 1.0, retain demo sync and refusal checks, DOS save loads,
  roundtrips and malformed-save checks where registered, edition rejection,
  supported multiplayer connection/co-op/video-cycle checks, and mismatched
  protocol peers. Use DUKE3D_NET_PEERS for available external builds; the
  built-in protocol peers do not require another release's game data.
- For 0.99, retain the CON runtime, actor/player/sector/game/menu, and
  real-renderer camera checks; demo format refusal/roundtrip and per-map
  DOS-reference sync; available DOS saves and cross-edition rejection; and
  the E1L5 episode-ending check enabled by its reference demo. The beta has
  no supported multiplayer test requirement. Preserve the existing optional
  DUKE3D_ROUTEBOT code and tests without requiring new reference recordings.

Match baseline and post-refactor test registration and data inputs. A missing
data path must not silently reduce the suite and appear as a successful
comparison. Record failures and skips separately. Linux and Windows remain
part of the existing CI coverage; local macOS results alone do not establish
cross-platform validation.

Existing tests are the primary evidence for unchanged behavior. Add a targeted
regression test only if a moved boundary has meaningful uncovered behavior;
do not add tests that merely assert filenames or repeat implementation.
Do not fix unrelated baseline failures as part of this refactor. Record them
separately and distinguish them from new failures. Do not claim validation
on platforms or renderer paths that were not exercised.

## Review and execution

Review this written spec before preparing the implementation plan. The plan
will specify the extraction order, precise shared declarations, and commands
for the available test matrix. Review that plan and choose its execution
method before source implementation begins.
