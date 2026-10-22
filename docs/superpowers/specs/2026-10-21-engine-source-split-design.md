# Build engine source split

Date: 2026-10-21
Status: Proposed for written-spec review; module split approved in conversation

## Intent and scope

Reorganize the existing Build engine into focused translation units, following
the user's reference diagram. The user approved the module split below.
This is a source-movement refactor: game and editor behavior, public function
signatures, file formats, arithmetic, and version-specific logic must remain
unchanged. There is no target percentage for the size of engine.c.

Work in the managed worktree at
`/Users/raynorpat/.codex/worktrees/e4fe/duke3d`, created from `v099` at
`2af4260f27424b656a8c44f7e5cadee5b19ab556`. Leave the original checkout and its
untracked files alone. The worktree initially has detached HEAD.

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
is outside this refactor. World and masked-sprite rasterization, rotated-sprite
rendering and its permanent-sprite replay queue, and the textured map renderer
remain in engine.c because they share renderer-local state. In particular,
nextpage stays with that queue. Tiles owns sprite bookkeeping, not the tightly
coupled world-sprite rasterizer.

The math module must preserve the distinction between the older logic square
root and the renderer square root. Geometry continues to use the same helper
and BUILD_ENGINEREV branch as before. Existing inline fixed-point operations
in pragmas.h remain inline.

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

## Build and documentation changes

Update source/build/CMakeLists.txt so each new .c file compiles exactly once
in buildengine. Remove the replaced a.c, cache1d.c, mmulti.c, and pragmas.c
entries; do not retain duplicate compiled implementations or introduce .c
includes for the new modules. Preserve compiler definitions, platform source
selection, linking, and polymost.c's HEADER_FILE_ONLY treatment.

Update the README source-layout description and source/build/VENDORED.md
with the new ownership and the mapping from upstream filenames. Update other
active build references if any exist; preserve historical plans and notes.
No changes to game logic, audio, renderer features, networking protocols,
configuration, save/demo formats, or supported platforms are in scope.

## Verification and acceptance

1. Build the unmodified worktree and run its existing CTest suite before
   moving code. Record the baseline commands, configuration, and results.
   Keep generated files in ignored build directories or a task-owned temp
   directory. Never run the game in the user's original data directories;
   point tests at read-only data and let their scratch directories hold writes.
2. Move one ownership group at a time and rebuild to catch missing declarations,
   duplicate definitions, and dependencies on static state. Compile the game,
   editor, and test executables, not only the engine library.
3. Run the complete applicable existing suites after the refactor for 1.5,
   1.4, 1.3D registered/shareware, 1.1 shareware, and 1.0 shareware. Also build
   and test 1.5 with DUKE3D_POLYMOST=OFF. Use available local release data to
   enable rendering, boot, save, demo-sync, and multiplayer tests where supported.
4. Frame checksums and deterministic demo/gameplay results must match their
   baseline or existing pins. Exercise existing opt-in GL tests when a working
   graphical context is available, comparing the same configuration before and
   after. Report unavailable data, contexts, and platforms explicitly.
5. Review the diff with move detection and inspect changed function bodies.
   Confirm the new modules are separate compiled translation units, that no
   implementation is duplicated, and that public symbols/types and version
   guards are preserved. Run git diff --check.

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
