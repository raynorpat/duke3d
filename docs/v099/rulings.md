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
