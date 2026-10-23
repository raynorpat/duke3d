# User maps from the episode menu

Status: approved by the user on 2026-10-23.

## Intent and scope

Let a single-player user select a custom map from a permanent **USER MAPS**
entry beneath the episode list, then choose difficulty and start playing.
The user chose a combined list and approved the browser layout mockup.
This is a small, focused menu feature using the existing C/SDL menu system.

## Approaches

- **Combined list (chosen):** merge files from all supported locations into
  one alphabetical list. Source labels distinguish matching filenames and
  avoid an extra navigation step.
- **Choose a location first:** smaller separate lists, but an additional
  step and no single overview. The user preferred the combined list.

## Discovery

Scan regular files directly in these four locations, without recursion:

| Label | Location |
| --- | --- |
| CURRENT | Current working directory |
| CURRENT / MAPS | `maps/` in the working directory |
| PROFILE | Existing user config/save directory |
| PROFILE / MAPS | `maps/` in the config/save directory |

Using both roots and both `maps/` subdirectories is my proposed interpretation
of the user's requested locations, also shown in the approved mockup.
Resolve PROFILE through the game's existing profile-path code, including
Windows overrides and fallback; do not hard-code OS paths or change the
working directory. Skip missing or inaccessible directories. Refresh on
opening the picker. Accept `.map` extensions case-insensitively, preserve
actual filename case, and sort by filename ignoring case, then source in
the table's order. Keep distinct files with identical names selectable;
avoid repeated entries when scan locations resolve to the same directory.

## Menu behavior

- Match the game's existing artwork, fonts, sounds, and input conventions.
  The mockup illustrates placement and flow rather than replacement artwork.
- Use a scrolling list with filename and source columns, a selected-path
  line, and position/total. Clip display text to fit without changing paths.
- Selecting a map leads to the existing difficulty page; confirming a skill
  starts that exact map with the usual single-player settings.
- Back from difficulty returns to the map list with the selected row visible;
  back from the list returns to USER MAPS in the episode menu.
- Selecting a stock episode uses the stock map, even after browsing custom
  maps. Merely opening or cancelling the picker does not select a map.
- An empty list shows **NO MAPS FOUND** and allows returning to episodes.
  A file removed or made unreadable before confirmation produces a menu
  message instead of attempting to start it.

## Constraints and proposed implementation calls

Keep the existing custom-map loading mechanism and difficulty behavior.
The old multiplayer picker has 17-byte records and a byte-sized count;
the new list must safely handle longer names and scrolling beyond that
picker's visible capacity without depending on those records. Leave the
multiplayer menu unchanged.

Preserve the 128-byte `boardfilename` field and save/demo formats. Any
selected path must fit, including its terminator; overlong paths remain
visible but cannot start and explain **MAP PATH TOO LONG**, never truncate.
Use relative paths for CURRENT and absolute paths for PROFILE so identical
filenames resolve to the chosen source. This path limit is my scope choice.

Expose the feature in registered/full versions (1.3D, 1.4, 1.5), preserving
the existing shareware restrictions. This version scope is my proposal.

## Non-goals and anti-goals

No arbitrary folder browser, recursive scanning, map downloads, map metadata
parser, multiplayer changes, or expanded map-format compatibility. Do not
change config locations, save/demo layouts, stock episode behavior, or the
process working directory. Verification must use temporary profile folders,
not write to the user's real config or map files.

## Success checks and remaining builder decisions

Verify discovery from all four temporary locations, mixed-case extensions,
matching names from different sources, overlapping roots, long filenames,
overlong paths, absent folders, and a list long enough to scroll. Exercise
episode -> maps -> difficulty and all back transitions through the menu
handler, including empty and disappearing-file cases and stock selection
after custom selection. Build full-version variants and check shareware menu
behavior remains unchanged. Where matching game data is available, launch
maps from each source and inspect the real menu visually.

The builder chooses the smallest list storage/scanning implementation and
exact row spacing consistent with the mockup and 320x200 menu coordinates.
Update README with the entry point, four locations, and path limit.
