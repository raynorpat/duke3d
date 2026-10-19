# 64-bit Demos, Editor and Network Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Demo recording and playback and the editor's map save are verified on 64-bit macOS. The Win64 network code uses the right socket length type.

**Architecture:** Two small code fixes (a compile-time size check on the demo `input` record, and `socklen_t` on Windows), a controller-run editor round-trip check, and doc updates. Spec: `docs/superpowers/specs/2026-10-19-64bit-demos-editor-net-design.md`.

**Tech Stack:** C99, CMake ≥ 3.24, SDL2, CTest.

## Global Constraints

- Work in the worktree `/Users/raynorpat/Projects/duke3d-64bit` on branch `64bit-demos-editor-net`. Never touch `/Users/raynorpat/Projects/duke3d`.
- Never write into `/Users/raynorpat/Downloads/DUKE3D`. The GRP there is read-only input; symlink it into a scratch run folder.
- Commit messages: one short line, a subsystem prefix (`game:`, `engine:`, `docs:`), and no trailers or metadata. Never push.
- Surgical changes in the existing style. Out-of-scope items stay parked; they are listed in the spec's "Out of scope".
- The shell's `grep` skips git-ignored files; use `/usr/bin/grep`.
- Builds: `cmake --build build` (no GRP) and `cmake --build build-grp` (configured with `-DDUKE3D_GRP=/Users/raynorpat/Downloads/DUKE3D/DUKE3D.GRP`). Both configure directories already exist in the worktree.

---

### Task 1: Demo input size assert and Win64 socklen_t

**Files:**
- Modify: `source/duke3d.h` (after the `input` typedef and its `#pragma pack(pop)` block, around lines 299-312)
- Modify: `source/build/mmulti.c:542`

**Interfaces:**
- Consumes: `BUILD_STATIC_ASSERT(cond, name)` from `source/build/build.h`, already used in `duke3d.h`, e.g. `BUILD_STATIC_ASSERT(sizeof(struct animwalltype) == 8, animwalltype_size);`.
- Produces: nothing used by later tasks.

- [ ] **Step 1: Prove the assert fires.** After the `#endif` that closes the `#pragma pack(pop)` block following `} input;`, add a deliberately wrong assert:

```c
BUILD_STATIC_ASSERT(sizeof(input) == 11, input_size);
```

Run `cmake --build build 2>&1 | /usr/bin/grep -m3 -E "input_size|error"`.
Expected: a compile error naming `build_static_assert_input_size` (negative array size).

- [ ] **Step 2: Set the real size.** Change the line to:

```c
BUILD_STATIC_ASSERT(sizeof(input) == 10, input_size);
```

Run `cmake --build build 2>&1 | tail -3`.
Expected: the build succeeds with 0 errors.

- [ ] **Step 3: Commit.**

```bash
git add source/duke3d.h
git commit -m "game: assert the demo input record stays 10 bytes"
```

- [ ] **Step 4: Fix socklen_t on Windows.** In `source/build/mmulti.c`, in the `#if PLATFORM_WIN32` block under `#elif UDP_NETWORKING`, change

```c
#  define socklen_t size_t
```

to

```c
#  define socklen_t int
```

Winsock's `recvfrom` takes an `int *` for the address length, and `size_t` is 8 bytes on Win64.

Then:
- run `cmake --build build 2>&1 | tail -3` to confirm the macOS build still has 0 errors;
- run `/usr/bin/grep -n "socklen_t" source/build/mmulti.c` and confirm the uses at the `recvfrom` sites (`fromlen`, `l`) now get `int` on Windows.

MSVC can't be run locally; CI verifies it after the user pushes.

- [ ] **Step 5: Run the tests.**

```bash
ctest --test-dir build 2>&1 | tail -3
cmake --build build-grp 2>&1 | tail -1 && ctest --test-dir build-grp 2>&1 | tail -3
```

Expected:
- `100% tests passed, 0 tests failed out of 2`
- `100% tests passed, 0 tests failed out of 4`

- [ ] **Step 6: Commit.**

```bash
git add source/build/mmulti.c
git commit -m "engine: use int for socklen_t in the Windows UDP code"
```

### Task 2: Editor map round trip (controller, real window)

