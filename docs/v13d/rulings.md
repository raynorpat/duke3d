# 1.3D variant: rulings and deferred items

Decisions made while building the 1.3D and 1.4 variants (plan:
`docs/superpowers/plans/2026-10-19-v13d-variant.md`), in the order they were made.
Each names what was decided, why, and what it costs if it turns out wrong. Row IDs
such as G13-A24 refer to the plan's differences ledger.

## Rulings

- **Setup:** work on new branch `v13d` cut from 64bit-cleanup HEAD (74c611e) instead of a worktree — spec says own branch; non-destructive; cost if wrong: one rebase.
- **Task 1:** reused the emulator.zip already downloaded this session (MD5 verified 6331dc16…) instead of downloading again — same URL and hash — cost if wrong: none.
- **Task 2:** no Ghidra LE loader; wrote tools/le_flatten.py (applies LE fixups and writes a raw image) and imported that as raw x86 — the LE pages are stub-relative (MZ stub at 0x352a4), which off-the-shelf loaders often get wrong — cost if wrong: re-import.
- **Task 2:** added a __watcall prototype through a Ghidra spec extension — Ghidra has no Watcom convention, and the decompiler was dropping register args — cost if wrong: decompile output only.
- **Task 2:** `ud` is recorded as individual fields (reccnt here; the rest under Formats in Task 6) instead of a struct base — the decompile shows only field addresses — cost if wrong: none for later tasks.
- **Task 3:** 1.3D BUILD_ENGINEREV pinned at 19960419 (any value in 19960320..19960426 is equivalent for logic) — EXE inside() matches that branch — cost if wrong: re-check one guard.
- **Task 3:** E-007 (FindDistance2D early returns) is fixed in Task 10 step 0 as a shared bug fix with its own unit test, though it lives in global.c rather than pragmas — it's the same class of shared-math port bug — cost if wrong: moves to another task.
- **Task 3:** plan step 1 needed the L suffix stripped from #if lines before unifdef (unifdef can't parse 19960419L) — mechanical — cost if wrong: none.
- **Task 4:** the plan's expected file list (GAMEDEF, PLAYER, ACTORS, CONFIG, _FUNCTIO, DUKE3D.H) was wrong — those guards only separate EDuke/NAM/WW2GI or 1.5-EXE revisions. The real 1.4-vs-GPL hunks are in GAME.C, MENUES.C, PREMAP.C and SOUNDS.C. Recorded what the diff shows — cost if wrong: none, the diff is mechanical.
- **Task 5:** the semantics comparison (step 3) went to two read-only subagents (compiler, interpreter), and their rows were checked against the cited disassembly lines before ledgering — the decompile is ~2.8k lines — cost if wrong: a missed difference surfaces as a desync in Task 14.
- **Task 6:** step 3 (1.4→1.3D game diff) went to five parallel read-only module agents (game, actors, player, premap, sector), each told to check structural claims in the disassembly. Their full reports are saved out of tree in ~/Projects/ref/notes/ and condensed into G13-* rows — cost if wrong: a missed row shows up as a desync in Task 14.
- **Task 7:** demo captures automated instead of the user driving the DOSBox-X debugger: a patched scratch EXE dumps state at demo end (tools/v13d/mkcapture.py), proven deterministic by a byte-identical rerun — less user effort, and reproducible — cost if wrong: the references would come from a patched EXE, but only the branch target changed and the cave runs after the last tic.
- **Task 7:** refs use the player sprite's x/y/z/ang/extra instead of ps[0].posx… (plan's _POSX names) — avoids depending on the 1.3D ps layout — cost if wrong: Task 14's -demoverify prints sprite values; it's a rename.
- **Task 7:** capture tools committed under tools/v13d/ (no game data) so captures can be repeated when bisecting — the plan only required the procedure in reference.md — cost if wrong: three small scripts in the tree.
- **Final:** Important #6 (rows depending on out-of-tree notes) — made G13-A02 self-contained; kept ~/Projects/ref/notes out of tree because the reports quote decompiled 1.3D code, which the user's "no 1.3D files in the tree" decision arguably covers — cost if wrong: Phase B implementers on another machine lack the detail; the user decides whether to commit them.
- **Task 7:** E-012 covers decoding only; our 1.3D build writes demos and saves with our LZW — spec already says recorded demos only need to play in our build and saves are written in our format — cost if wrong: add the 1996 compress for writing.
- **Task 7:** TEST1 is not e1l1start (user shot, blew a barrel, broke a wall), so it becomes an extra row e1l1played rather than filling a planned slot — cost if wrong: rename one row.
- **Task 7:** sw e1l1fight HEALTH 98 taken from the file (user confirmed they were hit before the screenshot; 73 on screen); other fields from the screen — cost if wrong: one value shares the loader's layout, but health decode was checked on 8 other saves.
- **Task 7:** sw e1late skipped at the user's request — cost if wrong: no shareware save past E1L1; full e2/e3 cover other maps.
- **Pre-flight:** E-012 changes from decode-only to guarding both compress and uncompress in 1.3D builds — main now writes native saves in the DOS layout, so a 1.3D build writing 1996 LZW gives DOS-1.3D-layout saves and demos, and its own files stay readable; supersedes the spec's "writes its own format" for 1.3D saves (tell the user) — cost if wrong: 1.3D native saves become DOS-compatible, which only widens compatibility.
- **Pre-flight:** -loadverify already exists on main (SAVESTATE health armor weapon ammo volume level posx posy); Task 17 reuses it and tests/dos_save.cmake, and the refs' LEVEL/X/Y change to volume, level and ps.posx/posy — cost if wrong: one refs rewrite.
- **Pre-flight:** data-dependent tests follow main's pattern (registered only when the data variable is set) instead of exit 77; edition checks still fail with "wrong edition" — cost if wrong: CI shows fewer tests rather than Skipped ones.
- **Pre-flight:** sub-project 2 left no -demoverify or network harness; Task 14 adds -demoverify; Task 16 decided when reached.
- **Task 8:** CI runs 1.4, 1.3D and 1.3D-sw on ubuntu only, 1.5 on all three OSes — the versions differ only in game logic; avoids 12 jobs — cost if wrong: a version-specific Windows/macOS break shows up later.
- **Task 9:** G14-009/G14-010 rows named the wrong functions (getsound holds the lock size; precachenecessarysounds holds the keepalive); rows corrected and ported per the recreation — cost if wrong: none, code follows the recreation.
- **Task 10:** E-007 FindDistance2D ported here (all versions) with tests/game_helpers.c RED→GREEN, though no task names it — it is an `all` E row and Task 10 step 0 covers `all` rows — cost if wrong: none; 1.5 also links MACT386.
- **Task 10:** E-005 row said "end of clipmove"; the recreation guard is on the early return when the slide reverses; ported there and row corrected — cost if wrong: none, follows the recreation.
- **Task 11:** the plan's "actor count from Task 5" check (never recorded) became a word-for-word comparison of script[] and actorscrptr[] against the script stored in the DOS reference save e1l1start.SAV, decoded by tests/v13d_save_decode.py at test time (no CON content committed) — stronger than a count; C-012 asked for bytecode equality — cost if wrong: the test needs refsaves/e1l1start.SAV in each data dir.
- **Task 11:** in 1.3D builds con_compile (in-repo 1.5 CONs) and game_boot aren't registered, and the DOS 1.5 save tests only register in 1.5 full builds — those inputs are other versions' data — cost if wrong: less coverage in non-1.5 builds.
- **Task 11:** C-015's CR fix applies to all platforms (terminate at [i], strip a trailing CR) — the old non-UNIX [i-1] cut a real letter from LF files on Windows — tested by con_compile (LF, E1L1 name) and con_compile_v13d (CRLF) — RED shown with the old code on con_compile_v13d_full.
- **Task 11:** C-017 and C-018 not ported (n/a with reasons in the ledger); C-001's interpreter half is unreachable.
- **Task 12:** E-001's 2048-entry table is a separate ksqrtable used only by the logic sqrt (LOGICSQRT); rendering keeps the 4096-entry nsqrtasm table, since the 1996 nsqrtasm callers are all rendering and rendering differences aren't ported — cost if wrong: none for game state.
- **Task 13:** in-memory player_struct keeps our layout (fields 1.3D lacks stay, unused by 1.3D paths); only weaprecs grows to 256; the DOS 1.3D layout is the save codec's job (Task 17) — cost if wrong: a 1.3D memory-overflow quirk would differ.
- **Task 13:** game_boot_v13d checks only that each run stays alive (title, E1L1, full E3L1); the plan's "-v2 rejected for shareware" and a clean-compile check were dropped — the game's stdout is lost when the timeout kills it and shareware /v2 shows no observable refusal headless; con_compile_v13d covers compiling — cost if wrong: a shareware /v2 regression goes unnoticed.
- **Task 13:** G13-S13 FANSPRITE wall scrap owner (a register leak in 1.3D) is the hitting sprite `spr` — cost if wrong: scrap owner differs, which only matters to code reading a scrap's owner.
- **Task 13:** premap P03 n/a (spriteqamount is already 64 in 1.3D), P09 n/a (same game variables reset; 1.3D may not reset lockclock — demos will tell), P11 n/a (net handshake, our builds only), P12 n/a (UI, position unknown); P08 ported for ffire and sound/music order only (gotpic clear and docacheit kept: cache only).
- **Task 13:** G13-L11 ports only the dropped clipping test; 1.3D then read sector[-1] (undefined in C), so sector 0 stands in as in later versions — cost if wrong: a noclip death in a void plays out slightly differently. L27 (getinput) n/a: input reading only affects recording, and our builds record their own format.
- **Task 13:** G13-G12 WATERDRIP's 1.3D condition reads sprite[-1] when j == -1 (undefined in C); treated as "not statnum 1", i.e. our parenthesised form; only the T2 krand is removed — cost if wrong: a map-placed drip could differ if 0x1cdbae happened to be 1.
- **Task 13:** G13-G27 (EGS has no overflow check in 1.3D) not ported: the check only fires on sprite overflow, where 1.3D would corrupt memory.
- **Task 13:** G13-G33 (opendemoread) moves to Task 14, which rewrites opendemoread for the 1.3D header.
- **Task 13:** not ported as n/a — G29 quote/fade UI (cloud panning ported), G31 getpackets (net, our builds only), G34 closedemowrite (our own demo format), G38 main/Startup/getnames/nonsharedkeys/dobonus flow (UI; the Plutonium Pak logo draws are removed in 1.3D, title and menus). G35 fakedomovethings (prediction only) deferred: ported constants would only change multiplayer prediction smoothness.
- **Task 13:** G13-G36 cheats: the "time" entry becomes "<RESERVED>", give-all cheats stop at weapon 10, scotty rejects episode 4; the table stays 26 entries (which entry 1.3D lacks for its 25 is unknown) — cost if wrong: one extra typable cheat.
- **Task 13:** G13-G37 command line: no default skill 2, /l also sets skill 1, /v masks with &3, /c takes 0-2, /q %16 and coop only, /s 4 doesn't set respawn; switches 1.3D lacks (/a /z /w /u /0-9 /d-as-demo, -map, -net) are kept — they do nothing unless used.
- **Task 13:** G13-A09 (insertspriteq) n/a — with spriteqamount fixed at 64 in 1.3D, `%spriteqamount` and the >0 check equal 1.3D's `&63`.
- **Task 13:** G13-A15 fire spreading confirmed in the disassembly (dist() at 0x4e360, hittype[k].temp_data[0], runs before the xrepeat store) and ported with the sprite[j] bug.
- **Task 13:** G13-A18 movetransports is a separate 1.3D function (too many differences to guard in place).
- **Task 13:** G13-A19 GREENSLIME "no quick_kick path" ported as the kill test only (frozen-slime auto-kick kept); A19/A20 FIRE/FIRE2 go on moveexplosions' execute list.
- **Task 13:** G13-A98 bpos writes ported where the report names them (moveplayers camera, player ±6144 warp, SE6/14 sprite loop and crush, SE17 warp and SE31 take the old position) since move() reads bpos.
- **Task 14:** 1.3D builds read and write the DOS 1.3D demo format (spec leaves our recording format to the builder); demo_refuse_v13d uses a synthesized 1.5-style header (byte 4 = 117) since sub-project 2 left no 1.5 recording to reuse — cost if wrong: none, the refusal rule only reads that byte range.
- **Task 15:** instead of reading the C paths of uncovered weapons and enemies against the EXE by hand, generated demos (seeded random walks, player start moved next to the target with startnear.py) were played by both the DOS executable and our build and compared (seed, sprites, ps[0]); 21 scenarios, all equal apart from render-written fields — a direct check rather than a reading, and the Phase A reports had already compared these paths — cost if wrong: code a random walk never triggers stays unverified.
- **Task 16:** the plan's "sub-project 2 network harness" doesn't exist; wrote tests/net_v13d.cmake (two peers as one execute_process pipeline, UDP on localhost) and a DUKE3D_NET_PEERS cache list of "<exe>=<data folder>" for the refusal peers, since a 1.5 build can't run the 1.3D CONs — cost if wrong: refusal tests only run where someone sets DUKE3D_NET_PEERS.
- **Task 16:** the version check already existed in getpackets; the defect (all versions) was that whichever peer started first never refused, because the later peer read its name packet and quit before sending its own. Fix: the refusing peer sends its version first. RED shown by disabling the reply (net_v13d refuse: first peer "didn't refuse" in both start orders), GREEN after — cost if wrong: one extra packet on a game that is exiting anyway.
- **Task 16:** added fflush(stdout) after "Multiplayer initialized." (all versions) so the connect test can see it after killing the peers at the timeout — cost if wrong: none at runtime.
- **Task 16:** built and tested in a scratch worktree (HEAD + only Task 16 changes) because uncommitted audiolib/OPL work by someone else in the main tree breaks the build; committed only game.c, tests/CMakeLists.txt, tests/net_v13d.cmake after checking they match the tested files byte for byte — cost if wrong: none.
- **Task 17:** 1.3D builds read AND write the DOS 1.3D save layout (spec only required reading; native format left to us) — one reader/writer, our saves and DOS saves interchangeable, wrong-edition/1.5 saves refused by BYTEVERSION — cost if wrong: 1.3D saves drop fields DOS 1.3D never saved (randomseed, multiplayer state), same as the original.
- **Task 17:** tests/dos_save.cmake already exists for 1.5 saves; added tests/dos_save_v13d.cmake. save_refuse_v13d uses the other edition's DOS save plus a 10-byte 1.5 header fixture (tests/data/v15_save_header.sav); a "native 1.3D shareware save" is the same format as a DOS shareware save under the ruling above. The refuse tests passed before implementation (existing BYTEVERSION check) — not RED.
- **Task 18:** the README the user committed (b6b4402) already had DUKE3D_VERSION and the data-dir cache variables; added a 'Playing 1.3D' section (data, loose demos, DOS saves), fixed '1.3D saves aren't supported', and listed the save tests. Closed the last five ledger rows; G13-G35 was ported rather than left deferred (50ed237).
- **Final:** the game never checks which edition its data is — the user asked for a GRP mismatch warning next; it covers this — cost if wrong: none.
- **Final:** 1.4 saves/demos use the 1.5 layout tagged 116 — spec scopes 1.4 to build and CON checks; no DOS 1.4 data to test — cost if wrong: real DOS 1.4 saves may not load in 1.4 builds.
- **Final:** music volume default 128→192 — the user's audio work, out of scope.
- **Final:** hittype ≥2048 zeroed and coop/camsprite/randomseed/clouds kept on a 1.3D load — matches what DOS 1.3D saves.
- **Final:** animoffs and vn/ln from saves unchecked — predate the branch; a crafted save could still misbehave there; left for a separate hardening pass — cost if wrong: a crafted save crashes instead of being refused.
- **Final:** games hang at the CON "internal defaults (Y/N)?" prompt with mismatched data — predates the branch; the GRP mismatch warning makes the cause visible — cost if wrong: still a hang for a non-interactive start.
- **Final:** editor built with the 1.3D engine revision in 1.3D builds — the 1.3D editor is a spec non-goal.
- **Final:** faithful 1.3D quirks (FIRE setsprite y-as-z, sprite[j] fire test, sector 0 stand-in, walock) — backed by demo sync and generated-scenario tests.
- **Final:** most 1.3D data tests aren't in CI — no data in the tree, by the user's decision.
- **Final:** untracked source/.!13590!game.c — gone, never committed.

