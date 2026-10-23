# Megaton menus on RmlUi

Date: 2026-10-23
Status: Draft for user approval. The user selected the faithful RmlUi port;
the detailed policies below remain proposals until this document is approved.

Builder review: completed. The reviewer identified only the proposed GL-only
boundary as a decision requiring user approval. Reference asset availability
remains an already-pending user question.

## Intent and agreed direction

Bring Duke Nukem 3D: Megaton Edition's menu UI into this port, preserving
most of its appearance and behavior and removing Steam features. The user
selected a faithful port of the menu layer to RmlUi over a fresh rewrite.
This is a project spanning UI resources, a C++ adapter, game actions, SDL
input, and final presentation. Do not import the Megaton engine or game fork.

Preserve the menu backgrounds and logos, fonts, animated selection cursors,
highlighting and transitions, sound cues, page navigation and back behavior,
mouse interaction, keyboard/controller operation, option cycling, sliders,
binding capture, confirmations, and save/load previews where the underlying
game supports them. Keep new-game/episode/difficulty selection, in-game
pause/resume/quit, local saves and maps, and game/audio/video/input settings.

The exact reference resource set is unresolved: the public repository omits
artwork/fonts and some RML documents requested by its loader. The user has
been asked whether a local Megaton installation is available. Do not claim
visual or functional parity until the required reference material is available.

## Approaches

1. **Faithful RmlUi port (selected by the user).** Reuse the RML/RCSS and
   adapt the custom navigation and animation behavior. Replace Megaton game
   bindings with a narrow interface to this port. This preserves details
   while containing the dependency changes.
2. **Fresh RmlUi implementation using Megaton as a visual reference.**
   Offers cleaner new UI code but requires re-creating interactions and
   increases the risk of behavioral differences. Not selected.

Reviving the bundled libRocket stack was rejected in the recommendation:
RmlUi continues the same document/style model and has maintained backends.
An unrelated UI toolkit would require replacing both markup and behavior.

## Current implementation and evidence

This checkout builds C99 code with CMake and SDL2. `source/menues.c` combines
page state, drawing, input and game actions. `cmenu()` is also reached from
gameplay hotkeys and startup paths; migration must include those entry points.
`source/gamepad.c` currently synthesizes keyboard-style menu navigation.
SDL events are drained centrally in `source/build/sdl_main.c`.

Presentation lives in `source/build/sdl_window.c`. Renderers 0-2 use an
indexed framebuffer, presented through SDL_Renderer until a GL context has
been created; afterward they use the retained GL context. Renderer 3 has
legacy GL and palette/shader presentation paths. Some paths swap inside
their own functions, so drawing UI only from `menus()` would be insufficient.

Upstream inspected: `TermiT/duke3d-megaton` master at
`6dca4d5523e84d59e49008d1007706b44214df98`. Relevant resources and code:

