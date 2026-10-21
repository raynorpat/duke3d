# 1.0 and 1.1 shareware variants: rulings and deferred items

Decisions made while building the 1.0 and 1.1 shareware variants (plan:
`docs/superpowers/plans/2026-10-20-v10-v11-variant.md`), in the order they were made.
Each names what was decided, why, and what it costs if it turns out wrong.

## Rulings

- **Task 2 (BYTEVERSION): no collision.** 1.1 uses 12 (`0x0c`) in getpackets'
  version check and as the first long of the save header. 1.0 has no BYTEVERSION:
  its name packet carries no version byte, and its save file starts with the save
  name, with no version long. 12 differs from 1.3D shareware's 14, and 1.0 has no
  value that could equal either. Evidence and addresses are in `reference.md`, under
  "BYTEVERSION". Cost if wrong: Tasks 13–14 add a release byte or tag to the
  handshake and save header. Note for those tasks: a 1.0 build cannot reject a
  mismatched peer or save by version, because the 1.0 formats have no version field.

- **Task 6 (reference saves): patched playback, not key injection.** `saveplayer` in
  both releases is plain file output, so `tools/v13d/mksave.py` calls it from a hook on
  playback's `domovethings` call at a set tic of a `gendemo.py` demo, and the same hook
  writes `STATE.BIN`. The DOSBox-X `AUTOTYPE` fallback was not needed. The expected
  values come from that memory dump. Cost if wrong: the saves come from a patched EXE,
  but the patch only adds a call after a finished tic. Run at the last tic of 1.1
  demo 1, the hook's dump matched the plain demo-end capture except for
  `ps[0].visibility`. `displayrooms` changes that field each frame, and the hook runs
  before that frame is drawn.
- **Task 6: a fifth save, `e1l2inv`.** No E1L1 spot the random walk could reach held
  both several weapons and inventory items. So `e1inv` has the weapons (E1L1 weapons
  room) and `e1l2inv` has the inventory (E1L2 inventory room). Cost if wrong: drop the
  `e1l2inv` rows and files.