## Deferred items

Minor findings from the reviews, left for later.

- le_flatten.py doesn't assert on additive fixups (fl&4); types 2/5/6 speculative
- mkcapture.py callsite limited to <0x40000; dead firsttic branch; no EXE MD5 check; user-specific fallback path
- refs .cmake header doesn't say RANDOMSEED is unsigned; statebin.py has no size check
- C-014 says DUKE_GAMEVER (should be DUKE3D_GAMEVER)
- address map "Module" column says game for functions in other modules
- save slot count (4 sw / 5 full) and file naming reconciled in reference.md but not the plan
- save-file item 19 scalar names unverified
- E-007 assumes 1.5's MACT386 FindDistance2D matches 1.3D's
- reference.md points at ~/Projects/ref/tools instead of tools/v13d
- cache1d.c relies on CMake for BUILD_ENGINEREV (no build.h include); savegame.h relies on duke3d.h for gamever.h
- demoverify used before declaration (game.c:7987); add a prototype to funct.h
- ~~garbled configure error for 1.4+shareware (CMakeLists.txt:20)~~ (fixed in b552652); shareware refusing a full E2/E3 demo says "isn't a 1.3D demo"; 1.4 builds say "not a DOS 1.5 save"
- 1.3D CON cut at fs-2 on every platform (C-012, faithful) loses the last character of an LF-only CON
- /v4, /v0 and /q16 give out-of-range volume and multimode 0 (divide by zero); clamp
- 1.5 banner "v1.4"→"v1.5" has no Fix: comment
- 1.3D attract demos desync after the player used cheats (G13-G33, faithful to DOS)
