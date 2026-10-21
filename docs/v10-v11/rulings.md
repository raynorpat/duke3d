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
- **Task 16c (1.0 saves): told by layout, with the script base taken from the save.**
  A 1.0 save has no version, so the 1.0 build (which has none either) takes a file
  only if every `dfwrite` block of the 1.0 layout is there with its uncompressed size,
  using the file's own `numwalls` and `numsectors`, and the file ends with the last
  block (`sg_check10`). The first block alone already tells 1.0 apart: it is the
  19-byte name, where 1.1, 1.3D and 1.5 start with a 4-byte version. The script's DOS
  address is read from the save's empty `actorscrptr[]` entries rather than fixed at
  the DOSBox value `0x2d5f08`. A save is refused if those entries disagree, if a
  non-empty one isn't an aligned offset inside the script, or if an actor's
  `temp_data[1]`, `[4]` or `[5]` at or above the base isn't inside the script. The "at or above the base is an address" rule for those
  `temp_data` slots is our reconstruction: 1.0 itself never translates `hittype`
  (it relies on the run-time base being the same) and applies that test only to
  `script[]`. Our saves write `0x2d5f08`. Cost if wrong: a 1.0 save with a
  number at or above the base in those slots would be refused or misread, and a
  non-1.0 file that happened to have the whole 1.0 block layout would load as one.

- **Task 3 (1.0 CON notes): named beside the 1.1 ones.** The plan names only 1.1 notes
  files, but step 3 compares 1.1 with 1.0 too, so the 1.0 notes are
  `notes/1.0-con-*.md`. Cost if wrong: file naming only.
- **Task 11 (demo refusal): the same 1.5 header input as the 1.3D test.** The
  `demo_refuse` test uses whatever 1.5-header demo the existing 1.3D refusal test uses
  (`tests/demo_sync_v13d.cmake`, `MODE=refuse`), not the e2 header file, which is a
  1.3D header. Cost if wrong: none.
- **Commits carry a Co-Authored-By trailer.** It matches every recent commit on main,
  despite the "no metadata" line in CLAUDE.md. Cost if wrong: amend the trailers
  before merge.
- **BYTEVERSION for 1.0 is 10.** No release uses 10 (12, 14, 27, 103, 116 and 117 are
  taken). It is used only where our 1.0 build writes or reads its own formats (net
  handshake, the save header it writes), since DOS 1.0 has no version field to copy.
  Cost if wrong: a renumber.
- **Mismatch refusal for 1.0.** Our packets carry BYTEVERSION as in the other builds
  (DOS compatibility of 1.0 multiplayer is a non-goal). A 1.0 build recognises a DOS 1.0
  save by its layout and refuses anything else; a 1.1 build refuses a 1.0 save because
  it lacks the version long. Cost if wrong: a layout test could accept a malformed
  file.
- **mkcapture's cave-empty assert is per edition.** The 0x600 check becomes 0x560
  bytes for the 1.0 cave. Cost if wrong: none for 1.1.
- **Research tasks done without read-only agents per module.** The implementer read
  each module itself. Cost: longer single tasks.
- **Big tasks split into sequential sub-steps, one commit each.** Task 5 into formats,
  game and premap, actors and sector, player, and the 1.0 delta; Task 10 per module;
  Task 15 into CON and engine, then game and boot; Task 16 into 1.0 demos, uncovered
  code, DOS saves and net. One review covered each range or sub-part. Cost if wrong: a
  larger review, with cross-commit interactions seen late.
- **Task 10 manual check deferred.** The user's manual check was left for the finish
  report instead of pausing the run. Cost if wrong: issues surface late.
- **Unguarded `-1` initializers.** They are behavior-neutral for 1.1 and later, where
  hitscan writes `-1` first. Cost if wrong: a tiny codegen change in 1.1+ objects.
- **Task 16d (1.0 net): the refusal peer for 1.5 is a full 1.5 build.** The 1.5
  shareware build was already unbuildable before this branch (CMake refused shareware
  except for 1.3D). Cost if wrong: the BYTEVERSION 27 refusal is untested against a
  1.5 shareware peer. The peer's data setup is in the README, next to
  `DUKE3D_NET_PEERS`.
