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
`-demodump file` writes the same diagnostic prefix as playback (remaining count,
RNG seed, native sprite array and native ps[0]), then the native sector and wall
arrays. The appended geometry helps inspect initialized doors and moving floors.

Each nonblank line is an absolute map coordinate:

```
x y
x y use [crouch]
x y fire [tics]
x y jump [tics]
x y crouch [tics]
x y jetpack
x y wait tics
x y weapon 1-10
x y face angle
x y pass
```

Comments start with `#`. Coordinates within 128 map units count as reached.
The bot moves toward each point and turns to face it. `use` pulses the use key
while approaching and presses it on arrival; `fire` holds fire; `jump` pulses
jump once per second and on arrival. `wait` stops at the point for the specified
positive number of simulation tics (26 per second). `crouch` holds crouch/dive
while approaching. Timed `jump`/`crouch` holds the key on arrival for the given
number of tics, including jetpack ascent/descent. `jetpack` presses the normal
jetpack toggle on arrival. `use crouch` presses use while crouching for low switches.
`fire tics` keeps firing for that many tics on arrival.
`weapon n` selects the normal numbered weapon key (1 is kick, 2 pistol, 3 shotgun).
`face angle` turns in place after reaching the point, using Build angles0–2047
(0 east,512 south,1024 west,1536 north), within8 angle units.
`pass` uses a384-unit arrival radius for flow checkpoints on conveyors/currents,
where external movement can prevent precise positioning. Use points on both sides of
a doorway to guide movement through it. A route may contain at most 4096 points.

The bot authors input. Its result is not reference evidence: play the resulting
demo in the DOS beta and capture that state with `tools/v13d/mkcapture.py sw099`.

For a reference route that repeatedly dies, the plan permits `-demogod` in the
beta author and `-demoverify` runs. Patch its scratch DOS capture EXE with
`mkcapture.py sw099 ... 1 krandlog demogod` to initialize exactly the same god
state. Record this requirement with the demo; the header cannot store it.

Regenerate completed references from the repository root:

```
tools/v099/mkrefdemos.sh /path/to/d3d_beta build-v099-routebot 1 2
```

Omit the level numbers to regenerate all six. DOSBox-X must be available, or
pass `--dosbox /path/to/dosbox-x`. Each route needing god mode has an exact
`# DEMOGOD` header. The script checks the five original data hashes, authors in
scratch, requires native playback to match, and then captures the DOS exit.
It installs a demo in the data folder's `refdemos/` and updates the committed
numbers only after an exact DOS/native match. Scratch captures and logs remain
under `/tmp/v099-ref-*` for inspection. The original EXE, GRP and CON files stay
read-only throughout.
