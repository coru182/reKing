# Changelog

## 1.0.4

- **Sharper text everywhere.** The title lettering that still looked low resolution is now drawn four times larger
  with crisp edges: the in-game dialogs ("are you sure?", "exit to menu", "your mission has failed") and messages
  such as "perfect mode!". So is any other text the PC version's fonts don't cover, and, without the PC version's
  Data folder, all of the game's text.

## 1.0.3

- **A PS5 controller on the loading screen.** The help screen shown while a level loads (after character select)
  showed the Xbox controller. It now shows a PS5 controller in the same place, with the same teal glow. The game's
  L2 / R2 labels and arrows still point at the right buttons.
- This works whether "Sharper text and menus" is on or off.
- For builders: `tools/mkcontroller.py --image` turns any front-view picture of a controller into that screen's
  picture (`art/controller.png`, which `build-ps5.sh` then builds in). Without one, the build uses the drawing in
  `ps5/art/`.

1.0.3 also includes everything in 1.0.2.

## 1.0.2

- **The first start installs the game, as reUndr does.** It unpacks your disc image into `/data/rotk/game/`, with a
  progress screen, and then the game starts. From then on the game starts straight away, and you can delete the
  `.iso`.
- The disc's `default.xbe` is copied and checked before anything else, so another game or another release is turned
  away in seconds instead of after copying gigabytes.
- When something is wrong, a screen says what it is: the game isn't found (with where to put it), the image is
  damaged, or it's another game or another release. Press Cross to close it.
- If the console has no room to unpack the image, the game plays from the `.iso` directly and a notification says so.
- Disc images are also found at the top of a USB drive. Files you unpacked yourself can go in `/data/rotk/game/` or
  in `rotk/game/` on a USB drive.

## 1.0.1

- One build runs every US dump: full or trimmed, clean or media-patched. The release check now compares the game's
  code block by block, where it used to need the exact same bytes.
- The code is on GitHub under GPL-3.0, with credits for every project it builds on.
- Releases come as a ready `PPSA99063` folder to download. A GitHub Actions workflow can build them too.
- The tile has an original crown icon. You can build your own home screen art in with `tools/mkart.py`.

## 1.0.0

- Nothing is logged while you play: no log line is formatted or written. To report a problem, create an empty
  `/data/rotk/logging-on` file and the logs come back.
- The save and load messages say PS5 where they said Xbox.
- The picture-capture tool from the test builds is gone.

## Test builds 0.1.0 to 0.1.6

- **0.1.0:** the first build. The US Xbox game's code was statically recompiled into a native PS5 app, with no
  emulator. It drew through Vulkan (Mesa RADV) at up to 4K, played with DualSense controllers and saved on the
  console.
- **0.1.1:** fixed the crash at start. Mesa's shader cache made a system call a PS5 app isn't allowed to make. The
  intro and the first level play.
- **0.1.2:** fixed hiccups during play, which came from log lines being written to disk on the game's thread. Added a
  choice of 30 or 60 fps. Every video can be skipped, and the EA and New Line logos are skipped.
- **0.1.3:**
  - The in-game menu (**L3 + R3**) has:
    - resolution (1080p, 1620p or 4K) and frame rate
    - V-Sync and an fps counter
    - texture filtering, field of view and draw distance
    - HUD placement
    - vibration, stick deadzone and trigger threshold
    - damage and the game's cheats
  - Sharper text and menus at 4K, with the PC version's larger fonts as an option.
- **0.1.4:** "Press START" reads "Press OPTIONS". The menu no longer closes the moment it opens.
- **0.1.5:** the blue X "Help" and the yellow Y "Level Info" in the menus' help bar are PlayStation buttons.
- **0.1.6:** sharper text is turned on for settings files kept from older builds.
- **Also along the way:**
  - Textures no longer lose their filtering after a few minutes of play.
  - The game's memory runs on the PS5's 16 KB pages.
  - The picture goes to VideoOut through Vulkan's display support.
  - L2 and R2 are named in the game's text.

## Thanks to

- Athelas by Shagg, the Return of the King Xbox port this one is built on
- XboxXBE-Recomp by TGP482
- reUndr-ps5 by bystritskiy, the model for this port, including the first-start unpacking and its screen
- mihawk-99's PS5_Vulkan, PS5_Mesa and PS5_PayloadSDK
- everyone else in the README's credits
