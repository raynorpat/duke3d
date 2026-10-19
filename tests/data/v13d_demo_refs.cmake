# Final game state of each original Duke Nukem 3D 1.3D demo, measured from the DOS
# executables. These are numbers only; the demos, GRPs and EXEs stay out of the tree.
#
# Captured by running a patched scratch copy of each DUKE3D.EXE in DOSBox-X. The patch
# makes playback dump reccnt, randomseed and sprite[] when the demo ends (see
# docs/v13d/reference.md, "Demo reference capture"). X/Y/Z/ANG/HEALTH are the single
# player sprite's (picnum APLAYER, statnum 10) x, y, z, ang and extra after exactly
# _TICS tics. RANDOMSEED is the engine's randomseed at that point.

set(V13D_DEMO_FULL_1_TICS 7031)
set(V13D_DEMO_FULL_1_X 61066)
set(V13D_DEMO_FULL_1_Y 6704)
set(V13D_DEMO_FULL_1_Z 4196)
set(V13D_DEMO_FULL_1_ANG 1060)
set(V13D_DEMO_FULL_1_HEALTH 0)
set(V13D_DEMO_FULL_1_RANDOMSEED 1681479628)

set(V13D_DEMO_FULL_2_TICS 11269)
set(V13D_DEMO_FULL_2_X 25839)
set(V13D_DEMO_FULL_2_Y -46115)
set(V13D_DEMO_FULL_2_Z 54525)
set(V13D_DEMO_FULL_2_ANG 481)
set(V13D_DEMO_FULL_2_HEALTH 0)
set(V13D_DEMO_FULL_2_RANDOMSEED 3746032333)

set(V13D_DEMO_SW_1_TICS 2164)
set(V13D_DEMO_SW_1_X 2733)
set(V13D_DEMO_SW_1_Y 30585)
set(V13D_DEMO_SW_1_Z 7938)
set(V13D_DEMO_SW_1_ANG 1578)
set(V13D_DEMO_SW_1_HEALTH 57)
set(V13D_DEMO_SW_1_RANDOMSEED 3141611328)

set(V13D_DEMO_SW_2_TICS 3325)
set(V13D_DEMO_SW_2_X 12059)
set(V13D_DEMO_SW_2_Y 15690)
set(V13D_DEMO_SW_2_Z -53342)
set(V13D_DEMO_SW_2_ANG 1512)
set(V13D_DEMO_SW_2_HEALTH 87)
set(V13D_DEMO_SW_2_RANDOMSEED 1672040291)

set(V13D_DEMO_SW_3_TICS 1582)
set(V13D_DEMO_SW_3_X 18235)
set(V13D_DEMO_SW_3_Y 58789)
set(V13D_DEMO_SW_3_Z 7936)
set(V13D_DEMO_SW_3_ANG 705)
set(V13D_DEMO_SW_3_HEALTH 0)
set(V13D_DEMO_SW_3_RANDOMSEED 2967469797)
