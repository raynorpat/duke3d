# Input improvements

Implement the seven behaviors described at
https://wiki.eduke32.com/wiki/Mouse_support_improvements, including legacy
joystick and SDL gamepad polling in the frame polling refactor.

## Approach

Keep the fixed simulation timer and existing version-specific gameplay.
Poll devices at frame cadence, accumulate input for simulation consumption,
and update local aiming without waiting for or interpolating between ticks.
The user approved this approach before implementation.

Moving simulation itself to frame cadence would affect physics, demos, and
network synchronization far beyond input. Polling more often while retaining
tick-interpolated aiming would leave the central latency problem unresolved.

## Changes and acceptance checks

1. Allow both mouse axes to contribute simultaneously. Verify diagonal
   movement and aiming against independent horizontal and vertical input.
2. Remove directional bias from signed shifts and preserve fractional motion
   through sensitivity scaling and simulation consumption. Verify tiny inputs
   of opposite signs cancel, including across multiple frames.
3. Preserve each wheel step and accept weapon cycling while a switch animation
   is underway. Verify rapid multi-step cycling, reversal, wrapping, and
   unavailable weapons using each release's weapon rules.
4. Keep vertical mouse aiming enabled by default, hold-to-aim disabled, and
   the mouse-aim toggle unbound. Preserve explicit saved user settings.
   Existing defaults already enable aiming and disable hold-to-aim; audit the
   bindings and reset paths rather than replacing them wholesale.
5. Maintain fractional yaw and pitch for local aiming and carry that precision
   into rendering. Audit the render interface as well as input storage:
   storing fractions alone does not solve visible stair steps if rendering
   immediately truncates them. Verify sub-unit input accumulates and affects
   the displayed view wherever the renderer supports that precision.
6. Apply vertical look in pitch-angle space, converting to the renderer's
   horizon/shear representation at its boundary. Verify equal input produces
   equal angular movement near the center and pitch limits, with correct
   inversion and clamping.
7. Separate device polling from tick consumption. Mouse deltas and wheel
   events accumulate exactly once; joystick/gamepad axes are sampled states
   integrated over elapsed time. Retain short button presses until consumed.
   Verify equivalent motion at different rendering rates and correct behavior
   when several simulation ticks occur between frames.

## Integration constraints

`faketimerhandler` currently invokes `getinput` only after its timer gate.
`CONTROL_GetInput` both consumes mouse deltas and polls gamepad and legacy
joystick devices. `displayrooms` interpolates yaw and horizon, including the
multiplayer predicted local view. The refactor must address all three layers.

Use one explicit frame polling path shared by all device types. Simulation
consumption must not poll again or double-apply immediate local camera motion.
Leave interpolation of world positions and remote players in place. Keep
gamepad deadzones, mappings, menu repeat, and hotplug behavior intact.

Reset pending deltas, fractional remainders, and elapsed-time state on focus
loss, menu/pause transitions, level changes, and camera ownership changes so
input does not leak into resumed gameplay. Avoid stale controller state after
disconnects or a switch between controller and legacy joystick handling.

Do not change savegame, demo, or network formats as an incidental consequence.
Check how existing formats quantize input before deciding where fractional
remainders and immediate camera state live. If accurate behavior requires a
format change, identify that separately before implementing it.

## Verification plan

1. Establish the existing CMake/CTest baseline and inspect release guards.
2. Add regression coverage for motion symmetry, diagonal input, fractional
   accumulation, queued wheel steps, frame/tick consumption, and virtual SDL
   joystick/controller input.
3. Build and run affected tests, then the available full suite. Compile the
   supported release configurations where the changes cross version guards.
4. Exercise live mouse/controller aiming when game data and devices are
   available; report any hardware-only checks that cannot be performed.

The unrelated untracked `tests/__pycache__/` is outside this work's scope.