The controller runs this itself, not a subagent, because it needs a real window and keystrokes. Nothing is committed. The comparison script lives in the session scratchpad.

**Files:**
- Create (scratch only): `<scratchpad>/maprt/cmpmap.py`, plus a run folder `<scratchpad>/maprt/run/`.

- [ ] **Step 1: Set up the run folder.**
  - Symlink `/Users/raynorpat/Downloads/DUKE3D/DUKE3D.GRP` as `run/duke3d.grp`.
  - Extract `E1L1.MAP` from the GRP into `run/e1l1.map` and keep a pristine copy `orig.map` outside `run/`. Extraction: read the GRP header `KenSilverman`, the int32 file count, and 16-byte entries of a 12-byte name plus an int32 size; data follows the directory. Use `python3 -I`.
  - Give the run a scratch `HOME` so no real config is touched.

- [ ] **Step 2: Write `cmpmap.py`.** It parses both files using the loadboard layout:
  - header: int32 version, int32 x/y/z, int16 ang, int16 cursectnum;
  - int16 numsectors, then 40-byte sectors;
  - int16 numwalls, then 32-byte walls;
  - int16 numsprites, then 44-byte sprites.

  It reports:
  - whether the sizes are equal;
  - whether the header, sectors and walls are byte-identical;
  - whether the sprite records are equal as a multiset (sorted lists of 44-byte records).

  It exits 0 only if all of these hold.

- [ ] **Step 3: Self-check the script.** `python3 -I cmpmap.py orig.map orig.map` exits 0. A copy with one sector byte flipped exits nonzero.

- [ ] **Step 4: Round-trip the map in the editor.**
  - In `run/`, start `BUILD_WINDOWED=1 /Users/raynorpat/Projects/duke3d-64bit/build/bin/build e1l1`.
  - Find the window id (JXA/CGWindowList) and take a window-only screenshot to confirm the map loaded.
  - Save without changes using the editor's save key. Check `source/build/build.c` for the 2D/3D save binding (in 2D mode the classic Build binding is Ctrl+S, or ESC then `S`). Send keystrokes with `osascript`.
  - Quit, and confirm `run/e1l1.map` was rewritten (its mtime changed).

- [ ] **Step 5: Compare.** Run `python3 -I cmpmap.py orig.map run/e1l1.map`.
  Expected: exit 0. Record the output.

- [ ] **Step 6: Load the saved map in the game.**
  - In `run/`, start `BUILD_WINDOWED=1 .../build/bin/duke3d -map e1l1.map`.
  - Take a window-only screenshot, start a game if needed, and confirm the map renders.
  - Note: if the GRP's CONs fail on 1.3D data, copy `testdata/game.con`, `defs.con` and `user.con` into `run/` as `GAME.CON`, `DEFS.CON` and `USER.CON`.

- [ ] **Step 7: Record the result** in the ledger (pass/fail, `cmpmap.py` output, screenshot paths). If the round trip fails, stop and report. That is a defect to fix before Task 3.

### Task 3: Docs

**Files:**
- Modify: `README.md` (the `**64-bit:**` paragraph, around lines 40-45)
- Modify: `source/build/VENDORED.md` (the "Local changes" list)

- [ ] **Step 1: README.** Replace the sentences

```
Demo recording, the editor's map saving and network play haven't
been verified on 64-bit. Those are the next steps.
```

with

```
Demo recording and playback and the editor's map saving are verified on
64-bit. Network play builds but hasn't been verified on 64-bit.
```

Keep the line wrapping style of the surrounding paragraph.

- [ ] **Step 2: VENDORED.md.** Append to the end of the "Local changes" list:

```
- `mmulti.c`: `socklen_t` is `int` on Windows (it was `size_t`, which is 8
  bytes on Win64 while Winsock's `recvfrom` takes an `int *`)
```

- [ ] **Step 3: Commit.**

```bash
git add README.md source/build/VENDORED.md
git commit -m "docs: note verified demos and editor saving on 64-bit"
```

### After all tasks

- **Demo check:** the user runs the spec's Testing step 4 by hand:
  1. Record with `/r /v1 /l1 /s2`.
  2. Quit through the menu.
  3. Play it back with `/ddemo1.dmo`.
- **README timing:** Task 3's README wording assumes this passes. If it fails, revert that sentence before merging.
