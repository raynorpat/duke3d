# Beta demo routes

Configure with `-DDUKE3D_VERSION=0.99 -DDUKE3D_SHAREWARE=ON
-DDUKE3D_ROUTEBOT=ON`. The bot is off by default and is not built in CI.
Run in a scratch data directory, for example:

```
duke3d /v1 /l1 /s3 -routebot e1l1.route -r
```

`/s3` selects the beta's normal skill (stored as 2). The bot runs the normal
simulation without drawing frames, feeds each input through the normal demo
writer, and closes `demo1.dmo` when the route finishes or the level ends. A death
or a waypoint that cannot be reached in 60 seconds returns exit 2 and preserves
the partial recording for diagnosis. `ROUTESTATE` reports the final player sprite
state in the same order as `DEMOSTATE`, followed by whether the level ended.

Each nonblank line is an absolute map coordinate:

```
x y
x y use
x y fire [tics]
x y jump
x y crouch
x y wait tics
x y weapon 1-10
```

Comments start with `#`. Coordinates within 128 map units count as reached.
The bot moves toward each point and turns to face it. `use` pulses the use key
while approaching and presses it on arrival; `fire` holds fire; `jump` pulses
jump once per second and on arrival. `wait` stops at the point for the specified
positive number of simulation tics (26 per second). `crouch` holds crouch/dive
while approaching. `fire tics` keeps firing for that many tics on arrival.
`weapon n` selects the normal numbered weapon key (1 is kick, 2 pistol, 3 shotgun). Use points on both sides of
a doorway to guide movement through it. A route may contain at most 4096 points.

The bot authors input. Its result is not reference evidence: play the resulting
demo in the DOS beta and capture that state with `tools/v13d/mkcapture.py sw099`.

For a reference route that repeatedly dies, the plan permits `-demogod` in the
beta author and `-demoverify` runs. Patch its scratch DOS capture EXE with
`mkcapture.py sw099 ... 1 krandlog demogod` to initialize exactly the same god
state. Record this requirement with the demo; the header cannot store it.
