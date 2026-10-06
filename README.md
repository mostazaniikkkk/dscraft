# dscraft
minecraft adaptation for nintendo DS


all written in summer 2011


feel free to use this (terrible) code for non-commercial purposes as long as you give credit to original authors; this repo includes some libraries i didn't write myself like lodepng, iniparser and pcx loader


## The Survival Update

Survival mode, timed mining and tools, Minecraft-style inventory and
crafting, dropped items, the world generator (including Superflat), world
management and fixes: "The Survival Update" by mostazaniikkkk. The in-game
credits are under "Credits" in the main menu.

### Bug fixes

Engine and menu:
- Builds again with a current devkitARM (libnds 1.8.3, libfat 1.1.5); the map
  streaming code no longer needs a patched libfat, and the missing math
  library is linked.
- Going out of the world and breaking a block no longer crashes the game
  (issue in the original repo): the world has a border, positions outside it
  read as air, and the cursor never targets them.
- The menu draws each screen's scene on its own screen: part of the top
  screen no longer shows on the bottom one, and the screens are not swapped.
- Creating a world no longer flickers between the screens: only the top
  screen is drawn while it is generated.
- The splash screens before the title screen are gone.
- A texture that does not fit in video memory is no longer written over the
  start of memory (which left the menu black); the menu uses a smaller block
  atlas.
- Removing a torch whose light had not been worked out yet no longer darkens
  the blocks around it.
- Plants drawn as crossed planes (torches excepted) are shaded like the rest
  of the world instead of always at full light, at night and in caves too.
- Screenshots no longer crash when the `screens` folder is missing, and are
  written a row at a time.
- Typed seeds: -1 is no longer the same world as 0.

Playing:
- A block is used up only when it is really placed (not when it is refused
  under the player), and blocks can be placed while jumping.
- The place button only acts on the block the cursor points at: the cursor
  no longer keeps an old block once it points at nothing.
- Fall damage counts the whole fall (it was measured one step late).
- Dropped items no longer scatter as far as they did.
- Netherrack needs a pickaxe, soul sand prefers a shovel and glowstone breaks
  quickly (they were treated as wooden blocks).
- The chest and furnace screens keep the window's frame (borders and the line
  above the inventory).

## Building

The game needs devkitARM with libnds 1.x and libfat 1.1.x (it uses libfat
internals), so the provided Docker image pins the last devkitPro release
before libnds 2.0:

    docker build -t dscraft-build .
    docker run --rm -v "$PWD:/dscraft" dscraft-build make

This produces `dscraft.nds`. Copy the `dscraft/` folder (texture packs and
worlds) to the root of the SD card. Pass `ARCH=-DDEBUGMODE` (or `DEBUGMODE2`,
`FATONLY`) to `make` to enable the optional debug builds.

The bottom-screen backgrounds (crafting table, furnace, chest, game menu) can
be edited as PNG: `art/gui/*.png` (256x192, transparent = colour 0). After
editing, `python tools/gui_png.py import furnace` (or `crafting`, `chest`,
`pause`) writes `nitrofiles/dscraft/<name>.bin`; `python tools/gui_png.py
export` writes the PNGs again. Each screen has 256 colours: new colours take
free palette entries, and only when none are left the nearest colour is used.
Remove `dscraft.nds` before `make` so the new background is packed (make does
not notice changes in `nitrofiles/`). The `make_*_bin.py` scripts rebuild
those backgrounds from `inventory.bin` and would overwrite hand edits.

## Worlds

The Singleplayer screen lists the worlds (`dscraft/worlds/*.map`, shown without
the extension).

