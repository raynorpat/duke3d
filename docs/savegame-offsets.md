# Save animation offsets

DOS saves store live animation pointers as byte offsets from `sector[]`. Sliding
walls animate `wall[].x` or `.y`, so these offsets can be negative. The loader
accepts sector ceiling/floor fields and checks wall x/y offsets against native and
measured DOS layouts. It checks the array range and field alignment
before forming a pointer.

| Version | DOS wall base | DOS sector base | Wall minus sector |
|---|---|---|---|
| 0.99 | `0x1b9e5c` | `0x20515c` | `-0x4b300` |
| 1.0 | `0x1d3d1c` | `0x21f01c` | `-0x4b300` |
| 1.1 | `0x1d4030` | `0x21f330` | `-0x4b300` |
| 1.3D full | `0x1fabc0` | `0x245ec0` | `-0x4b300` |
| 1.3D shareware | `0x1fad10` | `0x246010` | `-0x4b300` |
| 1.4 | unverified | unverified | no DOS wall translation |
| 1.5 | `0x251aa4` | `0x29d2a4` | `-0x4b800` |

The older addresses are recorded in [the beta actor audit](v099/notes/0.99-game.md),
[the 1.0/1.1 save layout](v10-v11/reference.md#save-file) and
[the 1.3D address map](v13d/reference.md). The original 1.1 save writer passes the
wall base at `0x39059` and sector base at `0x39085`. In the original Atomic 1.5 CD
executable (MD5 `7f92da0bd971a64a1cfde828eb98899b`), saveplayer passes the wall base
at `0x420dd` (8192 records of 32 bytes) and sector base at `0x42109` (1024 records
of 40 bytes). Both bases are after DOS/4GW fixups; their difference is independent
of the executable's runtime relocation.

The pinned reconstruction uses the 19960820 engine for 1.4 and a later engine for
1.5. Their public/static BSS declarations differ, including the VESA mode arrays;
1.5's relative addresses cannot safely be assumed for 1.4. Native 1.4 animation
offsets remain supported. Unmapped DOS 1.4 wall offsets are refused until an
original 1.4 executable establishes its layout.

Native saves retain their existing encoding relative to the current arrays. Any
set script pointer bitmap entry proves DOS provenance, because the native writer
zeros that bitmap. For those saves the loader uses only the measured DOS wall
layout. If every entry is zero, provenance is unknown: the loader accepts a unique
valid native or DOS interpretation, or two interpretations naming the same wall
field. If both name different fields, it refuses the save instead of moving the
wrong wall. A DOS compiler whose script exactly fills its array can leave its end
pointer unflagged, so an all-zero bitmap does not universally prove native origin.

The current macOS builds place native wall offsets at positive values, disjoint
from the negative DOS windows. A different linker may place the windows over each
other; unit tests supply such a layout explicitly and check ambiguous refusal,
positive DOS selection and agreement. A DOS wall pointer is also re-encoded with
the native writer's offset calculation and loaded back to the same field.

Both early unversioned loaders also decode and check bitmap-marked script offsets,
without overwriting the raw hittype block needed for DOS script-base validation.
Only the script end marker at index zero may name the end of the script array.
