# TODO

Small follow-ups left from the 1.0 and 1.1 shareware variants (branch `v10-v11`). Each was
reviewed and judged safe to leave for now. Ledger row IDs refer to the Differences ledger in
`docs/superpowers/plans/2026-10-20-v10-v11-variant.md`.

## Code

- [ ] **Signed overflow in `inside()`** (`source/build/engine.c`). The 1.0/1.1 and 1.3D forms
  (E11-001, E-004) rely on `int32` products wrapping, which is undefined in C. Compute them
  with `uint32_t` casts, or build `buildengine` with `-fwrapv`. Watch for it if demo sync ever
  breaks under a new compiler.
- [ ] **Two `TRAND` calls in one expression** (`source/player.c`, the 1.1 death path and the
  1.3D one below it). C leaves their order unspecified; DOS drew `&1` first. Split each into
  two statements.
- [ ] **1.3D demo header reader** (`source/game.c`, `opendemoread`): a truncated header still
  range-checks uninitialized locals. Start them out of range, as the 1.1/1.0 reader now does.
- [ ] **`checkhitsprite` and `cheatkeys` have 1.1-only copies** (`source/sector.c`). A future
  fix to either function has to be made twice; consider folding the 1.1 copy of
  `checkhitsprite` back into guards (about 15 `#if` blocks).
- [ ] **`checkweapons` 1.0 branch** (`source/player.c`) ends in `return;`, leaving the 1.1 body
  compiled but unreachable. Wrap the body in `#if`/`#else` instead.
- [ ] **`move` flag 2048 owner guard** (`source/gamedef.c`): the `j < 0` test isn't in the
  1.1 EXE (it reads `sprite[-1]`). Add a one-line comment saying so.
- [ ] **Net prediction drift in 1.0 and 1.1** (`source/game.c`, `fakedomovethings`): it still
  applies water friction and the 1.3D shrunk formula, which 1.0/1.1 `processinput` doesn't.
  Prediction only; visible as small corrections in net games.
- [ ] **1.0 `debugprint` opcode** uses the host `rand()`, not Watcom's generator. No shipped
  CON uses it; a mod that does would play a different sound.
- [ ] **1.0 demo `avel`** (`source/game.c`, `demo10_fromdos`) narrows s16 to s8 without a
  clamp. Safe for DOS demos (1.0 clamps to ±108); add a clamp or comment.
- [ ] **1.0/1.1 saves with a modified CON over 10240 longs** are cut to 10240 without a warning.
- [ ] **`ps[]`/`po[]` entries 8–15** keep old values when a 1.1/1.0 save loads. Unused (at most
  8 players), but clearing them would be tidier.
- [ ] **1.0 save writer heuristic** (`source/menues.c`): a genuine numeric actor
  `t[1]`/`t[4]`/`t[5]` that looks like a script offset is written as an address. Our loader
  reverses it; the DOS EXE wouldn't.
- [ ] **`sg_playerblocksize`** is compiled but unused in 1.0 builds.
- [ ] **Save loader hardening, all versions** (`source/menues.c`): `animateptr` offsets, stored
  script offsets and `kdfread` short reads aren't checked for 1.3D/1.4/1.5 saves (1.0 saves are
  fully checked by `sg_check10`).
- [ ] **Order screens before video is set up** (`source/game.c`, `showtwoscreens`): shareware
  builds draw them from `gameexit` even when a CON error exits before the video mode is set
  (`source/gamedef.c` missing-CON, error and modified-CON paths). Unreachable today (the prompt
  waits for a key first), but a `qsetmode == 200` check at the top would make it safe.
- [ ] **`ps.auto_run` in 1.0** is never cleared at a new game, matching the EXE. It only feeds
  local input.

## Tests

- [ ] **No ctest covers G10-L14** (1.0 `weapon_ang`), G11-A35 or other fixes found by scenario
  demos; the DOS captures are scratch only. Consider committing the scenario numbers as refs.
- [ ] **Burning and the space suit** (`burn_time`, G11-A15/L06, G11-A11/L13/L23) are never
  exercised: no shareware map reaches them. Ported from disassembly only.
- [ ] **No reference save has armor**, and every save name is empty.
- [ ] **Connect tests show no positive "sync was checked" marker**; a test would pass if the
  peers never reached a level. Print a line when the first sync value is compared.
- [ ] **`game_boot_v11_sw` was never seen failing** (it passed before the port it guards).
- [ ] **Edition-mismatch tests** (`v11_edition_mismatch`, `v10_edition_mismatch`) register only
  when both other data folders are set; register with whichever is present.
- [ ] **Refusal peers** in `DUKE3D_NET_PEERS` are wired by hand, and the 1.5 peer needs the
  git-ignored `build-v15/netpeer` folder. A 1.5 shareware peer (BYTEVERSION 27) can't be built.
- [ ] **`net_connect_v13d_full`** has timed out once under heavy parallel load. Watch it.

## Tools and docs

- [ ] **`tools/v13d/mksave.py`** duplicates `mkcapture.py`'s LE helpers and addresses, and
  places its hook at a fixed cave offset.
- [ ] **Ledger row G11-L33**'s Version column still says 1.1; it now covers 1.3D and 1.0 too.
- [ ] **Coverage list (1.1)** doesn't list HOLODUKE (an item, not a weapon or enemy).
- [ ] **README** names `net_refuse_v10_sw_3`; the number depends on the order of
  `DUKE3D_NET_PEERS` entries.
- [ ] **`CMakeLists.txt`** comment above `DUKE3D_VERSION` points only at `docs/v13d/`.
- [ ] **Shareware-only configure error** (`CMakeLists.txt`) repeats itself ("only supported as
  shareware: 1.1 and 1.0 only exist as shareware builds").
