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
  in our build it is undefined. We seed it with `-1`, selecting the engine's global
  search. The final E1L3 DOS capture logged 64 actual hints: 35 were outside the
  501-sector range, and 29 were sector 0 with neither it nor its neighbors containing
  the probe point. All took the global fallback. This policy also reproduces every
  tic of the earlier 7894-tic E1L3 DOS trace; seeding with `cursectnum` changed aim on
  overlapping slopes and later desynchronized RNG. Cost if wrong: another stale DOS
  value could select a different overlapping sector, changing `horizoff` and aim.
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
- **Task 9: retain native single-player display/backend representation.** Redraw and
  help use the port globals; DOS save fields will be translated in Task 14. Native
  level/time tables retain eleven slots per episode, with beta E1 selecting equivalent
  entries. DOS-only `/d`, `/i`, `/o`, `/~` switches have no SDL backend consumer; accepted
  switches add no unused state. Sound menu device cleanup and safe load-failure handling
  remain port infrastructure. Cost if wrong: display/backend behavior differs without
  affecting the single-player simulation.
- **Task 9: preserve native startup and configuration access.** The beta banner and
  prompt are shown, but its blocking console wait is DOS-only so native automated boots
  stay unattended. Modern Controls and Video entries remain after the eight original
  beta Options entries. Cost if wrong: startup/menu navigation differs from DOS.
- **Task 10: share the measured 1.0 demo codec.** Beta and 1.0 use the same
  nine-byte header and ten-byte records; their headers cannot identify the edition.
  Refusal checks therefore omit 1.0 demos. Cost if wrong: DOS reference playback
  will expose a format mismatch.
- **Task 10: fix full-buffer demo finalization for every version.** The port only
  rewrote the record count and cleared recording state when a partial batch remained.
  Exactly 2520 records produced an invalid zero-count demo. Finalization now runs
  after the optional partial write. Cost if wrong: closing an otherwise invalid
  recording changes its header and state.
- **Task 11: extend optional route input for required map actions.** The original
  maps contain underwater transitions and combat that the initial use/fire/jump/wait
  format cannot author. Added crouch/dive, timed fire, weapon-key selection, stationary facing and
  use while crouching to
  the optional bot; these remain ordinary recorded input. Cost if wrong: route
  authoring changes, while DOS playback remains the reference.
- **Task 11: use a wider arrival radius on conveyor waypoints.** Optional `pass`
  nodes accept384 units because conveyor drift can prevent the ordinary128-unit
  arrival. Other nodes retain128. Cost if wrong: an authored path changes; the
  DOS playback comparison still decides acceptance.
- **Task 14: translate beta display fields to native globals.** Only the selected
  player's saved `pus`, `pub` and help flag update the native globals; inactive
  players write the observed1/1/0 defaults. Loading requests a redraw, so save
  comparison ignores only the active player's two redraw shorts (0x322–0x325).
  Inactive flags and every simulation field still compare. Native-only fields
  absent from the beta record are initialized, with actor/audio sentinels -1.
  Cost if wrong: a redraw differs after loading, without changing simulation.
- **Task 14: zero-extend the two-byte respawn tail.** As specified during Task2,
  read the saved low16 bits rather than reproducing DOS loadplayer's accidental
  stale upper16 bits. The absent respawn-items settings reset to zero. Cost if
  wrong: a malformed or unusual save that depended on stale decompression bytes
  gets a deterministic respawn setting.
- **Task 11: use god-mode jetpack fuel for E1L5 height barriers.** Multiple
  walking attempts reached disconnected initialized ledges; all24 SE31 floor
  assignments and seven SE2 slope resets match the beta executable. The route
  uses ordinary jetpack toggle/ascent/descent inputs with the already matched
  god state, which supplies fuel. Cost if wrong: the reference route bypasses an
  undiscovered walking path; every resulting input still plays in DOS.
