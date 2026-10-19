# Expected state of each DOS Duke Nukem 3D 1.3D reference save. The saves stay out of
# the tree, in <data dir>/refsaves/<_FILE> (see docs/v13d/reference.md, "DOS reference
# saves"). HEALTH, ARMOR, CURWEAPON, AMMO and LEVEL were read off the DOS status bar,
# not parsed from the file, so a layout mistake can't hide in both the reference and
# the loader. CURWEAPON is the internal index (status-bar slot minus 1: pistol 1, RPG 4);
# AMMO is the current weapon's. X/Y are the player sprite's, parsed from the file as a
# supplement.

set(V13D_SAVE_e1l1played_EDITION full)
set(V13D_SAVE_e1l1played_FILE e1l1played.SAV)
set(V13D_SAVE_e1l1played_HEALTH 100)
set(V13D_SAVE_e1l1played_ARMOR 0)
set(V13D_SAVE_e1l1played_CURWEAPON 1)
set(V13D_SAVE_e1l1played_AMMO 44)
set(V13D_SAVE_e1l1played_LEVEL E1L1)
set(V13D_SAVE_e1l1played_X -28188)
set(V13D_SAVE_e1l1played_Y 12136)

set(V13D_SAVE_e2_EDITION full)
set(V13D_SAVE_e2_FILE e2.SAV)
set(V13D_SAVE_e2_HEALTH 78)
set(V13D_SAVE_e2_ARMOR 0)
set(V13D_SAVE_e2_CURWEAPON 4)
set(V13D_SAVE_e2_AMMO 5)
set(V13D_SAVE_e2_LEVEL E2L1)
set(V13D_SAVE_e2_X -35940)
set(V13D_SAVE_e2_Y 42368)

set(V13D_SAVE_e3_EDITION full)
set(V13D_SAVE_e3_FILE e3.SAV)
set(V13D_SAVE_e3_HEALTH 75)
set(V13D_SAVE_e3_ARMOR 0)
set(V13D_SAVE_e3_CURWEAPON 1)
set(V13D_SAVE_e3_AMMO 31)
set(V13D_SAVE_e3_LEVEL E3L1)
set(V13D_SAVE_e3_X 31903)
set(V13D_SAVE_e3_Y -45961)

set(V13D_SAVE_e1l1start_EDITION full)
set(V13D_SAVE_e1l1start_FILE e1l1start.SAV)
set(V13D_SAVE_e1l1start_HEALTH 100)
set(V13D_SAVE_e1l1start_ARMOR 0)
set(V13D_SAVE_e1l1start_CURWEAPON 1)
set(V13D_SAVE_e1l1start_AMMO 48)
set(V13D_SAVE_e1l1start_LEVEL E1L1)
set(V13D_SAVE_e1l1start_X -31243)
set(V13D_SAVE_e1l1start_Y 7160)

set(V13D_SAVE_e1l1fight_EDITION full)
set(V13D_SAVE_e1l1fight_FILE e1l1fight.SAV)
set(V13D_SAVE_e1l1fight_HEALTH 100)
set(V13D_SAVE_e1l1fight_ARMOR 100)
set(V13D_SAVE_e1l1fight_CURWEAPON 1)
set(V13D_SAVE_e1l1fight_AMMO 61)
set(V13D_SAVE_e1l1fight_LEVEL E1L1)
set(V13D_SAVE_e1l1fight_X 22047)
set(V13D_SAVE_e1l1fight_Y 57119)

# The user switched to the shotgun after saving, so the status bar showed the shotgun;
# the save itself holds the pistol. AMMO is the pistol's count from the weapons list.
set(V13D_SAVE_inventory_EDITION full)
set(V13D_SAVE_inventory_FILE inventory.SAV)
set(V13D_SAVE_inventory_HEALTH 136)
set(V13D_SAVE_inventory_ARMOR 70)
set(V13D_SAVE_inventory_CURWEAPON 1)
set(V13D_SAVE_inventory_AMMO 55)
set(V13D_SAVE_inventory_LEVEL E1L1)
set(V13D_SAVE_inventory_X 30341)
set(V13D_SAVE_inventory_Y 42178)

# Shareware saves live in the shareware data dir's refsaves/.
set(V13D_SAVE_sw_e1l1start_EDITION sw)
set(V13D_SAVE_sw_e1l1start_FILE e1l1start.SAV)
set(V13D_SAVE_sw_e1l1start_HEALTH 100)
set(V13D_SAVE_sw_e1l1start_ARMOR 0)
set(V13D_SAVE_sw_e1l1start_CURWEAPON 1)
set(V13D_SAVE_sw_e1l1start_AMMO 48)
set(V13D_SAVE_sw_e1l1start_LEVEL E1L1)
set(V13D_SAVE_sw_e1l1start_X -31243)
set(V13D_SAVE_sw_e1l1start_Y 7160)

set(V13D_SAVE_sw_inventory_EDITION sw)
set(V13D_SAVE_sw_inventory_FILE inventory.SAV)
set(V13D_SAVE_sw_inventory_HEALTH 47)
set(V13D_SAVE_sw_inventory_ARMOR 83)
set(V13D_SAVE_sw_inventory_CURWEAPON 2)
set(V13D_SAVE_sw_inventory_AMMO 10)
set(V13D_SAVE_sw_inventory_LEVEL E1L1)
set(V13D_SAVE_sw_inventory_X 26012)
set(V13D_SAVE_sw_inventory_Y 40691)
