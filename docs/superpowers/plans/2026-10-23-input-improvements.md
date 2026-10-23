# Input Improvements Implementation Plan

> **For agentic workers:** Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Complete the seven mouse improvements and include joystick/gamepad in frame polling.
**Architecture:** Separate device sampling from simulation consumption. Preserve serialized input and player structures; keep fractional local look in a sidecar and expose fractional camera rendering through the engine.
**Tech Stack:** C, SDL2, CMake, CTest.
**Spec:** docs/superpowers/specs/2026-10-23-input-improvements-design.md

## Global Constraints

Keep fixed simulation timing and existing demo/network/save formats. Preserve saved controls and version-specific gameplay. Leave unrelated untracked files alone.

## Review Focus

- Short presses between ticks must survive sampling.
- Held controls must survive release of another device mapped to the same action.
- Focus/menu/level transitions must clear pending motion.
- Wheel reversals and multiple steps must preserve ordering.
- Controller rotation must be independent of rendering rate.

### Task 1: Device sampling and wheel events

Files: source/control.c, source/control.h, source/gamepad.c, source/gamepad.h, source/build/sdl_driver.c, source/build/engine.h, tests/gamepad.c, tests/input_polling.c, tests/CMakeLists.txt.

Interfaces: CONTROL_PollDevices(void) samples devices; CONTROL_GetInput(ControlInfo *) consumes mouse motion and returns sampled joystick state. Engine input callbacks prepare context before pumping events, latch device press edges while processing them, and sample states afterward without linking the engine to the game. Wheel events are queued individually and consumed through CONTROL_GetWheelDirection(void).

- [x] Add regression tests for frame sampling, simultaneous axes, short presses, overlapping mappings, wheel ordering, and disconnect cleanup; run and observe failures.
- [x] Implement sampling and consumption separation with explicit reset and startup/shutdown callback ownership.
- [x] Run input tests and SDL smoke tests; expect all pass.

### Task 2: Precise immediate aiming

Files: source/inputaim.c, source/inputaim.h, source/player.c, source/game.c, source/build/engine.c, source/build/polymost.c, source/build/engine.h, source/CMakeLists.txt, tests/input_aim.c.

Interfaces: pure INPUT_AddLook(InputAim *, int64_t, int64_t, int32_t, double), INPUT_AddRate(InputAim *, int64_t, int64_t, uint32_t, int32_t, double), and INPUT_ConsumeLook(InputAim *, int, int32_t *, int32_t *) implement Q16/spherical look. CONTROL_ConsumeLook, CONTROL_QueueInput and CONTROL_AcknowledgeInput bridge frame samples, packetized FIFO entries and simulation/prediction. CONTROL_GetView supplies pending camera offsets. Engine setviewanglefraction(int32_t, int32_t) supplies fractional yaw/horizon for a draw pass.

- [x] Add failing tests for signed tiny motion, fractional accumulation, spherical pitch, frame-rate-independent stick look, and resets.
- [x] Implement local look accumulation, consuming whole simulation units exactly once while retaining fractional residuals. Use current local view rather than interpolating live look; preserve demo and remote interpolation.
- [x] Carry fractions into renderer orientation and horizon calculations; verify integer calls remain unchanged.
- [x] Run input/renderer regression tests and compile release variants.

### Task 3: Weapon cycling and defaults

Files: source/sector.c, source/player.c, source/_functio.h, source/premap.c, source/savegame.c, tests/input_polling.c, tests/input_defaults.c.

- [x] Add failing tests for cycling during weapon lowering/raising, ordered wheel steps, and unbound mouse-aim toggle.
- [x] Permit next/previous selection while switching, retain each queued step, and preserve unavailable-weapon rules. Clear pending input on level/load boundaries.
- [x] Run full CTest suites and review the whole diff; record hardware-only limits.

## Implementation decisions and verification

- Keep original player, save, demo and network record layouts. The local camera
  retains Q16 precision; simulation input remains quantized to the original
  transport fields, with remainders carried forward.
- New discrete weapon-cycle commands use reserved values 13/14. Original 11/12
  retain their historical behavior: changing them broke DOS demo compatibility.
  Multiplayer peers must use a build that understands the new commands.
- Live level resets preserve configured mouse look; original demo reset paths
  remain historical.
- Independent review identified batched event loss, packetized FIFO accounting,
  and legacy strafe routing. Each received a failing regression test and fix.
- Physical-device latency/feel is not established by these automated tests.
- Two 1.1 multiplayer connect/desync tests also fail on unchanged HEAD, confirmed
  using a separate baseline build; no unrelated multiplayer fix is included.

Spherical pitch uses the renderer's actual horizon focal scale, obtained through
`getviewhorizscale()`, rather than assuming EDuke32's projection constant. An
end-to-end mouse regression at center/high aim reproduced and verifies this.

Final validation: all seven supported configurations built. The full suites
passed 317 test runs after the review fixes. After projection calibration,
1.5 passed its full 34-test suite and each other release passed all six input
regressions (0.99, 1.0, 1.1, 1.3D full/shareware, 1.4). The 1.3D full UDP
connection test also passed. The two 1.1 UDP desync failures were reproduced on
unchanged HEAD and are outside this branch's scope.
