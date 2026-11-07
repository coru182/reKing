# reKing

**The Lord of the Rings: The Return of the King (US Xbox), statically recompiled into a native PS5 app.**

The original Xbox game's code is translated to C++ and compiled for the PS5, like
[reUndr](https://github.com/bystritskiy/reUndr-ps5) does for Need for Speed: Underground. It's not an emulator: it
draws through Vulkan (Mesa's RADV) at up to 4K and 60 fps, and plays with DualSense controllers.

> [!IMPORTANT]
> **Bring your own game.** This repository holds no game files and no game code, and there is no prebuilt app: the
> game's code is recompiled on your machine from the `default.xbe` of **your own** disc, and the app reads the
> game's files from your own disc image. Please don't ask for or share game files or builds made from them.

An unofficial fan project, not affiliated with or endorsed by Electronic Arts, New Line Cinema, Warner Bros., the
Tolkien Estate, Microsoft or Sony Interactive Entertainment.

## Features

- Native 4K output, rendered at 1080p, 1620p or 4K; 30 or 60 fps
- PlayStation buttons throughout: the button pictures in hints and menus (Cross, Circle, Square, Triangle), "Press
  OPTIONS", L2 / R2, and the save messages say PS5
- An in-game settings menu (**L3 + R3**): resolution, frame rate, V-Sync, fps counter, texture filtering, sharper
  menus, field of view, draw distance, HUD placement, skippable videos, vibration, deadzones, damage, cheats
- Every video can be skipped; the EA / New Line logos are skipped
- Sharper menus at 4K: the menus' full-screen pictures redrawn four times larger
- Two players (sign in a second user, a guest is enough), saves on the console
- No logging while you play

## Building

You need a Linux machine (WSL 2 with Ubuntu 24.04 works) with:

```sh
sudo apt install git cmake ninja-build clang-18 lld-18 llvm-18 python3 python3-pip patch zip
```

and Docker, for reUndr-ps5's PS5 toolchain. You also need `default.xbe` from your own disc image of the **US**
release (title EA-062): extract it from the `.iso` with an Xbox ISO tool such as `extract-xiso`. A clean dump and a
media-patched one both work.

```sh
git clone https://github.com/coru182/reKing.git
cd reKing
./setup.sh /path/to/default.xbe     # dependencies, then the game's code recompiled from your default.xbe
./build-ps5.sh                      # the app: build-ps5/PPSA99063-1.0.0.zip
```

`setup.sh` downloads the pinned dependencies into `deps/` and builds reUndr-ps5's toolchain with its own
`tools/rebuild-deps.py` (PayloadSDK, Mesa RADV for the PS5, libc, compiler-rt) and `tools/build-native-tool.py`. That
build takes an hour or more; if you already have a built reUndr-ps5 checkout, pass it: `REUNDR=/path/to/reUndr-ps5
./setup.sh ...` (and the same `REUNDR=` for `build-ps5.sh`).

The app only runs with the release it was recompiled from: it checks the disc image's `default.xbe` at start.

### Your own home screen art

The tile is a plain crown by default. To use your own pictures (a square image for the tile, a 16:9 one for the
background):

```sh
pip install numpy pillow
python3 tools/mkart.py my-icon.jpg my-background.png art/                # optionally --crop x0,y0,x1,y1
./build-ps5.sh package
```

`art/` is left out of git; `build-ps5.sh` puts its `icon0.png`, `pic0.dds` and `pic1.dds` in the app.

## Installing and playing

You need a PS5 that runs homebrew, with ShadowMountPlus (it registers apps in `/data/homebrew`) and an FTP server.

1. Unzip `PPSA99063-1.0.0.zip`. Over FTP, copy the `PPSA99063` folder to `/data/homebrew/`. A "reKing" tile appears
   on the home screen after about 30 seconds.
2. Over FTP, create `/data/rotk/` and copy your disc image into it (for example `/data/rotk/rotk.iso`), or put it on a
   USB drive in a folder named `rotk`. The game reads the image directly.
3. Start the tile.

| Xbox | DualSense |
| --- | --- |
| A B X Y | Cross Circle Square Triangle |
| Black / White | R1 / L1 |
| Triggers | R2 / L2 |
| Start / Back | Options / touchpad click |
| Stick clicks | L3 / R3 |

Press **L3 + R3** at any time for the settings menu (L1 / R1 change page, Circle or Options to go back). Settings are
in `/data/rotk/settings.toml`, saves in `/data/rotk/hdd/`. `package/README.txt`, which comes with the app, has the
details.

To find a problem, create an empty file `/data/rotk/logging-on`: the game then writes `/data/rotk/logs/`. Delete it
again afterwards.

Known limit: the controller picture on the help screen shown for a moment after character select is still the Xbox
controller.

## How it fits together

- [Athelas](https://github.com/Shagg88/Athelas) 1.8.2 by Shagg: The Return of the King's Xbox port (its hooks,
  settings, menus, fonts, button pictures) and the SDK it ships, built on
  [XboxXBE-Recomp](https://github.com/TGP482/XboxXBE-Recomp) by TGP482 (the recompiler and the Xbox runtime: kernel,
  Direct3D 8 on Vulkan, DirectSound).
- `patches/athelas-ps5.patch`: the changes to them for the PS5: guest memory on 16 KiB pages, threads, the Vulkan
  surface on VideoOut, a logging switch, and the PlayStation wording and button pictures.
  `patches/media-patched-xbe.patch` leaves out the disc check's hook for a media-patched `default.xbe` (setup.sh
  decides).
- `ps5/`: the PS5 host: the entry point and disc image, the app shell and settings, DualSense input (libScePad),
  sound (sceAudioOut), the in-game menu (Dear ImGui), the PS5 wording of the game's text, and POSIX calls the PS5
  doesn't allow from an app.
- [reUndr-ps5](https://github.com/bystritskiy/reUndr-ps5) by bystritskiy: the pinned PS5 toolchain (mihawk-99's
  PS5_PayloadSDK and PS5_Vulkan with Mesa RADV), its link recipe and packaging tool, and the parts of its PS5 host
  this one follows.
- [glslang](https://github.com/KhronosGroup/glslang) 15.4.0 compiles the game's shaders at run time (from HLSL);
  Mesa keeps the compiled ones in `/data/rotk/shadercache`.

## Licence

This repository's own code is GPL-3.0 (`LICENSE`). Its patches change Athelas and XboxXBE-Recomp, which are MIT; the
app you build also contains Dear ImGui, toml++, glslang, Mesa, and the PS5 toolchain, whose link recipe and platform
library are GPL-3.0-or-later. `package/licenses/` has every licence and `package/licenses/NOTICE.txt` the details.
The game, its assets and the code recompiled from it belong to their owners and are covered by none of this.
