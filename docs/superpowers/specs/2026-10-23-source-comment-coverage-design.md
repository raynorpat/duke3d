# Remaining source comment coverage

## Intent and scope

Continue the function summaries and selective inline explanations established
through `4057429`. Cover save compatibility, animation decoding, input and
configuration, software Polymost, networking, editor operations, and audio
internals. Explain contracts, units, ownership, state transitions and legacy
arithmetic that a maintainer cannot easily infer from the statements alone.

Use short Quake-style function banners and plain, direct inline comments.
Preserve useful existing comments and original attribution. Do not impose a
comment quota or annotate obvious wrappers just to increase coverage.

## Current branches

The isolated worktree is `/Users/raynorpat/.codex/worktrees/e4fe/duke3d`, at
`4057429`. Main now includes `d84c766` (`input: add frame polling and precise
camera aiming`). Their common ancestor is `55cbcaa`.

Input work is no longer deferred. Reconcile main into the isolated worktree
before commenting: main changed the old `source/build/sdl_driver.c`, while the
worktree replaced it with `sdl_main.c`, `sdl_input.c`, `sdl_window.c`,
`sdl_sys.c` and their private header. Preserve the input behavior and tests,
the SDL split, platform filename changes, and previous comments.

Do not modify the original checkout or its untracked files. Do not merge these
passes back to main as part of this work.

## Approach

Use seven focused passes, each with a subsystem commit and independent checks.
First reconcile the branches as a separate integration commit. Capture the
combined implementation as the baseline for all comment-only comparisons.
This keeps conflict resolution separate from documentation changes.

A single tree-wide editing pass would make factual review and regression
attribution harder. Automatic blanket function comments would mostly restate
names and miss the version and arithmetic contracts that matter here.

## Constraints and acceptance

- After branch reconciliation, source changes must be comments only.
- Preserve signatures, executable tokens, preprocessor directives, arithmetic,
  data layouts, file formats and platform/version behavior.
- Treat unexpected legacy behavior as something to describe accurately;
  record unrelated bugs separately instead of fixing them in these passes.
- Keep imported library notices and existing technical explanations intact;
  do not perform a blanket annotation of vendored `dbopl.c`.
- Review active and alternative version branches before making universal claims.
- Verify raw C tokens and actual preprocessed units, including the translation
  units that own textually included renderer files. Normalize diagnostic
  `__LINE__` expansion only after checking it is not used by functional logic.
- Use existing builds and tests; no new tests merely to verify comment presence.
- Record missing fixtures, untested platforms and pre-existing failures honestly.
- Each pass is complete when its complex paths have useful, accurate comments,
  the executable-token comparisons match, its checks pass relative to baseline,
  and its subsystem commit is recorded.

## Execution

Use the previously selected native execution method in the existing worktree.
This document and its plan authorize no immediate source implementation: the
current request is to prepare the plan. Retain the existing code-review and
verification workflow when execution is requested.
