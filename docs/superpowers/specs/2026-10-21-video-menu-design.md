# VIDEO Menu: Live Video Settings — Design

Date: 2026-10-21
Status: Approved

## Intent

Phase 4 of the Polymost work (phases 1–3: `docs/superpowers/specs/2026-10-19-polymost-design.md`,
`2026-10-20-editor-polymost-design.md`, `2026-10-21-polymost-classic-shading-design.md`, all merged).
The game's OPTIONS → VIDEO page (added in phase 3 with contrast, saturation, display mode and vsync)
becomes the place to change video settings **live, mid-game, without a restart** *(user)*. Phase 3
kept one OpenGL context for the whole session, so switching renderer is a drawing change rather than
a window teardown.

## Goals

The VIDEO page offers these rows *(user's list; layout and wording my call)*:

| Row | Choices | When it applies |
|---|---|---|
| RENDERER | Classic, Polymost solid, Polymost software, OpenGL (0–3) | live; OpenGL greyed when GL is unavailable (and with `DUKE3D_POLYMOST=OFF` only Classic) |
| DISPLAY | windowed, borderless, fullscreen | live; "exclusive" from phase 3 is renamed **fullscreen** in the menu *(user)*; config value names stay compatible |
| RESOLUTION | windowed: window sizes; fullscreen: the display modes SDL reports for the main display, built at runtime *(user)* | windowed → sets the window size; fullscreen → switches the display mode; borderless → fills the main screen at its current resolution and the row greys out *(user)* |
| VSYNC | on, off | live |
| BRIGHTNESS | today's 8 levels (`ud.brightness`, F11, `ScreenGamma`) | live; moved here from OPTIONS *(user agreed)* |
| CONTRAST, SATURATION | phase 3 sliders | shader path, as now |
| SUPERSAMPLING | off, 2k, 4k, 8k | renderer 3 draws at 2560×1440 / 3840×2160 / 7680×4320 (aspect matched to the window, my call) and scales down to the window *(user: "render high and scale down")*; needs an FBO, so the GL 2.1 shader path only (greyed otherwise) *(my call)* |
| TEXTURE FILTERING | nearest, bilinear, trilinear (mipmapped) *(my call on the three choices)* | OpenGL only (greyed in 0–2). On the 1.1/1.5 paths it sets their texture filter. On the shader path, filtering other than nearest switches it to colour drawing *(user's choice)*: distance shading through the shade tables stays exact; see-through blending and fades become approximations while filtering is on; nearest keeps today's exact palette-index path |

Other behaviour *(my calls, user agreed with the playback)*:
- Every change applies immediately and is saved to `duke3d.cfg` when leaving the VIDEO page (and on exit). New keys: `Supersampling` (0/2k/4k/8k as 0,1,2,3 or a string — builder's call, documented), `GLTextureFilter`. The existing `Renderer`, `ScreenWidth`/`ScreenHeight`, `DisplayMode`, `VSync`, `ScreenGamma`, `GLContrast`/`GLSaturation` keys are what the page edits.
- The editor has no menu; it reads `supersampling` and `gltexturefilter` from `build.cfg` (same meanings).
- `GLDrawPath` and `GLInternalWidth`/`GLInternalHeight` stay config-only (page space). Internal size and supersampling are mutually exclusive: if both are set, internal size wins and supersampling greys out *(my call)*.
- OPTIONS loses the BRIGHTNESS row (moved to VIDEO); F11 still steps brightness.
- Resolution list: windowed sizes are the display modes that fit inside the desktop plus the current size; duplicates removed; sorted; the current one selected. Changing it is applied on confirm (Enter), not while scrolling *(my call, to avoid mode flicker)*.
- Applying a renderer or mode change during demo playback or multiplayer is allowed; it must not affect game state or sync.
- **Resolution in renderers 0–2** *(user)*: the chosen resolution is the size the game draws its 8-bit picture at (`ScreenWidth`/`ScreenHeight`), up to the engine's 1600×1200 cap; choices above the cap draw at the largest fitting size and are scaled up to fill. The list also offers the classic sizes (320×200, 640×400, 640×480 …). In borderless they draw at the desktop size up to the cap. Renderer 3 always draws at the window's real size (or its internal size / supersampling).
- **First switch to OpenGL** *(my call)*: a run that started in renderers 0–2 has no GL context. The first time OPENGL is picked, the window is rebuilt once with GL (phase 3 then keeps that context for the rest of the run). OPENGL shows as available until an attempt fails; then it greys out for the run and the previous renderer is restored with a message. Classic-only runs never create a GL context.
- **Supersampling sizes** *(my call)*: 2k/4k/8k are fixed heights (1440/2160/4320) with the window's aspect. A choice whose size is not larger than the window greys out (so supersampling only ever scales down). Clamped to the GPU's maximum framebuffer size with a message.
- **Run-only overrides** *(my call)*: the page shows the values in effect (including `-renderer`, Alt+Enter, `BUILD_WINDOWED`, a fullscreen→borderless fallback). Leaving the page saves only rows the user changed; a changed row becomes the saved setting. Untouched rows keep the saved value, so a one-run override isn't written.

## Non-goals

- Changing the classic renderer's look.
- An in-editor menu.
- New renderers or draw paths.

## Anti-goals

- Demo sync or multiplayer sync affected by any VIDEO change.
- A change that crashes, leaves a black screen, or loses the window (every change must be reversible from the menu).
- Classic renderer 0 output changed (pins byte-identical); phase 3's exact shader path unchanged when filtering is nearest and supersampling off.

## Constraints

- Same build matrix (1.3D, 1.3D shareware, 1.4, 1.5, 1.5 POLYMOST=OFF); 1.3D and 1.5 menus differ (`menues.c` version #ifs) — the page works in all.
- The menu is drawn at 320×200 scale; rows must fit (the phase 3 VIDEO page starts at y=50, 16 apart; nine rows need a tighter layout or two columns — builder's call, shown to the user as a capture).
- 64-bit conventions; AGENTS.md commits; never run in `~/Downloads/DUKE3D`; never touch the user's real `~/Library` config.

## Success check

*(My guess; the user's check as in earlier phases.)* The user opens OPTIONS → VIDEO mid-game and in the attract demo, changes each row, sees it take effect immediately without a restart or crash, quits, and finds the values saved. Automated tests cover config round trips, the resolution list builder (from a fake mode list), the supersampling/filtering selection logic, and render_gl checks that supersampling and filtering change the frame as intended and that nearest/off leaves the shader path byte-identical.

## Left to the builder

- Menu layout to fit the rows; slider vs. cycling choices per row.
- How supersampling's FBO size follows the window aspect and the GPU's maximum texture/renderbuffer size (8k may exceed it: clamp with a message).
- The colour-drawing variant of the shader path when filtering is on, and how translucency and fades are approximated there.
