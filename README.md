# Planepoints
This program reads entity data from Titanfall 2 maps and creates a display of trigger boundaries.

The shape of triggers is stored as keyvalues that describe each plane in 4 numbers. The intersections of these planes are found to get the start and end point of each edge of the planes, which can then be put into a `DebugDrawLine()` call.

Created by Pinsplash, much help from OzxyBox.

# To generate cfg files
(Note: Once this project is running smoothly, this will be done for you.)

Extract the entity data with the [Titanfall VPK Tool](https://github.com/SenorGeese/Titanfall2/blob/master/tools/Titanfall_VPKTool3.4_Portable.zip). The important file will be named *`mapname`*`_script.ent`. You may also choose to take it from [this repository](https://github.com/Pinsplash/Titanfall2EntityLumps/) for convenience. Drag the file onto `planepoints.exe`.

Copy the commands from the text box and put them in a cfg file in `/Titanfall2/r2/cfg/`.

# Settings
You can specify a file when running the program to determine which entities have lines drawn for them and the characteristics of the lines. The program will also put every group of lines used to create a trigger's shape into its own section, which can easily be copied into another cfg file to view an entity in isolation. The cfg files that are in this repository were generated with the `settings.txt` file also in the repository.

These are the settings that exist:

* **default**: If "allow", all entities (including non-triggers) are allowed by default and exceptions (**disallow**) are made to prevent certain things from having lines made for them. If disallow, the reverse.
* **drawontop**: If "yes", makes lines draw in front of other things, even when they would normally be hidden behind something. If no, the opposite. Both options can be difficult in different ways.
* **drawtriggeroutlines**: If "yes", draws outlines of triggers. If no, then doesn't.
* **drawentcubes**: If "yes", draws cubes around the origins of entities (including triggers). If no, then doesn't.
* **duration**: Number of seconds for the lines to be visible. Note: you can immediately erase lines with the command `clear_debug_overlays`.
* **allow**: Adds a criterion to allow. Multiple can be defined. An entity only needs to match one **allow** criterion to be allowed. If **default** is "allow", this will re-allow an entity if it was blocked by a **disallow** criterion. Order does not matter.
* **disallow**: Adds a criterion to disallow. Multiple can be defined. An entity only needs to match one **disallow** criterion to be disallowed. If **default** is "disallow", this will re-dis-allow an entity if it was accepted by an **allow** criterion. Order does not matter.
* **must**: A criteria which is strictly required. Multiple can be defined. All **must** criteria need to be met for an entity to be allowed, even after accounting for **allow** and **disallow**.
* **avoid**: A criteria that cannot be allowed in any circumstance. Multiple can be defined. All **avoid** criteria need to *not* be met for an entity to be allowed, even after accounting for **allow** and **disallow**.

Allow and disallow criteria work as follows: A property to select by, and then potentially something that the value of the property must match. A * can be used to limit the filtering to only the characters up until that point in a value's string.

Explanations of the first few properties can be found [here](https://github.com/Pinsplash/Titanfall2EntityLumps/).

* **classname**
* **editorclass**
* **script_flag**
* **spawnclass**
* **targetname**: A name by which other things can refer to a specific instance of an entity. Multiple entities can share the same targetname if it is necessary to create an effect across multiple entities at once.
* **_istrigger**: Tells if the entity is a trigger. All entities with a `*trigger_brush_` keyvalue are considered triggers and can use the trigger outline display. There is no second part needed for this property.

Examples:

`"allow" "classname trigger_*"` will draw lines for all entities with a classname that starts with `trigger_`.

`"allow" "classname func_*"` will draw lines for all entities with a classname that starts with `func_`. Putting this together with the above line would mean that entities can have either trigger_ OR func_ to start their classname.

`"disallow" "spawnclass *"` will disallow all entities with any spawnclass set at all.

`"allow" "_istrigger"` will draw lines for all triggers. Putting this together with the above line when **default** is "allow" would mean that entities cannot have a spawnclass unless they are a trigger. To reiterate, the order of the lines does not matter.

`"disallow" "editorclass trigger_flag_set"` will disallow all entities with the editorclass `trigger_flag_set`.

`"disallow" "script_flag TitanOnElevator"` will disallow a certain trigger (a `trigger_flag_touching`) in `sp_beacon` with the given script flag. Putting this together with the above line would mean that entities must not have the editorclass `trigger_flag_set` NOR the script flag `TitanOnElevator`.

# To show in-game
Put the cfg file(s) in `/Titanfall2/r2/cfg/`.

Recommended to run the game with [VanillaPlus](https://northstar.thunderstore.io/package/NanohmProtogen/VanillaPlus/).

In the console, put `exec` followed by the name of the cfg file that matches with the appropriate map. The game must not be paused when running the command!

# Limitations
* Cannot reflect the current state of the trigger. (Disabled or not, exists currently or not.) This may be possible to overcome after thoroughly analyzing what is possible.
