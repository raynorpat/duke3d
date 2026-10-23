# Runtime Duke release selection

Date: 2026-10-23
Status: Approved by the user, 2026-10-23

## Intent and decisions

Ship one Duke executable that selects the supported release's game logic,
engine behavior and file formats from the base GRP's size and CRC-32. Remove
version-specific build settings and preprocessor selection. Modified or unknown
GRPs require an explicit command-line version. Preserve existing release
compatibility rather than modernizing the historical game behavior.

User decisions:

- Use a shared runtime profile and ordinary runtime conditions.
- Unknown GRPs must require an override; there is no default release fallback.
- Detect shareware by checksum and disallow custom maps with shareware data.
- Include the investigation and fix for
  [GitHub issue #1](https://github.com/raynorpat/duke3d/issues/1), concerning
  Atomic data failing rendering checks pinned to 1.3D data.

The detailed policies labeled as proposed defaults below are the author's calls
for design approval, not additional decisions already made by the user.

## Scope and constraints

This is a project across the game, engine, build configuration and test harness.
Existing `DUKE3D_GAMEVER`, `BUILD_ENGINEREV`, `VOLUMEONE`, `VOLUMEALL` and
`PLUTOPAK` guards select much more than content: CON dialects, weapon numbers,
random-number behavior, structures, LZW codecs, save layouts and demo/network
formats. A startup detector alone cannot remove the separate builds.

Preserve all seven currently supported profiles and their established tests.
Keep the current C99, CMake and SDL2 foundation, existing resource lookup and
case-insensitive base filename handling. Non-version options, including
renderers, controls and the optional route bot, remain build options.

Non-goals: new release support (including Mac shareware), a launcher or file
picker, searching installed games, switching releases during a process, new
mod formats, cross-release saves/demos/network play, or reproducing an old
Build editor. Do not commit proprietary GRPs, scripts, saves or demos.

Failure conditions include changes to established demo synchronization or DOS
save compatibility; quietly guessing an unknown GRP's release; permitting
custom maps with shareware; and making issue #1 pass by removing assertions or
blindly replacing expected checksums.

## Approaches

1. **Shared runtime profile, selected once. Chosen by the user.** Convert the
   existing version branches to runtime conditions, share maximum-capacity
   in-memory structures, and keep release-specific encodings. This most closely
   follows the verified code, though the conversion touches many branches.
2. **Subsystem implementations selected through function dispatch.** Split
   gameplay, engine and codecs into separate implementations selected at
   startup. This reduces some conditional checks but introduces substantially
   more restructuring and potential duplicated code. Not selected.

Keeping per-release builds behind a launcher would retain the separate defines
the user wants removed, so it does not meet the goal.

## Profile and detection

Use one small dependency-free profile module that both game and engine targets
can link. The engine must not depend on game objects. Store immutable profile
descriptors and one selected profile; game and engine use that same selection.
Platform initialization may run before selection only if it does not consume
release-dependent values. Initialize all release-dependent state after selection
and before the banner, config/save scanning, engine math, scripts or networking.

| Override token | Game revision | Engine revision | Protocol byte | Edition |
|---|---:|---:|---:|---|
| `0.99` | 199601040 | 19960104 | 9 | shareware |
| `1.0` | 199601290 | 19960129 | 10 | shareware |
| `1.1` | 199602200 | 19960220 | 12 | shareware |
| `1.3d-shareware` | 199604190 | 19960419 | 14 | shareware |
| `1.3d` | 199604190 | 19960419 | 103 | full |
| `1.4` | 199610210 | 19960820 | 116 | full, Plutonium |
| `1.5` | 199610320 | 20000614 | 117 | full, Atomic |

Retain symbolic numeric release constants where they make comparisons readable;
these are labels, not compile-time version selectors. The profile owns version
and edition facts; derive straightforward comparisons instead of introducing
an extensive feature-flag or function-table framework.

Move the existing base-GRP fingerprints and CRC implementation out of
`game.c`. The seven supported size/CRC pairs remain unchanged. Calculate a full
CRC when reading an unknown-size GRP as well, so diagnostics can identify it.
Handle unreadable files, read errors and malformed archives as errors, not as
successful detection. File size alone never identifies a release.

Retain the known 1.5 Mac shareware fingerprint as recognized but unsupported.
It does not automatically become a supported eighth profile. As a proposed
default, it may use an explicit supported shareware override, with a diagnostic
that this forces compatibility rather than providing native Mac support.

## Startup and command line

Proposed CLI: `-gameversion TOKEN`, tokens above, matched case-insensitively.
For example, `duke3d -gameversion 1.3d` runs an unknown modified base GRP as
registered 1.3D. `1.3d-shareware` removes the ambiguity between its two editions;
0.99, 1.0 and 1.1 have only supported shareware profiles.

Perform an early, narrow parse before version-dependent legacy parsing. Help
must describe the option and work without a GRP. Missing, invalid or repeated
override arguments are errors naming valid choices. Consume the option/value
so old parsers cannot interpret them as maps or legacy switches.

Startup resolves the existing `duke3d.grp` base path, fingerprints it, selects
the effective profile, mounts it as the first archive and initializes release
defaults. Existing `/g` arguments remain add-on archives and never replace the
base fingerprint or trigger another profile selection. A new base path option
is outside scope.

Selection rules (proposed defaults):

- Supported fingerprint, no override: use its exact release and edition.
- Unknown fingerprint, no override: fail with path, size, CRC and an example
  override; do not start scripts, the engine or network sessions.
- Explicit override: select the requested supported profile and log both the
  detected identity (if any) and forced profile. Overrides may intentionally
  select different behavior for recognized data, but cannot select a full
  profile for a recognized shareware archive; reject that conflict.
- Unsupported recognized data without an override: explain the unsupported
  identity instead of silently mapping it to a supported release.
- Any selected shareware profile enforces shareware restrictions, even for an
  unknown/modified archive. Selection cannot change later through add-ons,
  scripts, a demo, a save, a network peer or config values.

Loose scripts retain existing lookup behavior; detecting a GRP does not prove
that loose CONs match it. Existing compiler diagnostics remain relevant.

## Runtime conversion

Replace release preprocessor branches with conditions on the selected profile.
Keep the branches' arithmetic, ordering and random-call sequences intact.
Where a branch previously selected an entire function, retain small explicit
release-specific functions and a narrow runtime choice rather than mixing
incompatible bodies mechanically. No parallel copies of entire subsystems.

Version-specific declarations must compile together: retain the union of player,
input and other required fields; allocate buffers/arrays to the largest supported
capacity. Separate allocated capacity from active limits such as script length,
sound count and weapon-record count. Replace version-dependent initializers
(episode/skill names, defaults and keyword tables) with explicit initialization
after selection. Resolve version-dependent tile/sound IDs, weapon numbering,
1.0 `rand` behavior and input layouts deliberately; they cannot remain selected
by header guards. Preserve unrelated platform and renderer preprocessor guards.

Select historical Build math, collision and LZW behavior using the profile's
engine revision. All old codec implementations must coexist; compile-time
function choices become runtime choices. Profile selection precedes any
revision-dependent precomputation. Avoid redundant selection inside pixel loops
where a condition can be placed around an existing larger operation.

Save and demo formats remain release-specific. Keep explicit DOS field codecs,
record sizes/counts, script limits, block boundaries and compression variants.
Increasing native capacities must not change bytes written by `sizeof`, loop
bounds or serialization helpers. Audit these callers rather than only changing
header constants. Keep protocol identity and existing cross-version refusal
rules. 0.99 remains supported to its existing single-player scope.

Do not switch profiles based on a save/demo header. Preserve existing behavior
for old formats that lack reliable version fields; retain available format
validation rather than claiming all cross-version files can be identified.
Diagnostic state dumps/tests that depend on native structure layouts must be
updated to compare meaningful fields or explicit encodings.

## Shareware map policy

Custom-map rejection applies to every selected shareware profile, including
0.99. Enforce it in game level selection/loading as well as menus and argument
parsing. Reject explicit `-map` and legacy bare-map requests with a clear message;
do not silently enter the requested map. Block user-map menu actions and
network/demo/save paths that attempt to select a custom-map slot or filename.
Validate episode and level selection against the profile's built-in campaign.

Proposed interpretation of the restriction: shareware campaign map names and
slots come from that release's built-in campaign allowlist, and map bytes must
come from the first/base GRP. Loose `E1L1.MAP` replacements, maps from later
`/g` archives, and CON redirects to other map names cannot bypass it. Preserve
ordinary resource overrides for non-map assets; a blanket ban on all add-ons
would exceed the request. Use the existing base-only map lookup mechanism
where practical (the current loader's byte-255 suffix selects archive zero),
or make that choice explicit at the loading boundary if that is simpler.

An unknown modified GRP forced to shareware can contain altered campaign maps
inside its base archive. This is not content authentication: its campaign
slots still load from that archive, but external/custom-map selection remains
disabled. No attempt is made to prove the provenance of arbitrary saved world
geometry. Full profiles retain custom-map support.

## Issue #1 and rendering references

The issue reports seven Atomic failures on macOS arm64, reproduced on unchanged
`d84c766`, before the later HUD/framing change. Current expectations include
pins explicitly documented for the 1.3D GRP. Treat the reported Atomic values
as observations, not approved new expectations.

Reproduce both data sets with the old pins, using clean output/cache settings.
Compare the relevant map, ART and palette inputs and captured output; determine
whether differences come from data, intentional engine revisions or a renderer
defect. Preserve existing references throughout the investigation. Fix a real
renderer defect if found; introduce separate expectations only where evidence
establishes legitimate differences, documenting why each family differs.

Runtime rendering test executables must select the same profile from their GRP
before engine initialization; they currently bypass `game.c`. Test expectations
must be selected from fingerprint identity, not a build option or filename.
Register distinct 1.3D and Atomic CTest cases in the same build whenever both
data sets are available. Keep user checksum overrides working, but prevent old
cached 1.3D defaults from silently becoming Atomic expectations. Unknown data
has no stock golden reference: rendering assertions require an explicitly
provided reference, otherwise report that edition as having no golden coverage.
Preserve existing platform caveats for floating-point Polymost checksums.

## Build, editor and tests

Remove `DUKE3D_VERSION` and `DUKE3D_SHAREWARE` as supported CMake options and
remove their compile definitions. Proposed migration behavior: reject explicit
legacy settings with instructions to remove them and use the runtime option;
include existing CMake caches in that diagnostic. Update README examples and
current test/build automation; historical design documents remain historical.

Proposed editor policy: explicitly initialize its shared engine to the current
default 1.5 revision, preserving the normal editor behavior. This task does not
introduce historical editor emulation or impose shareware game restrictions on
map authoring. Engine-only tests select their intended profile explicitly.

Replace macro/configuration tests with independent table expectations for all
seven runtime profiles. Adapt existing per-release tests to one build, using
their existing external data inputs and isolated run directories. Register all
applicable releases together instead of selecting tests by build version.
Keep tests without commercial data useful; mark unavailable data coverage
clearly rather than claiming it ran. Network peers can use one executable in
separate processes with different data/profiles.

Required verification:

1. Detection and CLI: all known fingerprints and metadata; streaming CRC;
   unknown data requiring an override; accepted overrides; invalid arguments;
   unreadable/malformed data; help without data; unsupported Mac identity;
   recognized-shareware/full-override conflict; add-ons not changing selection.
2. Shareware: all four shareware profiles reject custom-map entry paths;
   replacement loose/add-on campaign maps do not load; packaged campaign maps
   still work. Full profiles still accept custom maps.
3. Compatibility: existing CON compile/boot, DOS demo synchronization, save
   decoding/round trips/refusal and multiplayer checks across supported profiles,
   including engine/math and both LZW families. Retain independent DOS references.
4. Rendering: reproduce and resolve issue #1 with retained 1.3D coverage and
   independently justified Atomic expectations, including the seven failing
   cases, passing HUD cases and unaffected resolutions.
5. Integration: game, editor and test targets build once; no release-selection
   preprocessor guards or version CMake options remain; appropriate headless
   suite passes and editor/default rendering smoke checks pass.

## Builder discretion and delivery

The builder chooses module names, exact helper interfaces, capacity constants,
test organization and conversion order. Prefer direct conditions and existing
codecs. A single source of truth for runtime fingerprints may also provide
test-side metadata; independent expected values must still catch table errors.
Avoid unrelated cleanup, new dependencies and speculative abstractions.

Implement incrementally with focused checks, then run the combined release
matrix. Record any missing external fixtures or platform limitations. The design
must be approved before implementation; the next step after approval is a
written implementation plan.
