# 64-bit Demos, Editor and Network — Design

Date: 2026-10-19
Status: Approved (brainstorming)

This is sub-project 2 of 3 for running duke3d on 64-bit targets. It follows the core
cleanup (`2026-10-19-64bit-cleanup-design.md`), which is merged into `main`.

1. Core 64-bit cleanup (done)
2. **Demos, the build editor's map save path, and network play on 64-bit** (this spec)
3. Loading original DOS `.SAV` files (separate spec)

## Goal

Demo recording and playback and the editor's map save are verified on a 64-bit macOS
build. The network code is correct for Win64 and still builds on all three OSes.

## Findings

A survey of the three areas found that they already read and write their formats one
field or one byte at a time:

- **Demo header:** written by `opendemowrite` byte by byte, 674 + `multimode` bytes, the
  same as DOS. The body is `sizeof(input) * multimode` per tick.
- **Demo input:** `input` is packed to 10 bytes, the DOS size, but nothing asserts it.
- **Maps:** `saveboard`/`loadboard` use `write8/16/32` and `kread8/16/32` with 40/32/44-byte
  records, already asserted in `build.h`.
- **Network packets:** built and parsed byte by byte in `game.c`, `menues.c` and
  `mmulti.c`. No struct or pointer crosses the wire.

So the work is two small fixes plus verification.

## Scope

In scope:

1. `BUILD_STATIC_ASSERT(sizeof(input) == 10, input_size);` in `source/duke3d.h`, next to
   the existing struct-size asserts.
2. `source/build/mmulti.c`, Windows branch: `#define socklen_t size_t` becomes
   `#define socklen_t int`. Winsock's `recvfrom` takes an `int *`, and `size_t` is 8 bytes
   on Win64.
3. Manual verification on macOS (see Testing).
4. Docs:
   - `README.md`'s 64-bit paragraph says demo recording and playback and editor map
     saving are verified, and network play builds but isn't verified on 64-bit.
   - `source/build/VENDORED.md` gets a line for the `mmulti.c` change.

Out of scope (parked follow-ups, none specific to 64-bit):

- **Demo rewind call:** `closedemowrite` calls `fseek(frecfilep,SEEK_SET,0L)` with its
  arguments swapped. It works only because `SEEK_SET` is 0.
- **Empty demo on the buffer boundary:** a demo whose recorded tick count lands exactly
  on the `recsync` buffer boundary is saved with `reccnt` 0.
- **Unchecked packet length:** `mmulti.c` `getpacket` trusts the remote packet length
  when copying into 576-byte buffers.
- **Loopback player slot:** `mmulti.c` picks `myconnectindex` by IP only. Two instances on
  one machine therefore can't connect, which blocks a loopback network test.
- **Player-table overrun:** loops run `i <= numplayers`, so they can index one past the
  16 `allowed_addresses` entries with a full 16-player config.
- **Unchecked map writes:** `saveboard` ignores `write()` errors.
- **Map-name overflow:** `build.c` copies `argv[1]` into `boardfilename[128]` unbounded.
- **1.3D demos:** playing the original 1.3D demos (`DEMO1.DMO`/`DEMO2.DMO` use an older
  header layout). This belongs to the 1.3D work on `v13d`.
- **DOS save loading:** sub-project 3.

## Testing

1. **Compile-time:** the new `input` size assert. CI builds all three OSes, including
   Win64 with the `mmulti.c` change.
2. **Existing CTest suite** passes unchanged: `sdl_driver_smoke` and `con_compile`, plus
   `render_e1l1` and `game_boot` with `DUKE3D_GRP` set.
3. **Editor round trip (controller, real window):**
   1. Copy `E1L1.MAP` out of the GRP into a scratch run folder containing a `duke3d.grp`
      symlink.
   2. Run `build/bin/build e1l1` there, save without changes, and quit.
   3. Compare the saved file with the original.
      - Pass: the same size, and identical apart from sprite record order. `saveboard`
        writes sprites in `statnum` order.
      - A small script compares the header, the sector and wall bytes exactly, and the
        sprite records as a multiset.
   4. Load the saved map in `duke3d -map e1l1.map` and confirm it renders. This
      switch takes `-`, unlike the `/` game switches.
4. **Demo (the user, real window):**
   1. Start E1L1 with `/r /v1 /l1 /s2` and play for a minute.
   2. Quit through the menu so `closedemowrite` writes the header.
   3. Play it back with `/ddemo1.dmo`.
   - Pass: the playback replays the session, with no "out of sync" messages, until the
     demo ends.
5. **Network:** not verified. It needs two machines until the loopback player-slot issue
   above is fixed.

## Definition of done

1. macOS arm64 builds with zero errors, and `ctest` passes (2/2 without a GRP, 4/4 with).
2. The editor round trip passes.
3. The user's demo check passes.
4. When the user pushes, all three CI jobs build and pass.
5. The README and `VENDORED.md` are updated as above.
