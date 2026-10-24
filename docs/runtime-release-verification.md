# Runtime release verification

Date: 2026-10-23. Platform: macOS arm64, AppleClang 21, installed SDL.
This report covers the unified runtime build against the unchanged `58bdcc3`
baselines, not unrun Windows/Linux jobs.

| Profile | Original data coverage |
|---|---|
| 0.99 shareware | Matching CONs, campaign boot, six original-reference demo checks, DOS saves, shareware map policy and episode ending |
| 1.0 shareware | Matching CONs, campaign boot, original demos and DOS saves, protocol/map refusal; existing matching-peer sync failure |
| 1.1 shareware | Same coverage and existing matching-peer sync failure |
| 1.3D shareware | Matching CONs, boot, original demos/saves, matching/refusing peers and base-only maps |
| 1.3D full | Matching CONs, three-episode boot, original demos/saves, matching/refusing peers and rendering references |
| 1.4 | Data-free profile, engine, CON, codec, input, gameplay and demo cases; no original GRP supplied |
| 1.5 Atomic | Boot including episode four, independent DOS save decoding/round trips, matching/refusing peers and rendering references |

The same executable passes the preserved 19 original demo/switch checks and
70 full-game DOS save checks; one codec also accepts all 74 supplied original
saves. Independent DOS decoders and original state references were not regenerated.
Shareware replacement fixtures distinguish base, loose and add-on map bytes;
all four shareware profiles load the base map while full profiles keep overrides.
CLI tests cover modified-GRP overrides, known-shareware conflicts, malformed
archives, help, add-ons and macOS bundle path handling.

## Baseline failures and unavailable coverage

`v099_render_camera` still reports the two current moving/remote horizon
mismatches. `net_connect_v10_sw`, `net_connect_v10_sw_coop`,
`net_connect_v11_sw`, and `net_connect_v11_sw_coop` still connect then report
out of sync. All five fail the unchanged baselines too. They remain ordinary
failing CTest cases, not hidden expected failures. Socket tests need permission
to bind localhost; sandbox bind failures are environmental.

Issue #1's seven Atomic rendering failures were resolved by fingerprint-specific
references after controlled map/ART substitution proved the map-data cause.
The [investigation](render-reference-investigation.md) retains all measurements.
Polymost checksums are floating-point and optional.

No original 1.4 GRP, native Windows/Linux execution or beta multiplayer validation
is claimed. Early unversioned files cannot always identify their source release.

## Final verification runs

- Fresh no-data configure/build: 138/138 CTest cases pass.
- Fresh `/tmp/duke-runtime-final` build with all six supplied GRPs, DOS saves,
  route bot and GL tests enabled: 363/368 pass in 180.35 seconds. The five
  failures are exactly the established cases above; no new failures or skips.
- All seven real-window GL cases pass, including immediate/VBO/shader paths,
  live video switching, supersampling and texture filtering.
- Both beta route recording checks, editor/default-renderer smoke checks,
  matching 1.3D/Atomic peers and all three real cross-release peer pairs pass.
- Clean rendering cache: 35/35 selected rendering/HUD/policy cases pass;
  combined fixture rendering and reused-cache regressions: 39/39 pass.

The final independent review is recorded after its findings are resolved.
