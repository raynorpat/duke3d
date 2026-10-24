# Release rendering reference investigation

Date: 2026-10-23. Baseline: `58bdcc3` (before runtime conversion).
Platform: macOS arm64, AppleClang 21, SDL2; RelWithDebInfo, classic software renderer.

## Issue #1: E1L1 data differs

The unmodified 1.5 build, using Atomic GRP size 44,356,548 / CRC `fd3dcff1`,
reproduces all seven failures in [issue #1](https://github.com/raynorpat/duke3d/issues/1).
The same executable rendering 1.3D size 26,524,524 / CRC `bbc9ce44` matches the
existing references. A matching 1.3D engine build also renders normal E1L1 as
`e0517697`.

Controlled substitutions use loose files in separate temporary directories;
GRPs and executable are otherwise unchanged. No proprietary assets are committed.

| Data supplied to the unchanged 1.5 renderer | Normal view | Tilt/low-detail pair | Textured map 640x400 |
|---|---|---|---|
| 1.3D GRP | e0517697 | 64e7f538 | 7282abe5 |
| Atomic GRP | 99d777cd | 67030c2c | b08ff175 |
| Atomic + only E1L1.MAP from 1.3D | e0517697 | 64e7f538 | 7282abe5 |
| Atomic + only ART files from 1.3D | 99d777cd | 67030c2c | b08ff175 |
| Atomic + E1L1.MAP and ART from 1.3D | e0517697 | 64e7f538 | 7282abe5 |

E1L1.MAP is 102,806 bytes in 1.3D and 104,562 in Atomic. SHA-256 prefixes are
`e474f3c004f52be7` and `582949828ad025fc`. PALETTE.DAT is identical; LOOKUP.DAT
and 129 existing ART tiles differ, but replacing ART alone does not change any
of the three tested results. Visual inspection of the normal captures shows the
same rooftop scene with small scene-detail differences, not a renderer failure.

The map-only substitution restores every affected textured-map resolution:

| Size | HUD (both) | Atomic map | Atomic with 1.3D map = old reference |
|---|---|---|---|
| 320x200 | 9f03fd16 | 763b8c56 | 763b8c56 |
| 640x400 | c6b6b632 | b08ff175 | 7282abe5 |
| 640x480 | 92082500 | 3378e21d | dfe0efbe |
| 1280x1024 | 43229c16 | e5dc3af1 | 441cd127 |
| 480x640 | 6a4e30ce | 28839020 | 28839020 |
| 1280x720 | 7507d7cd | d99dddf4 | cc44ab59 |
| 1600x900 | 872652c4 | ddec2065 | cc8d7da2 |

Conclusion: the seven issue failures are legitimate E1L1 asset differences.
Retain 1.3D references and use separate Atomic references selected by fingerprint.
No renderer change is justified by these failures. Floating-point Polymost
references retain their existing platform restrictions.

## Baseline compatibility inventory

Local fingerprints also verified: beta `02f18900` / 9,690,241; 1.0 shareware
`a28aa589` / 10,429,258; 1.1 shareware `912e1e8d` / 10,442,980;
1.3D shareware `983ad923` / 11,035,779. Their supplied folders contain original
CONs and the existing demo/save fixtures used by CTest. Original Atomic saves
are available. A 1.4 GRP is not available; its data-free tests ran.

| Profile | Baseline CTest cases | Established failures after socket-enabled reruns |
|---|---:|---|
| 0.99 shareware | 57 | v099_render_camera: moving-sector horizon 137 vs 126; remote-camera horizon 128 vs 120 |
| 1.0 shareware | 57 | net_connect_v10_sw, net_connect_v10_sw_coop: peers connect, then report out of sync |
| 1.1 shareware | 58 | net_connect_v11_sw, net_connect_v11_sw_coop: peers connect, then report out of sync |
| 1.3D shareware | 56 | none after socket-enabled network rerun |
| 1.3D full | 57 | none after socket-enabled network rerun |
| 1.4 | 33 | none; no original 1.4 data supplied |
| 1.5 Atomic | 104 | the seven rendering reference failures above |

Sandboxed network attempts cannot bind local UDP sockets, so their initial nine
failures per tested network profile are environmental. Outside the sandbox all
seven protocol-refusal tests pass for 1.0/1.1, while the two connect cases remain
red. Both connect cases and all refusal tests pass for full/shareware 1.3D.
The baseline failures above must be reported separately from conversion regressions.

Reproduction uses fresh `/tmp/duke-runtime-baseline-*` CMake directories,
`cmake --build ... --parallel 4` and `ctest --test-dir ... --output-on-failure`.
Render controls use `SDL_VIDEODRIVER=dummy BUILD_WINDOWED=1` and existing
`render_e1l1` / `render_classic2d` executables. Captures/logs stay outside git.

## Reference selection and old caches

The runtime wrapper asks the built `game_profile --identify` helper for the
size/CRC identity before choosing checksums. Both fixture families register in
one build. Renaming a GRP does not change its references; changing its bytes
removes stock golden coverage. Modified rendering data also needs an explicit
`DUKE3D_RENDER_GAMEVERSION` test profile; without one those probes are skipped.

`DUKE3D_RENDER_13D_*` and `DUKE3D_RENDER_ATOMIC_*` override one reference family.
`DUKE3D_RENDER_GENERIC_*` applies only to the `DUKE3D_GRP` tests. Values are
`stock` (the fingerprint table), a hex checksum, or empty (print only).
For example, `DUKE3D_RENDER_ATOMIC_E1L1` and
`DUKE3D_RENDER_13D_CLASSIC2D_640x400_MAP` affect distinct named cases.

Legacy nondefault `DUKE3D_E1L1_CHECKSUM`, `DUKE3D_E1L1_TILT_CHECKSUM` and
`DUKE3D_CLASSIC2D_<size>_<HUD|MAP>` overrides migrate to generic keys once.
Cached old 1.3D default values are ignored. An explicit old override equal to
its default cannot be distinguished from an implicit default: set a new key to
preserve that intent. Floating-point Polymost checksums remain opt-in.
