# TODO

Follow-ups from the 1.0/1.1 variants and the 0.99 beta, reviewed on 2026-10-22
on `todo-followups`. Checked entries include fixes, verified existing behavior,
and the explicitly documented decisions below. Original DOS data stays outside
the repository. Historical row IDs refer to the variant plans under
`docs/superpowers/plans/`.

Final verification: 284 tests passed across all seven versions (24/24/45/44/50/50/47),
plus four optional routebot checks. Independent review found no remaining blocking
issues. The one unchecked item below requires an unavailable original executable.

## Code

- [x] **Signed overflow in `inside()`:** coordinate differences, products and
  determinants use unsigned 32-bit arithmetic while preserving all three historical
  algorithms. Existing overflow cases pass the signed-overflow sanitizer.
- [x] **Death-path `TRAND` order:** draw the side bit before the angle byte in
  separate statements, including the beta form. Fixed-seed checks cover the result
  and final RNG state.
- [x] **1.3D demo headers:** reject files shorter than 24 bytes and initialize
  header locals. Every truncated length is tested in both 1.3D editions.
- [x] **Separate `checkhitsprite` / `cheatkeys` copies:** reviewed and retained.
  Their case sets, flame reactions, damage handling and input paths differ
  throughout; folding them would add substantial conditional code without fixing
  behavior. The existing comments explain the separation.
- [x] **1.0 `checkweapons`:** preprocessor branches exclude the unreachable 1.1
  body rather than relying on an early return.
- [x] **`move` flag 2048:** the negative-owner guard now explicitly notes its
  departure from the 1.1 executable's invalid sprite read.
- [x] **1.0/1.1 prediction:** water and shrunk friction match `processinput`.
  Regression cases compare predicted and actual velocities for all four combinations.
- [x] **1.0 `debugprint`:** all game-layer C-library random calls share the
  measured Watcom generator and initial seed. Native networking retains its own
  host generator. Literal sequence checks cover both the alias and direct helper.
- [x] **1.0 demo `avel`:** the existing conversion comment documents the original
  input clamp to ±108 and why signed-byte storage is sufficient. Valid DOS demo
  behavior is preserved.
- [x] **Oversize CON saves:** old shareware builds warn when the DOS script limit
  truncates a modified CON beyond 10240 longs.
- [x] **Unused players:** loading an eight-player save clears `ps[]` / `po[]`
  entries 8–15.
- [x] **1.0 writer heuristic:** reviewed; retain the documented limitation by
  explicit user decision. A numeric actor value matching a script offset cannot
  be distinguished without adding runtime assignment-type tracking. Native reload
  reverses the heuristic; DOS compatibility for those ambiguous mod values remains
  limited.
- [x] **`sg_playerblocksize`:** excluded from builds where it is unused.
- [x] **Save loader hardening:** preflight compressed block boundaries and validate
  stored script, actor and animation offsets before restoring game state. Mutation
  fixtures cover truncation, invalid offsets and legal boundary values.
- [ ] **Original DOS 1.4 wall animations:** the executable is unavailable, so its
  wall-to-sector address delta remains unverified. Native 1.4 wall offsets and
  sector animations are supported; unmapped DOS wall offsets are refused safely.
  Verify the original delta when a 1.4 executable becomes available. The measured
  deltas for the other releases are supported.
- [x] **Quit screens before video setup:** skip them unless `qsetmode == 200`.
  The regression previously waited for input before a framebuffer existed.
- [x] **1.0 `ps.auto_run`:** intentionally preserved across new games, matching DOS;
  a regression now locks down this local-input behavior.

## Tests

- [x] **G10-L14 / G11-A35:** committed direct scenarios use cited DOS/disassembly
  literals. The old scratch captures were unavailable; expectations were not
  regenerated from native output.
- [x] **Burning and space suit:** scenarios exercise flame hits, water removal,
  burn timing/RNG, suit activation/depletion, space damage/death, gravity and jumping.
- [x] **Armor and save names:** new original-DOS `e1armor` references cover armor
  100 and nonempty names in 0.99, 1.0 and 1.1, with round-trip comparisons.
- [x] **Positive sync evidence:** both connecting peers must report that an actual
  remote/local sync comparison occurred.
- [x] **1.1 boot:** the private test copy reports a completed gameplay tic with the
  expected live player. The old alive-without-marker behavior fails the assertion.
- [x] **Edition mismatch:** verified the existing registration works with only
  one other data folder configured, for both 1.0 and 1.1.
- [x] **Refusal peers:** automatic protocol fixtures cover mismatched BYTEVERSIONs,
  including 117 and unsupported-build protocol 27, without sibling game data.
  Optional complete external peers remain available through `DUKE3D_NET_PEERS`.
- [x] **Network startup under load:** a forced delay reproduced equal time-seeded
  peer IDs with the old launcher. The new test waits for A's actual ID before
  delaying and launching B; the same case and concurrent test load pass.

## Tools and docs

- [x] **DOS patch helpers:** capture and save tools share LE mapping and edition
  addresses. Save hooks follow the actual capture payload and check cave bounds.
  All existing no-name patch outputs remain byte-identical; names are optional.
- [x] **G11-L33:** the ledger version column includes 1.0 and 1.3D.
- [x] **1.1 HOLODUKE:** the coverage list explicitly distinguishes captured
  inventory from unchecked activation.
- [x] **README peer names:** documents automatic protocol peers and optional
  full-game peers without a brittle numbered test name or required private folder.
- [x] **Version-option comment:** links all variant reference directories.
- [x] **Shareware configure message:** verified the existing concise wording and
  all invalid version/edition combinations.

## 0.99 beta

- [x] **Moving-sector view:** added the per-player flag, measured actor writes,
  input reset, DOS save-byte translation and current-camera selection. A real SE0
  rotation matches literal DOS camera coordinates and raw-renderer pixels;
  third-person interpolation remains covered separately.