- [RML/RCSS resources](https://github.com/TermiT/duke3d-megaton/tree/6dca4d5523e84d59e49008d1007706b44214df98/gameroot/data)
- [Navigation plugin](https://github.com/TermiT/duke3d-megaton/blob/6dca4d5523e84d59e49008d1007706b44214df98/code/rocket/RocketMenuPlugin.cpp)
- [Game bindings and resource loader](https://github.com/TermiT/duke3d-megaton/blob/6dca4d5523e84d59e49008d1007706b44214df98/code/gui_private.cpp)
- [GL-specific animated cursor](https://github.com/TermiT/duke3d-megaton/blob/6dca4d5523e84d59e49008d1007706b44214df98/code/rocket/FrameAnimationDecorator.cpp)
- [RmlUi 6.3 SDL renderer](https://github.com/mikke89/RmlUi/blob/6.3/Backends/RmlUi_Renderer_SDL.cpp)
- [RmlUi integration](https://mikke89.github.io/RmlUiDoc/pages/cpp_manual/integrating.html)

The 6.3 SDL2 renderer explicitly requires SDL's OpenGL backend and uses
premultiplied-alpha custom blending. It is not evidence of a portable
software fallback. Megaton's animated cursor also issues GL calls directly.

## Proposed rendering boundary

**Author's scope choice for approval:** the new UI requires compatible GL
presentation. It works with game renderers 0-3; software game rendering and
software UI presentation are separate concerns. True no-GL runs, including
`DUKE3D_POLYMOST=OFF`, keep the existing classic menus. A new CPU UI renderer
is outside this migration. This is a limitation, not full new-menu coverage
on every existing presentation backend.

When Megaton menus are selected, request GL presentation without changing the
selected game renderer or saving a different renderer setting. Reuse the
existing window/context lifetime code; factor only the minimum needed to
present indexed frames through GL without entering renderer 3. Failure to
create the UI/context leaves a usable classic menu and reports the reason.
Do not require GL or create a context for classic-menu-only runs.

Use RmlUi's legacy GL renderer as the starting point, adapted to the engine's
existing context. Do not use a sample backend that creates its own window,
event loop or swap. Do not raise the game's GL requirement silently: establish
the UI backend's actual minimum during implementation, and retain classic
menus on contexts that cannot support it. No GL3-only effects are required.

Add a narrow presentation callback from the engine to the game-owned UI.
The engine/editor must not link back to game or UI objects. Composite the UI
once after indexed-frame conversion or renderer 3's palette/FBO resolve and
before the final display fade and buffer swap. Keep native UI resolution
independent of internal game resolution and supersampling. Scope/restore GL
state so the following frame's palette shaders, buffers, clipping, textures,
matrices, viewport and blend state remain valid. Request/handle stencil
capability if required by the chosen backend; simple pages must not assume
an available stencil buffer without checking it.

Resize, fullscreen and renderer transitions refresh UI viewport/resources
without losing the current page, selection, modal or unconfirmed edit. Use
the retained context; do not transplant Megaton's texture grab/readback scheme
unless a real context destruction path needs it. A UI initialization or
resource failure falls back before it can consume menu input or hide the
classic menu. Clear active UI/input state when changing menu implementations.

## UI and game boundary

Keep the game C99. Enable C++17 only when building the optional UI target;
expose a small `extern "C"` interface for lifecycle, events, update/render,
page opening, and game commands. Keep RmlUi types out of game/engine headers.
Use owned C++ state rather than exposing native serialized game structures.

Port the useful navigation plugin and animation behavior. Update namespaces,
element/document ownership, plugin callbacks, font effects, decorator APIs,
texture handling, and premultiplied alpha against the pinned RmlUi release.
Do not treat it as a search-and-replace port. Implement the animated cursor
through RmlUi geometry/textures rather than raw GL inside a decorator. Keep
animation timing independent of simulation ticks and network/demo clocks.

Replace `gui_private.cpp`/`dnAPI` integration with focused bindings to existing
game actions. Where `menues.c` contains the only implementation of an action,
extract just that action for both menu implementations rather than invoking
classic input/drawing code behind the new UI. Keep one authority for config
and validation; presentation code must not independently reproduce rules.

Map menu entry points, including Escape, quicksave/load, load/save hotkeys,
quit confirmations, title/attract mode and in-game pages. A migrated page
must handle its full flow, including cancellation and errors. During staged
development, incomplete flows stay on the classic implementation as a whole;
do not accept input in both menu systems or strand a player between them.
Inventory every reachable classic page before switching the final default.

## Content and behavioral mapping

| Area | Result |
| --- | --- |
| Main/in-game/episode/skill/confirmation pages | Preserve Megaton presentation and interaction; use this port's game actions and release-appropriate content. |
| Save/load | Existing local slots, names, previews, DOS-compatible codecs, quicksave/load and error behavior; no cloud APIs or Megaton save format. |
| Local maps | Existing four-location scanner, duplicate source labels, path-length checks and difficulty/back flow; remove Workshop entries/downloads. |
| Video | Megaton styling with this port's live renderer/display/resolution/vsync/brightness/contrast/saturation/supersampling/filtering behavior and availability rules. Retain fades and changed-row persistence; do not restore Megaton's obsolete Apply workflow. |
| Audio/game/input | Keep this port's supported settings, ranges, defaults and bindings; preserve Megaton control interactions where compatible. Add rows/pages in the same style when this port has additional settings. |
| Help/credits | Retain applicable content and attribution; remove Steam-specific instructions and service commands. |
| Multiplayer | Preserve this port's existing non-Steam setup and connection path in the new presentation. Remove Steam lobby browsing, matchmaking, friends, invites, lobby chat and Workshop map exchange; do not invent a replacement online service. |

Removing Steam must remove service dependencies and actions, not leave dead
buttons, disabled Steam placeholders, polling managers or SDK stubs. Community
Hub, Workshop, cloud saves, Steam identity/avatar access, lobby notifications
and Steam-specific launch arguments are not part of the port.

Follow the selected game's episode/skill availability and shareware
restrictions, including custom-map restrictions. Do not add unsupported
expansion/gameplay support merely because Megaton has themed menu variants.
Supported installed add-ons may use their available theme; unsupported
themes are not exposed as playable campaigns.

## Layout and input

Use Megaton's 1920x1080 virtual layout as the fidelity reference. Uniformly
fit it within the drawable viewport with centered unused space rather than
cropping controls at 4:3 or stretching typography on ultrawide displays. This
aspect policy is an author's proposal. Use the same transform in reverse for
mouse hit testing, accounting for SDL window coordinates versus HiDPI drawable
pixels. Keep scissor rectangles and text clipping consistent with that fit.

Preserve selection memory, directional repeat, A/Enter accept, B/Escape back,
mouse hover/click/wheel and binding capture. SDL text input supplies editable
text; physical key events supply bindings. Suppress the event opening or
closing a menu so it cannot also activate a row or fire in resumed gameplay.
Choose one controller-navigation path: adapt the existing synthesized menu
actions or replace them for this UI, but never also inject raw navigation
events. Preserve device hotplug, deadzones and existing gameplay mappings.
Release capture and clear pending input/repeat on focus loss, menu transitions,
disconnects and capture cancellation. Keep frame-rate input improvements intact.

## Resources, build and availability

Add opt-in `DUKE3D_RMLUI`, default OFF during migration. OFF builds keep the
current dependency/build behavior, including the C-only toolchain. ON adds
the C++ adapter, pinned RmlUi 6.3 and its font dependency. Fetch/build these
statically through CMake when needed, with fixed source hashes; do not install
machine-global packages or enable sample/Lua/Python/SVG/Lottie dependencies.
Avoid passing C-specific warning flags to the new C++ sources.

Proposed runtime selection: one `MenuStyle` config value, `classic` or
`megaton`; missing/unknown values choose Megaton in UI-enabled builds and
classic otherwise. Explicit classic selection must not initialize RmlUi or
GL. An unavailable Megaton UI falls back to classic with a diagnostic and
does not overwrite the user's saved preference. Keep classic accessible via
config throughout the migration. Changing style live is outside scope.

Keep public, adapted RML/RCSS separate from private Megaton artwork/fonts.
Record upstream attribution and inspect licensing for each imported source
and resource; repository visibility alone is not a grant for its art/font
files. Do not commit commercial installation files. Public menu files live
in a dedicated UI resource directory; `assets/` references resolve against
one explicit external, read-only Megaton asset directory. The exact directory
mapping is to be set after inspecting the reference installation. Do not
search the user's disk or alter their installation to assemble resources.

Validate the required resource manifest before activating the UI. Missing
public layouts are reconstructed only from an available reference and marked
as such; missing reference material remains an explicit fidelity blocker.
Do not label invented replacement artwork as a faithful port. Tests can use
small redistributable fixtures, but visual parity requires the real assets.
CLI and any macOS bundle must locate public UI resources independently of
the game-data working directory. External private assets stay external.

## Other accepted work and non-goals

The repository has approved, separate designs for runtime release selection
and a macOS launcher/data directory. They are not implemented by this task.
Use whichever release/path APIs exist when implementation starts; preserve
the same release/edition policies. If that work lands first, test profiles
instead of per-release executables. Do not create a second release detector,
data picker, installation manager or launcher preference store.

Do not change gameplay, demo/network synchronization, save formats, profile
locations, Build editor UI, game renderers or GL draw-path selection as an
incidental effect. A modern UI does not change shareware/content permissions.
No Steam integration, replacement matchmaking, new expansion support, CPU UI
renderer, new game settings or wholesale menu/game refactor.

## Verification and delivery boundary

Start from the available build/test baseline. Verify the UI-disabled build
and classic behavior remain intact. For the enabled path, test commands,
page routing/back transitions, unavailable options, text/binding capture,
controller repeat/hotplug and input handoff using independent expected
behavior. Exercise save/load and local-map flows through the existing codecs
and scanner, including missing files and canceled operations.

Render and inspect the menu at 4:3, 16:9, ultrawide and HiDPI dimensions;
compare reference appearance, cursor animation, sounds and navigation. Check
title and in-game pages over renderers 0-3, and renderer 3's available draw
paths, supersampling/internal sizes and display changes. Check final fades,
screenshots, state restoration and no duplicate presentation. Inject missing
assets/font/layout and GL startup failure; verify a usable classic fallback.
True no-GL builds/runs must continue to operate with classic menus.

Build Windows/macOS/Linux where available, exercise all supported release/
edition policies, and run the affected existing input, video, map, save,
demo and network tests. Use temporary profiles, never the user's live config.
Report hardware/platform/reference checks that cannot actually be performed.

Deliver in coherent stages: reference/resource inventory; one complete
navigation/rendering slice; local game/settings bindings; remaining reachable
flows and Steam removal audit; visual and compatibility verification. These
are design boundaries, not an approved implementation plan. Keep the UI
opt-in until the parity checklist is complete and the user accepts it.

## Decisions still needing user input

- Availability/path of a Megaton installation containing the missing reference
  artwork, font and layouts. Resource fidelity work depends on this answer.
- Approval of the proposed GL-only new UI with classic-menu fallback for
  true no-GL operation, as part of approving this document.

The builder chooses filenames, the smallest adapter signatures, dependency
fetch details, exact layout adaptations for extra settings, and test fixture
structure within these boundaries. Any additional feature, loss of an existing
menu function, or requirement to redistribute private assets returns for design
discussion rather than becoming an implicit implementation choice.
