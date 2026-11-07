reKing 1.0.3 - The Return of the King (US Xbox), recompiled for the PS5
=======================================================================

An unofficial fan port: the original Xbox game's code, statically recompiled into a native PS5 app. Not an emulator.
No game files are included: it plays your own copy of the US (NTSC-U) Xbox disc.

What you need
-------------
- A PS5 that runs homebrew, with ShadowMountPlus (it registers apps in /data/homebrew) and an FTP server.
- Your own disc image (.iso) of The Return of the King, US Xbox release (title EA-062). A full dump and a trimmed
  one, clean or media-patched, all work.
- Room on the console for the game's files (about as much as a trimmed disc image), and for the disc image itself
  until the first start has unpacked it (or put the image on a USB drive).

Install
-------
1. Unzip PPSA99063-<version>.zip on your computer. You get a folder named PPSA99063.
2. Over FTP, copy the whole PPSA99063 folder to /data/homebrew/ on the PS5.
   ShadowMountPlus adds a "reKing" tile to the home screen (give it about 30 seconds).
3. Over FTP, create the folder /data/rotk/ and copy your disc image into it, e.g. /data/rotk/rotk.iso.
   Or put it on a USB drive, in a folder named rotk or at the top of the drive.
4. Start the reKing tile. The first start unpacks the disc image into /data/rotk/game/ (a screen shows how far it
   is; it takes a few minutes, longer from a USB drive), then the game starts. From then on the game starts from
   that folder straight away, and you can delete the .iso.

Already have the disc's files (default.xbe and the Data folder)? Copy them to /data/rotk/game/ instead of an .iso,
or to rotk/game/ on a USB drive. If there isn't room to unpack the image, the game reads the .iso directly (a
notification says so) and you keep the .iso there.

If something is wrong, a screen says what (game not found, a damaged image, another game or another release of it);
press Cross to close it.

To update, copy the new PPSA99063 folder over the old one. Your saves, settings and the game's files in /data/rotk/
are kept.

Play
----
Start the reKing tile. Player 1 is the user who started it. For two players, sign in a second user on the PS5
(a guest is enough) and turn on their controller.

  Xbox            DualSense
  A B X Y         Cross Circle Square Triangle
  Black / White   R1 / L1
  Triggers        R2 / L2
  Start / Back    Options / touchpad click
  Stick clicks    L3 / R3

In-game menu
------------
Press L3 + R3 (both sticks in) at any time. The game stands still behind it. Move with the d-pad or left stick,
Cross to choose, L1 / R1 to change page, and Circle, Options or L3 + R3 to go back to the game.

  Picture   render resolution (1080p, 1620p, 4K), frame rate (30 or 60), V-Sync, frame rate counter, texture
            filtering and sharpness, smooth menus and HUD, sharper text and menus, field of view, draw distance,
            HUD at the screen edges, skipping videos and logos
  Controls  vibration, stick deadzone, trigger threshold
  Game      damage you take and deal, the game's own cheats

Changes apply at once and are saved when you close the menu, except those marked "(next start)".

Sharper text and menus (on by default)
--------------------------------------
The title, main menu, save and options screens' pictures are upscaled four times for a 4K screen.
The game's text can be redrawn from the PC version's larger fonts too, if you own The Return of the King for PC
(2003): copy that game's Data folder to /data/rotk/pc/Data/ on the PS5 (the files used are
Data/Game/GlobScen.scw and Data/Game/fro/fro03.scw, so those two are enough). The title and main menu then use the
PC game's larger pictures as well. Nothing from it is copied into the app; it is read from that folder at start.

PlayStation buttons
-------------------
The game shows PlayStation buttons: in the hints during play, in the menus' help bar (character select's "Help",
stage select's "Level Info"), "Press OPTIONS", and the save and load messages say PS5 where they said Xbox.
The help screen shown while a level loads (after character select) has a PlayStation-layout controller instead of
the Xbox one: a drawing of reKing's own, placed so the game's labels still point at the right buttons.

Settings file
-------------
/data/rotk/settings.toml holds every setting (the menu writes it too). Edit it over FTP while the game is closed.
The ones that matter on a PS5:

  [graphics] fps = "30"              the console's frame rate; "60" runs the game at 60 fps
  [graphics] skip_all_videos = true  any video can be skipped with Cross (on by default here)
  [graphics] skip_logos = true       the EA / New Line logos are passed over (on by default here)
  [display] render_percent = "100"   the drawing resolution: 100 is the full 4K output, 50 is 1080p
  [input]                            stick deadzone, trigger threshold, vibration
  [controls]                         which DualSense button presses each Xbox button

Some settings in the file are for the PC (window, keyboard, Discord, upscalers) and do nothing here.

Where things are
----------------
/data/homebrew/PPSA99063/   the app
/data/rotk/                 your disc image (until it is unpacked), settings.toml
/data/rotk/game/            the game's files, unpacked from the disc image at the first start
/data/rotk/pc/Data/         optional: the PC version's Data folder (sharper text)
/data/rotk/hdd/             saves
/data/rotk/logs/            the logs, only while /data/rotk/logging-on exists (see below)
/data/rotk/shadercache/     compiled shaders (safe to delete)

Reporting a problem
-------------------
No log is kept while you play. To find a problem, create an empty file named logging-on in /data/rotk/ over FTP and
start the game: it then writes /data/rotk/logs/rotkx_*.log and stderr.log. Say what you did and what happened, and
include the newest of both. Delete logging-on again afterwards. Never share game files or disc images.

Credits
-------
The Return of the King Xbox PC port from Athelas (Shagg), XboxXBE-Recomp (TGP482), reUndr's PS5 host, its first-start
unpacking and screen (bystritskiy),
PS5_Vulkan / PS5_Mesa / PS5_PayloadSDK (mihawk-99), Mesa RADV, Dear ImGui, glslang. Licences are in licenses/.
