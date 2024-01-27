# Planepoints
This program reads entity data from Titanfall 2 maps and creates a display of trigger boundaries.

The shape of triggers is stored as keyvalues that describe each plane in 4 numbers. The intersections of these planes are found to get the start and end point of each edge of the planes, which can then be put into a `DebugDrawLine()` call.

Created by Pinsplash, much help from OzxyBox.

# To generate cfg files
(Note: Once this project is running smoothly, this will be done for you.)

Extract the entity data with the [Titanfall VPK Tool](https://github.com/SenorGeese/Titanfall2/blob/master/tools/Titanfall_VPKTool3.4_Portable.zip). The important file will be named *`mapname`*`_script.ent`. Drag the file onto `planepoints.exe`.

Copy the commands from the text box. Put them in a cfg file in `/Titanfall2/r2/cfg/`.

# To show in-game
Put the cfg file(s) in `/Titanfall2/r2/cfg/`.

Recommended to run the game with [VanillaPlus](https://northstar.thunderstore.io/package/NanohmProtogen/VanillaPlus/).

In the console, put `sv_cheats 1`, `enable_debug_overlays 1`, and finally `exec` followed by the name of the cfg file that matches with the appropriate map.

# Limitations
* Cannot reflect the current state of the trigger. (Disabled or not, exists currently or not.) This may be possible to overcome after thoroughly analyzing what is possible.
