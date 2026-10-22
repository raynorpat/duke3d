# Beta scenario checks

These 37 no-god prefixes were compared with original DOS beta playback on 2026-10-22.
Every tic count, player sprite position/angle/health and RNG seed agrees. All active
sprite records also agree, except the heatsensor case’s player yoffset (native 0,
DOS 30), a display field for the face-aligned player sprite. No unexplained
active-sprite differences remain. These checks found no additional desync.

The short enemy cases confirm script entry, including aliases and script-only
identities; they do not establish full AI branch coverage. Pickup checks require
a resource grant, and weapon checks require an actual attack. Empty-ART script
identities use a replacement in a scratch GRP, rather than an invented map placement.

## Reproduction

Use original beta data, the existing `tools/v13d/startnear.py` and `gendemo.py sw099`.
For each row, make a new scratch directory, relocate the listed map start near
`picnum,index`, and supply the replacement picnum only when shown. Generate 200
records at volume 0, level=map-1, the stored skill, seed and optional weapon key.
Then change only the little-endian record count in bytes 0–3 to N; retain the body.
This preserves the exact measured prefix. Link the original three CON files into
the scratch directory. Run native `-demoverify 1` without `-demogod`; run a scratch
DOS capture as described in [reference.md](reference.md#dos-capture-and-save-tooling-task-7).
The stored skill is passed directly to gendemo; CLI `/sN` instead stores N-1.
The seed here controls generated input; it does not replace the game RNG initializer.

A concrete stored skill 1 example:

```sh
python3 tools/v13d/startnear.py "$data/DUKE3D.GRP" "$work/duke3d.grp" E1L1.MAP 1680 0
python3 tools/v13d/gendemo.py sw099 "$work/demo1.dmo" 0 0 1 200 1
python3 -c 'import pathlib,struct,sys; p=pathlib.Path(sys.argv[1]); b=bytearray(p.read_bytes()); struct.pack_into("<I",b,0,103); p.write_bytes(b)' "$work/demo1.dmo"
```

## Measured prefixes

Anchor is `map: picnum,index`; replace is optional. Key is the ordinary weapon
selection number passed to gendemo. State is `N X Y Z ANG HP RANDOMSEED`.

| Check | Anchor | Replace | Skill | Seed | Key | DOS state |
|---|---|---|---|---|---|---|
| damage-skill1 | E1L1: 1680,0 | — | 1 | 1 | — | `103 25766 39162 8064 624 91 18917096` |
| damage-skill3 | E1L1: 1680,0 | — | 3 | 1 | — | `79 25846 39139 8064 1912 88 1309473148` |
| damage-skill4 | E1L1: 1680,0 | — | 4 | 1 | — | `79 25846 39139 8064 1912 88 1309473148` |
| death | E1L1: 1680,0 | 2630 | 4 | 2 | — | `112 26332 39402 7840 1158 0 3803987782` |
| enemy-bigdummyduke | E1L1: 1680,0 | 1430 | 2 | 1 | — | `1 26079 39136 7936 0 100 3627281966` |
| enemy-boss1stayput | E1L1: 1680,0 | 2631 | 2 | 1 | — | `1 26079 39136 7936 0 100 911283842` |
| enemy-boss3 | E1L1: 1680,0 | 2710 | 2 | 1 | — | `1 26079 39136 7808 0 100 3627281966` |
| enemy-dummyduke | E1L1: 1680,0 | 1425 | 2 | 1 | — | `1 26079 39136 7936 0 100 3627281966` |
| enemy-egg | E1L1: 1680,0 | 675 | 2 | 1 | — | `1 26079 39136 7936 0 100 3782089111` |
| enemy-fatguy | E1L1: 1680,0 | 1920 | 2 | 1 | — | `1 26079 39136 7808 0 100 3782089111` |
| enemy-jellyfishstayput | E1L4: 1821,1 | — | 2 | 1 | — | `1 -13377 32384 -85130 0 100 2250581973` |
| enemy-lizman | E1L1: 1680,0 | 2120 | 2 | 1 | — | `1 26079 39136 7808 0 100 3627281966` |
| enemy-lizmanstayput | E1L1: 1680,0 | 2121 | 2 | 1 | — | `1 26079 39136 7808 0 100 911283842` |
| enemy-liztroopontoilet | E1L1: 1741,0 | — | 2 | 1 | — | `1 29183 44608 3776 0 100 3627281966` |
| enemy-liztrooprunning | E1L1: 1680,0 | 1681 | 2 | 1 | — | `1 26079 39136 7936 0 100 3627281966` |
| enemy-organtic | E1L1: 1680,0 | 2420 | 2 | 1 | — | `1 26079 39136 7808 0 100 3782089111` |
| enemy-snakedrone | E1L1: 1680,0 | 1880 | 2 | 1 | — | `1 26079 39136 7808 0 100 3201154380` |
| pickup-airtank | E1L3: 56,0 | — | 2 | 1 | — | `7 29504 11365 31024 0 100 60560886` |
| pickup-ammolots | E1L1: 1680,0 | 48 | 2 | 1 | — | `7 26063 39136 7846 0 100 3756951448` |
| pickup-bodysuit | E1L1: 1680,0 | 58 | 2 | 1 | — | `7 26063 39136 7846 0 100 3756951448` |
| pickup-boots | E1L3: 61,0 | — | 2 | 1 | — | `7 31087 20608 -29018 0 100 1991880894` |
| pickup-cola-alt | E1L1: 52,0 | 51 | 2 | 2 | — | `184 14862 43753 -2560 424 97 3110139248` |
| pickup-crystalammo | E1L1: 1680,0 | 46 | 2 | 1 | — | `7 26063 39136 7846 0 100 3756951448` |
| pickup-firstaid | E1L1: 53,0 | — | 2 | 1 | — | `4 31224 43136 -10496 0 100 3627281966` |
| pickup-firstgunsprite | E1L1: 21,0 | — | 2 | 1 | — | `7 15311 43552 7846 0 100 3845067376` |
| pickup-fuelcan | E1L1: 1680,0 | 45 | 2 | 1 | — | `7 26063 39136 7846 0 100 3756951448` |
| pickup-heatsensor | E1L2: 59,0 | — | 2 | 1 | — | `7 1359 3200 6656 0 100 2412808987` |
| pickup-heavyhbomb | E1L3: 26,0 | — | 2 | 1 | — | `9 47826 22119 -11294 0 100 2228993341` |
| pickup-holoduke-alt | E1L1: 1680,0 | 1348 | 2 | 1 | — | `7 26063 39136 7846 0 100 3756951448` |
| pickup-jetpack | E1L1: 57,0 | — | 2 | 1 | — | `7 15343 46336 -61778 0 100 3714643279` |
| pickup-sixpak | E1L1: 52,0 | — | 2 | 2 | — | `184 14862 43753 -2560 424 100 2020999345` |
| pickup-steroids | E1L1: 55,0 | — | 2 | 1 | — | `7 23051 50524 -43354 0 100 3880949854` |
| weapon0 | E1L1: 1680,0 | — | 2 | 1 | 1 | `65 25883 39136 8064 1060 100 75344967` |
| weapon6 | E1L1: 23,0 | 25 | 2 | 1 | 7 | `37 19877 48567 -54784 1588 90 2716965949` |
| weapon7 | E1L1: 23,0 | 24 | 2 | 1 | 8 | `29 19877 48512 -54784 0 90 976011374` |
| weapon8 | E1L1: 23,0 | 27 | 2 | 1 | 9 | `43 19877 48568 -54528 1220 76 1331465076` |
| weapon9 | E1L1: 47,1 | — | 2 | 3 | 6 | `89 14816 47040 -53468 320 100 4149732615` |
