# reKing

**The Lord of the Rings: The Return of the King (US Xbox), statically recompiled into a native PS5 app.**

The original Xbox game's code is translated to C++ and compiled for the PS5, like
[reUndr](https://github.com/bystritskiy/reUndr-ps5) does for Need for Speed: Underground. It's not an emulator: it
draws through Vulkan (Mesa's RADV) at up to 4K and 60 fps, and plays with DualSense controllers.

> [!IMPORTANT]
> **Bring your own game.** The app is the game's program recompiled for the PS5; it contains none of the game's
> files. Its graphics, sound, videos and levels come from your own disc image of the US Xbox release, which you copy
> to the console. Please don't ask for or share disc images here.

An unofficial fan project, not affiliated with or endorsed by Electronic Arts, New Line Cinema, Warner Bros., the
Tolkien Estate, Microsoft or Sony Interactive Entertainment.

## Install

You need a PS5 that runs homebrew, with [ShadowMountPlus](https://github.com/drakmor/ShadowMountPlus) (it registers
apps in `/data/homebrew`) and an FTP server.

1. **The app.** Download `PPSA99063-<version>.zip` from [Releases](../../releases/latest), unpack it, and copy the
   `PPSA99063` folder to `/data/homebrew/` on the console. The **reKing** tile appears on the home screen.
2. **Your disc.** Copy your disc image (`.iso`) of The Return of the King, **US Xbox release**, to `/data/rotk/` on
   the console (create the folder), or to a `rotk` folder on a USB drive. A full dump and a trimmed one, clean or
   media-patched, all work. The app reads the image as it is: nothing is unpacked.
3. **Play.** Start the tile.

If the disc image is missing or is another release, the app says so on screen.

**To update,** copy the new `PPSA99063` folder over the old one. Your saves and settings in `/data/rotk/` are kept.

## Controls

| Xbox | DualSense |
| --- | --- |
| A B X Y | Cross Circle Square Triangle |
| Black / White | R1 / L1 |
| Triggers | R2 / L2 |
| Start / Back | Options / touchpad click |
| Stick clicks | L3 / R3 |

Press **L3 + R3** at any time for the settings menu (L1 / R1 change page, Circle or Options to go back). For two
players, sign in a second user on the PS5 (a guest is enough) and turn on their controller. Settings are in
`/data/rotk/settings.toml`, saves in `/data/rotk/hdd/`; the app's `README.txt` has the details.

## Features

- Native 4K output, rendered at 1080p, 1620p or 4K; 30 or 60 fps
- PlayStation buttons throughout: the button pictures in hints and menus (Cross, Circle, Square, Triangle), "Press
  OPTIONS", L2 / R2, and the save messages say PS5
- An in-game settings menu: resolution, frame rate, V-Sync, fps counter, texture filtering, sharper menus, field of
  view, draw distance, HUD placement, skippable videos, vibration, deadzones, damage, cheats
- Every video can be skipped; the EA / New Line logos are skipped
- Sharper menus at 4K: the menus' full-screen pictures redrawn four times larger
- Two players, saves on the console
- No logging while you play

Known limit: the controller picture on the help screen shown for a moment after character select is still the Xbox
controller.

## Reporting a problem

Open an [issue](../../issues): say what you were doing, what you expected and what happened. To get a log, create an
empty file `/data/rotk/logging-on` on the console and start the game; attach the newest `/data/rotk/logs/rotkx_*.log`
and `stderr.log`, then delete `logging-on` again. **Never attach game files or disc images.**

## For developers

### Building

You need a Linux machine (Ubuntu 24.04; WSL 2 should do) with:

```sh
sudo apt install git cmake ninja-build clang-18 lld-18 llvm-18 llvm-18-dev zlib1g-dev python3 python3-pip patch zip
```

and Docker, for reUndr-ps5's PS5 toolchain. You also need `default.xbe` from your own disc image of the US release
(title EA-062): extract it from the `.iso` with an Xbox ISO tool such as `extract-xiso`.

```sh
git clone https://github.com/coru182/reKing.git
cd reKing
./setup.sh /path/to/default.xbe     # dependencies, then the game's code recompiled from your default.xbe
./build-ps5.sh                      # the app: build-ps5/PPSA99063-<version>.zip
```

`setup.sh` downloads the pinned dependencies into `deps/` and builds reUndr-ps5's PS5 toolchain with its own scripts
(PayloadSDK, Mesa RADV for the PS5, libc, compiler-rt, the packaging tool), about 20 minutes the first time. With a
built reUndr-ps5 checkout already at hand, pass it: `REUNDR=/path/to/reUndr-ps5 ./setup.sh ...` (and the same
`REUNDR=` for `build-ps5.sh`).

At start the app checks the disc's `default.xbe` against the one it was built from: the same bytes, or the same
release with a few bytes of code changed (a media-patched or clean dump, `ps5/xbe_match.h`).

### Your own home screen art

The tile is a plain crown by default. To use your own pictures (a square image for the tile, a 16:9 one for the
background):

```sh
pip install numpy pillow
python3 tools/mkart.py my-icon.jpg my-background.png art/                # optionally --crop x0,y0,x1,y1
./build-ps5.sh package
```

`art/` is left out of git; `build-ps5.sh` puts its `icon0.png`, `pic0.dds` and `pic1.dds` in the app.

### Making a release on GitHub

The **Build reKing** workflow (Actions tab) builds the app on GitHub's machines and puts it in a release.

1. Put your `default.xbe` somewhere with a private direct-download link (for example a Dropbox link ending in
   `?dl=1`). Never commit it to the repository.
2. In the repository's **Settings > Secrets and variables > Actions**, add the secret `XBE_URL` with that link.
   Optionally add `XBE_SHA256` with the file's checksum (`sha256sum default.xbe`) so a wrong download is caught.
   Secrets never appear in the logs, and workflows from forks don't get them.
3. Set `VERSION` in `build-ps5.sh` (and `contentVersion` in `package/sce_sys/param.json`) for the new release.
4. **Actions > Build reKing > Run workflow.** Tick "Publish the release" to publish it straight away as the latest
   release; otherwise it is a draft that only you see until you publish it from **Releases**. A run takes roughly
   half an hour, plus about 20 minutes when the PS5 toolchain isn't kept yet (the first run, or after a week
   without runs, when GitHub drops it). "Only the PS5 toolchain" prepares that part without your `default.xbe`.

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

## Credits

Made by [coru182](https://github.com/coru182), with Claude (Anthropic). Built on the work of:

- **The game:** EA Redwood Shores, who made The Return of the King, and Electronic Arts, who published it; Hypnos
  Entertainment, who made its PC version (whose larger fonts and menu pictures the optional sharper text reads). And
  J.R.R. Tolkien, and New Line Cinema's films, for the world it's set in.
- **[Athelas](https://github.com/Shagg88/Athelas) by Shagg:** The Return of the King's Xbox port (its hooks,
  settings, menus, sharper text, PlayStation button pictures and ROTK Fix compatibility) and the SDK this builds on.
- **[XboxXBE-Recomp](https://github.com/TGP482/XboxXBE-Recomp) by TGP482:** the static recompiler and the Xbox
  runtime (kernel, Direct3D 8 on Vulkan, DirectSound, XAPI).
- **[XenonRecomp](https://github.com/hedge-dev/XenonRecomp) by hedge-dev:** the overridable function layout and
  instruction hooks the recompiler follows; **[Zydis](https://github.com/zyantific/zydis) by zyantific:** the
  disassembler it reads the game's code with.
- **[reUndr-ps5](https://github.com/bystritskiy/reUndr-ps5) by bystritskiy:** the PS5 port this one is modelled on:
  the pinned toolchain, the link recipe, the packaging tool and the parts of its PS5 host this one follows.
- **[PS5_Vulkan](https://github.com/mihawk-99/PS5_Vulkan), [PS5_Mesa](https://github.com/mihawk-99/PS5_Mesa) and
  [PS5_PayloadSDK](https://github.com/mihawk-99/PS5_PayloadSDK) by mihawk-99:** Vulkan on the PS5: Mesa's RADV with a
  PS5 winsys and VideoOut, and the payload SDK's platform layer.
- **[ps5-native-app-boilerplate](https://github.com/blackbearreloaded/ps5-native-app-boilerplate) by
  BlackBearReloaded:** the ELF converter, the fake-signed SELF writer and the clean-room `libc.prx` (through
  PS5_Vulkan).
- **[ps5-payload-dev/sdk](https://github.com/ps5-payload-dev/sdk) by John Törnblom:** the public PS5 headers, sysroot
  and toolchain.
- **[Mesa](https://gitlab.freedesktop.org/mesa/mesa)** (RADV, ACO, NIR, with AMD's AddrLib) and
  **[LLVM / Clang](https://github.com/llvm/llvm-project)**.
- **[glslang](https://github.com/KhronosGroup/glslang)** and the Vulkan headers (Khronos Group),
  **[Dear ImGui](https://github.com/ocornut/imgui)** (Omar Cornut), **[toml++](https://github.com/marzer/tomlplusplus)**
  (Mark Gillard), **[stb](https://github.com/nothings/stb)** (Sean Barrett), **[zlib](https://zlib.net/)** (Jean-loup
  Gailly and Mark Adler) and **Liberation Sans** (SIL Open Font License).
- **On the console:** [ShadowMountPlus](https://github.com/drakmor/ShadowMountPlus) by drakmor,
  [ftpsrv](https://github.com/ps5-payload-dev/ftpsrv) by ps5-payload-dev, and the PS5 homebrew scene whose jailbreak
  and loaders run it.

## Licence

This repository's own code is GPL-3.0 (`LICENSE`). Its patches change Athelas and XboxXBE-Recomp, which are MIT; the
app you build also contains Dear ImGui, toml++, glslang, Mesa, and the PS5 toolchain, whose link recipe and platform
library are GPL-3.0-or-later. `package/licenses/` has every licence and `package/licenses/NOTICE.txt` the details.
The game, its assets and the code recompiled from it belong to their owners and are covered by none of this.

## Legal notice

This is an unofficial, non-commercial fan project, independent of the upstream authors and rights holders. The game and
"The Lord of the Rings: The Return of the King" belong to their rights holders (Electronic Arts, New Line Cinema and
the Tolkien licensors). Xbox is a trademark of Microsoft Corporation. PlayStation, PS5 and DualSense are trademarks of Sony Interactive Entertainment
Inc. All other trademarks belong to their respective owners.
