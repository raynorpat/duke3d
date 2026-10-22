# 0.99 beta variant: rulings and deferred items

Decisions made while building the 0.99 beta variant (plan:
`docs/superpowers/plans/2026-10-21-v099-variant.md`), in the order they were made.
Each names what was decided, why, and what it costs if it turns out wrong.

## Rulings

- **Spec: crashes are fixed, not copied.** If the DOS beta crashes or hangs somewhere in
  E1 (for example after the boss, EDuke32 `8b9f4345`), our 0.99 build fixes it. The fix is
  guarded `< GAMEVER_10`, gets a ledger row marked `fix (departs from EXE)`, and a ruling
  here. No DOS demo can check past such a point, so the stretch after it is checked by
  playing it in our build. Cost if wrong: a user who wants the crash gets a working game.
- **Spec: normal skill, cheats only where needed.** Reference runs use the normal skill
  level. Cheats are used only where a run cannot otherwise reach the thing being checked.
  Cost if wrong: a recorded run takes a different path and its demo needs recording again.
- **Spec: the DOS beta's playback is the reference.** The expected result of any
  behavior question is what the beta EXE does when it plays a demo, not what the scripts
  or EDuke32 suggest. Cost if wrong: a check passes against our build while the EXE
  differs.
- **G099-066: the PLAYERONWATER `statnum &= 0x8000` is fixed.** In a lotag-2 transporter
  the beta zeroes the dummy player's `statnum` without changespritestat (`0x152a7`). The
  sprite stays linked in its own list, so movedummyplayers' later deletesprite unlinks it
  as list 0, and a list-13 head can be left on a freed sprite: a crash or hang. Our 0.99
  build makes no store there and skips the sprite as the EXE does. Because the beta's spawn
  never sets bit 15 (G099-016), 1.0's `cstat &= 32767` would also change nothing. Cost if
  wrong: a sprite that the beta would treat as statnum 0 until it is deleted keeps its real
  statnum.
- **G099-086: the slope look-ahead `updatesector` seed is fixed.** processinput passes an
  uninitialized stack word as the start sector (`0x40434`). In DOS it is stale stack data;
  in our build it is undefined. We seed it with `cursectnum`. The result differs only where
  sectors overlap at the probe point. Cost if wrong: on an overlapping slope a demo's
  `horizoff` (and so aim) differs from DOS.
- **G099-023: `warp##` digits outside the level tables are ignored.** The beta has no
  range check, so `0` or a high digit indexes the level tables out of bounds. A cheat never
  runs in a demo, so this departs from no reference run. Cost if wrong: typing a bad warp
  in our build does nothing instead of whatever the EXE does.
- **G099-021: lotsofglass's missing `sect >= 0` test is copied.** The out-of-sector read
  lands on the engine `palette[]` just before `sector[]`; with the beta's PALETTE.DAT the
  read `floorz` is `0x163f00`, so `z` always fails the ±8192 test. We port that outcome
  (one discarded krand, then the fallback `z`) without the read; EGS uses the sprite's own
  sector, so nothing is written out of bounds. Cost if wrong: if the palette bytes differ
  at run time, the krand count still matches but a glass piece's `z` may not.
- **G099-011: the missing FEM4, PODFEM1, TRASH and DUKECAR cases are copied.** The sprites
  stay stat 0 with their map tags; no code reads them as those actors, and nothing is out of
  bounds. Only E1L5's one FEM4 is affected. Cost if wrong: none found.
- **G099-061: the WATERDRIP krand drain is copied.** A landed map drip resets every tic and
  draws one krand per tic for the rest of the level. It is deterministic and demo sync
  depends on it. Cost if wrong: none; it is what the EXE does.
- **Task 7: reference saves use the demo-playback saveplayer hook.** Runtime capture
  confirms the beta can save directly after domovethings, so no AUTOTYPE save fallback
  is needed. The startup-banner Enter is still automated. Cost if wrong: regenerate the
  four saves with another recording path.