- **Task 11: diagnostic filenames are not game switches.** The legacy parser
  treated an absolute `/tmp/...` diagnostic filename as `/t`, enabling monster
  respawn and causing late replay desyncs. Both command-line parsers now skip
  arguments belonging to demo/save verification and dumps. Cost if wrong: a
  diagnostic argument no longer doubles as an unrelated game switch.
- **G099-128: render from the measured beta camera fields.** The beta has no
  `opyoff`, `truefz` or `truecz`; its camera uses current bob and a hittype floor
  cap. The guarded renderer follows those calculations and the measured camera
  branches. The existing native representation still lacks the display-only
  per-player on-sector-effector interpolation flag. Cost if wrong: smoothing
  on some moving sectors differs; simulation and save layout are unchanged.

- **G099-126: preserve the beta glass-wall target within the wall array.**
  Beta `checkhitwall` at `0x48e0b` addresses `wal + nextwall*32`,
  clearing `wall[dawallnum + nextwall].cstat`. The opposite wall remains
  blocking. Version 1.0 (`0x4d953`) and beta stained glass (`0x48ecb`) use
  the wall-array base correctly. Preserve the beta outcome when the sum is
  below MAXWALLS; skip a store beyond that array rather than corrupt memory.
  E1L3 first diverged at tic 95 because wall 1773 was incorrectly cleared by
  the port when shooting wall 2331; beta instead clears unused wall 4104.
  The corrected native replay matches the DOS 1875-tic player/RNG trace.

- **G099-127: correct shared early forcefield texture orientation.** Beta,
  1.0 and 1.1 all OR 85 into forcefield wall flags;1.3D adds vertical texture
  flip 256. The inherited port forced the later flip in every version. Apply
  the measured OR 85 below GAMEVER_13D under the spec's shared-port-bug policy,
  preserving any flip already stored in the map. Cost if wrong: old-version
  forcefield textures have a different vertical orientation; collision flags
  and simulation remain unchanged.

- **C099-005/006: keep native compiler error interaction.** ESC abort and
  the beta's reload prompts only affect interactive/error paths; they do not
  change compilation of the unmodified beta CONs or demo playback. Mark those
  differences n/a. Cost if wrong: prompts differ when compiling broken or
  modified scripts.
- **G099-078: correct the shared unaimed shrinker multiplier.** Both beta
  and 1.0 multiply the firing z velocity by 100;1.1 uses 97. Apply 100 below
  GAMEVER_11 under the spec's shared-port-bug policy. Cost if wrong: unaimed
  shrinker trajectories differ in the affected early versions; the measured
  executable instructions and focused regression establish the intended value.
- **Task11: bound death/restart comparisons before DOS batching.** Native
  headless replay handles MODE_RESTART immediately after its tic; the beta
  finishes the frame's input batch first, so post-death restart timing depends
  on rendering speed. Full-route references use god mode, and the no-god death
  scenario stops at the first death before a restart is requested. Cost if
  wrong: a demo spanning death/restart can diverge at the frame boundary;
  ordinary living playback and the first-death comparison remain checked.

- **G099-129: beta quit does not wait for absent later-version ads.** The
  beta exits through gameexit 0x20930, Shutdown 0x1ca50, then a DOS text-mode
  DUKE3D.BIN display with no key waits. Native beta previously inherited 1.0's
  two graphical-ad waits, but tiles 3290/3291 are empty in beta data. Exclude
  beta from that call; keep its normal shutdown and existing native text-mode
  policy. Cost if wrong: native quit omits the DOS-only text advertisement.

- **Task13: observe gameplay ending after replay, without changing attract behavior.**
  Beta attract playback intentionally returns to the title at EOL. The private
  native test and scratch DOS observer hand the accepted boss replay’s EOL state
  to the real main ending path. DOS remains alive through 60.04 seconds of the
  victory path; its eventual INT21 exit is deliberately diagnostic. No inferred
  post-boss crash patch is applied. The native test additionally checks score,
  ordering, title return and normal shutdown. Cost if wrong: this diagnostic
  handoff would not model a live gameplay exit; the original ending routines
  and accepted boss state remain the code and input under observation.