- "New world" opens the world options: the name and the seed (tap them to
  type with the on-screen keyboard; Shift switches case), the game mode (Survival or
  Creative, fixed for the life of the world) and "Superflat", Minecraft's
  classic flat world: bedrock, two layers of dirt and grass, no trees or water.
  "Create" generates a 256x256 world on the SD card (about 8 MB) named after
  the world (`My World.map`, or `My World 2.map` if the name is taken).
  The seed works like Minecraft's: a number is used as it is, other text by
  its hash, and the same seed always gives the same world. Left empty, the
  seed is random (mixing the clock with the frames since start-up and the
  stylus, so worlds differ even when the console clock does not work).
  Normal worlds have Minecraft Beta's bedrock, coal ore veins in the stone,
  iron ore veins in its lower half (Beta's density: about half as much as coal),
  patches of pumpkins and of wild carrots on farmland (at least one of each),
  (Beta's vein generator, same density), (a full bottom layer, then
  thinning out up to height 4), hills, lakes and sea, sand beaches and oak forests, with
  the spawn point on dry land. Progress is shown while it is written; "Back"
  cancels and removes the partial file.
- "Delete" switches the list to delete mode: tap a world, then confirm
  (its chests and furnaces go with it).
  Worlds built into the ROM cannot be deleted.
- What chests and furnaces hold is kept next to the world in `<name>.chests`
  and `<name>.furnaces`, written whenever the world is saved.

Generator tests run on the host: `sh tests/worldgen/run.sh` generates worlds
and checks every precalculated face, light and occlusion byte against the
game's own rules.

## Moving and the game menu

- START opens the game menu (the game stands still): back to the game, a
  screenshot (saved to `dscraft/screens/` on the SD card), or save and quit
  to the title screen. An open inventory or chest closes first.
- Hold SELECT to sneak (Minecraft 1.8): 0.3 of the speed, the view drops a
  little, you do not step off edges, steps make no sound, and with something
  in hand you place against chests, furnaces, crafting tables and doors
  instead of using them.
- Press forward twice quickly to sprint (x1.3, the view widens); it stops when
  you let go, sneak, enter water or run into a wall.
- Creative: jump twice quickly to fly or to stop flying. Hold jump to go up
  (scheme 1: keep the stylus down after the jump's double tap; schemes 2 and
  3: hold A) and SELECT to go down; touching the ground lands. Flight keeps
  some momentum, about 11 blocks a second, twice when sprinting. Holding
  A+B+X+Y still toggles the old free-camera mode that goes through blocks.

## Creative mode

The inventory works like Minecraft's creative inventory: a catalogue with every
block (scroll it with the arrows on the right) and the item bar. Drag a block
from the catalogue to the bar to copy it, drag between bar slots to swap them,
and drop a bar slot on the catalogue to empty it.

## Survival mode

Survival or creative is chosen when a world is created ("New world" on the
Singleplayer screen) and belongs to that world: it cannot be switched later.
Survival worlds carry their survival data in the map header; worlds without it,
like the bundled ones, are creative.

- Hold the dig button to mine (R or L in destroy mode, R in scheme 2, Y in
  destroy mode in scheme 3). The cursor turns red as the block gives way.
  Break times follow Minecraft: hardness, the right tool and its material.
- Mined blocks drop as small spinning items (grass gives dirt, stone gives
  cobblestone, coal ore gives coal). Walk over them to pick them up. Stone and
  coal ore need a pickaxe to drop anything; bedrock cannot be broken. Placing a block uses one.
- At most 5 items lie on the ground: a sixth one makes the oldest disappear.
  Drops of the same item within a block of each other merge into one stack,
  and drops vanish after 5 minutes. They are not saved with the world.
- Inventory and crafting follow Minecraft Beta 1.7: 36 slots (9 in the item
  bar), stacks of 64 (tools and doors: 1). Open it with "Inventory".
  - Tap a slot to pick a stack up, tap another to put it down, merge or swap.
    Dragging a stack to one slot moves it there.
  - Drag over several slots to spread the stack (Minecraft 1.5+): evenly, or
    one item per slot while holding L. To make a crafting table, pick up
    planks and drag over the four grid cells with L held.
  - Hold L while tapping for a right click: take half a stack, or put one item.
  - Hold R while tapping for a shift click: move a stack between the item bar
    and the inventory, or craft as many as possible from the result slot.
  - Tap outside the window (left or right edge) to throw the held stack
    (with L: one item). Thrown items can be picked up again after 2 s.
  - Crafting: place ingredients in the 2x2 grid of the inventory, or in the
    3x3 grid of a crafting table (use it with the place button). Shapes
    follow Minecraft and mirrored shapes work. Recipes: log -> 4 planks,
    2 planks stacked -> 4 sticks, 2x2 planks -> crafting table, wooden and
    stone pickaxe / shovel / axe, ladder (7 sticks -> 2), door (6 planks),
    torch (coal or charcoal on a stick -> 4), chest (8 planks around an empty
    centre), furnace (8 cobblestone around an empty centre), wooden and stone
    hoe, pumpkin -> 4 pumpkin seeds, and iron pickaxe / shovel / axe / hoe
    and iron armour from iron ingots.
  - Closing the window puts the grid back in the inventory; what does not fit
    is thrown.
- Chests: 27 slots, opened with the place button. Two chests side by side
  join into a double chest (54 slots, shown as two pages: the arrows on the
  right switch them). As in Minecraft, a chest cannot be placed next to a
  double chest or between two chests, and does not open with a solid block
  on top. Its front faces you when placed; a second chest turns to match the
  first. In the chest screen, tap / drag / L work as in the inventory and R
  (shift) moves a stack between the chest and the inventory. Breaking a chest
  (hardness 2.5, faster with an axe) drops it and everything in it, subject
  to the 5-item limit on the ground. In creative a chest is only a block:
  creative has no item stacks to put in it.
- Furnaces (Minecraft Beta 1.7): use one to open it. Put something to smelt
  on top and fuel below; the result comes out on the right. Smelting takes
  10 s per item: cobblestone -> stone, sand -> glass, log -> charcoal, iron
  ore -> iron ingot. Fuel:
  coal and charcoal 80 s (8 items), wooden blocks (planks, logs, crafting
  table, chest) 15 s, sticks 5 s. The flame and the arrow show the fuel left
  and the progress. A burning furnace shows its lit front and gives light,
  and keeps working with its screen closed while its area is loaded. Shift
  (R) takes the result to the item bar, or puts input and fuel back in the
  inventory. Breaking it (hardness 3.5, a pickaxe to keep it) drops it and
  its contents. Charcoal looks like coal in Beta; it is drawn browner here,
  as in later versions, since the DS shows no item names. In creative a
  furnace is only a block.
- Leaves and saplings: broken leaves never drop themselves (there are no
  shears); one time in 20 they drop a sapling (Beta) and one time in 200 an
  apple (oak leaves, Minecraft 1.0). When a log or a leaf goes, the natural
  leaves around it check for a log within 4 steps through leaves; those
  without one decay over the next seconds and drop the same way. Leaves
  placed by the player never decay. Plant a sapling on grass or dirt: with
  light 9 or more above it (daylight, or a torch close by) it grows in two
  random-tick stages (about 10 minutes each, as in Minecraft) into an oak, or
  one time in 10 into a big oak with branches. Without sky or light 8 it pops
  off. The game's light is per face, so Minecraft's light levels are
  estimated: sky through the column above (leaves -1, water -3) dimmed by the
  time of day, and torches (14) and lit furnaces (13) by distance.
- Apples (Beta food): use the place button to eat one, it heals 2 hearts;
  they do not stack.
- Farming (Minecraft 1.4): use a hoe on grass or dirt to make farmland. It
  is wet while water is within 4 blocks (same level or one up); away from
  water it dries and, with nothing planted, turns back to dirt. Falling on it
  can trample it. Plant carrots or pumpkin seeds on top of it: they grow in 8
  stages with light 9 or more, much faster on wet farmland, slower with the
  same crop all around. A grown carrot gives 1 to 4 carrots; a carrot is also
  eaten (1.5 hearts) and stacks to 64. A grown pumpkin stem grows a pumpkin
  next to it on farmland, dirt or grass and bends towards it; stems drop
  seeds by stage. Carrots, stems and seeds have pictures drawn for this game
  (the Beta packs have none); crops are drawn as crossed planes, since the
  engine places vertices on half-block steps (Minecraft's # shape needs
  quarters), and farmland is a full block.
- Iron armour (Minecraft Beta 1.7): helmet (5 ingots), chestplate (8),
  leggings (7) and boots (4), worn in the four slots on the left of the
  inventory screen (each slot takes only its piece). Every armour point takes
  away 4% of the damage (falls, drowning and the void too): 3, 8, 6 and 3
  points, 20 for the full set, less as the pieces wear. A hit wears each piece
  by a quarter of its damage (at least 1); they last 132, 192, 180 and 156
  hits. The armour bar shows above the item bar, on the right.
- Tools: wooden, stone and iron pickaxe, shovel and axe (and hoes). The tool
  in the selected slot is used. Wooden tools last 60 uses, stone 132, iron
  251; a bar on the icon shows the wear. Iron is faster (Beta's efficiency 6)
  and mines what needs an iron pickaxe (gold and diamond blocks). Iron ore
  needs a stone pickaxe or better and drops itself; smelt it into ingots. Tools and sticks are shown in hand and spin on the
  ground as flat items.
- Below the bedrock is the void. 64 blocks under the world is the death zone
  (as in Minecraft Beta): 4 damage every tick. Items that fall there are
  destroyed; creative players are brought back to the surface.
- Ten hearts sit above the item bar. Falls of more than three blocks and
  running out of air under water hurt; health regenerates after 4 s.
- On death the world is saved and you respawn where you first entered
  survival on that world. The inventory is kept.
- Survival state is stored in the unused part of the map header, so existing
  maps keep working. Creative play leaves it untouched.

Rule tests run on the host: `sh tests/survival/run.sh`.
