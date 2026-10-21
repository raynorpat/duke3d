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
