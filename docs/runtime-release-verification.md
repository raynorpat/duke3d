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

- Fresh no-data configure/build after the review fix: 139/139 CTest cases pass.
- Fresh `/tmp/duke-runtime-final` build with all six supplied GRPs, DOS saves,
  route bot and GL tests enabled: 363/368 pass in 180.35 seconds. The five
  failures are exactly the established cases above; no new failures or skips.
- All seven real-window GL cases pass, including immediate/VBO/shader paths,
  live video switching, supersampling and texture filtering.
- Both beta route recording checks, editor/default-renderer smoke checks,
  matching 1.3D/Atomic peers and all three real cross-release peer pairs pass.
- Clean rendering cache: 35/35 selected rendering/HUD/policy cases pass;
  combined fixture rendering and reused-cache regressions: 39/39 pass.

## Final independent review

A fresh gpt-6-astra reviewer examined the whole branch in subsystem passes and
independently reran 41 focused checks, all passing. It found one Important issue:
the new GRP inspector accepted extents that overflow the legacy signed-32-bit
mounter. A complete sparse 2 GiB member archive reproduced the acceptance and
negative member length. The inspector now rejects directory-plus-member extents
above `INT32_MAX` before mounting, including when a version is forced.

`grp_extent_limits` failed before the fix and passes after it. Its member,
cumulative-size, directory-prefix and directory-count cases exercise both the
identity tool and real forced startup. The post-fix no-data suite passes 139/139;
all 364 supplied-data/routebot/GL acceptance cases pass in 132.50 seconds after
excluding only the same five baseline failures listed above. Those failures stay
visible in the unfiltered run and are not marked expected failures in CTest.
No Critical/Important findings remain; no deferred minors.

## Implementation rulings

The following decisions are retained in execution order. Interim demo framing
and startup fallback staging were completed during the later conversion tasks.

- Ruling: Preserve and report baseline camera/network failures instead of expanding this conversion into unrelated fixes — independently reproduced before changes — risk: existing defects remain visible in final matrix.
- Task 6: Ruling: defer end-to-end DOS demo runs until Task 8 converts startup/demo framing — gameplay cases run now, but the temporary build profile still frames files as modern — an integration regression could remain until that gate.
- Task 7: Ruling: save-name scanning now uses the existing early layout validator for unversioned saves — interpreting their name block as BYTEVERSION hid valid early saves — scanning up to ten early files does more decoding.
- Task 8: Ruling: enable explicit -gameversion selection before automatic detection — full-game runtime demo/network verification needs a selected profile in this task — the temporary build-default fallback remains until Task 9 and must not ship.
- Task 10: Ruling: task completion gate excludes the five independently established beta-camera/early-network baseline failures — whole unfiltered run remains recorded and failures stay visible — cost: this branch does not repair those existing defects.
- Final: Ruling: reviewer set aside baseline camera/network repairs — preserve the five visible independently reproduced failures, as scoped in the initial ruling — cost: existing defects remain.
- Final: Ruling: reviewer set aside original 1.4-data and native Windows/Linux execution — fixtures/platforms unavailable, so report them as unverified while retaining their test registrations — cost: those executions remain uncertified locally.
- Final: Ruling: reviewer set aside general legacy resource-loader hardening beyond base inspection — fix the newly required base boundary only; broad loader modernization exceeds this conversion — cost: pre-existing malformed add-on/resource behavior remains.
- Final: Ruling: reviewer set aside authenticating saved geometry and modified maps embedded in forced shareware bases — the approved policy controls selection, not provenance — cost: embedded modifications cannot be detected by that policy.
