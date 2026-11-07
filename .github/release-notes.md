**The Lord of the Rings: The Return of the King (US Xbox), statically recompiled into a native PS5 app.**

You need a PS5 that runs homebrew, with ShadowMountPlus and an FTP server, and your own disc image (.iso) of the US
Xbox release.

1. Unzip `PPSA99063-<version>.zip` and copy the `PPSA99063` folder to `/data/homebrew/` over FTP. A "reKing" tile
   appears on the home screen after about 30 seconds.
2. Copy your disc image to `/data/rotk/` (for example `/data/rotk/rotk.iso`), or to a `rotk` folder on a USB drive.
   A full dump and a trimmed one, clean or media-patched, all work.
3. Start the tile. The first start unpacks the image into `/data/rotk/game/` (a screen shows the progress), then the
   game starts. After that it starts straight away, and you can delete the `.iso`.

Press L3 + R3 at any time for the settings menu. To update, copy the new `PPSA99063` folder over the old one: your
saves, settings and the game's files in `/data/rotk/` are kept.

`README.txt` in the app has the details. No game files are included.
