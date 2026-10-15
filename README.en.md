<a id="top"></a>

<h1 align="center">DsRunner</h1>

<p align="center">
  <b>A modern launcher for the Nintendo DS</b><br>
  Console tiles, big pictures, search, favorites, color themes and "return from game", for the M3 DS Real and many other flashcards.
</p>

<p align="center">
  <a href="https://github.com/HolziDape/DsRunner/releases/latest"><img alt="Download" src="https://img.shields.io/badge/Download-latest%20version-2ea44f?style=for-the-badge"></a>
  <img alt="Nintendo DS" src="https://img.shields.io/badge/Nintendo%20DS-DS%20%C2%B7%20DS%20Lite%20%C2%B7%20DSi%20%C2%B7%203DS-5b6ee1?style=for-the-badge">
  <a href="LICENSE.txt"><img alt="License Zlib" src="https://img.shields.io/badge/License-Zlib-lightgrey?style=for-the-badge"></a>
</p>

<p align="center">
  <a href="README.md">Deutsch</a> · <b>English</b>
  <br><br>
  <a href="#download">Download</a> ·
  <a href="#installation-m3-ds-real">Installation</a> ·
  <a href="#controls">Controls</a> ·
  <a href="#compatible-flashcards">Flashcards</a> ·
  <a href="#troubleshooting">Troubleshooting</a> ·
  <a href="CHANGELOG.md">Changelog</a>
</p>

<p align="center">
  <img src="docs/images/hero.png" alt="DsRunner: console level, a GBA folder with play statistics and search results" width="100%">
  <br><sub>Emulator screenshots with homebrew games. The UI is shown in German here; English is one switch away.</sub>
</p>

## Contents

[Features](#features) · [Download](#download) · [Installation](#installation-m3-ds-real) · [Emulators](#emulators-for-other-consoles) · [Controls](#controls) · [Return from game](#returning-to-dsrunner-from-a-game) · [Search](#search) · [Menu and language](#menu-and-language) · [Color themes and music](#color-themes-and-music) · [Pictures](#pictures-for-your-games) · [Settings](#settings) · [DsRunner Stream](#dsrunner-stream) · [Updating](#updating-to-a-new-version) · [Flashcards](#compatible-flashcards) · [Troubleshooting](#troubleshooting) · [Developers](#for-developers) · [Credits](#credits-and-license)

## Features

| | |
|---|---|
| **Starts automatically** | when the DS is switched on, without going through another menu (ready-made for the M3 DS Real, [by hand](docs/FLASHCARDS.md#autostart) for many other cards) |
| **Consoles and games** | first NDS, GBA, NES and others as tiles with a drawn console, then the games of that console |
| **Three views** | big tiles (3×2), small tiles (4×3) and Coverflow |
| **Other consoles** | GBA, NES, Game Boy / Color and SNES work right away, the emulators are in the package |
| **Search** | across all games with an on-screen keyboard, results grouped by console |
| **Favorites and history** | favorites with X, "last played" sorting, **continue** with START |
| **Play statistics and names** | "Played 12× · last yesterday"; "Golden Sun" instead of "Golden Sun (Europe) (En,Fr,De).gba" |
| **Return from game** | a key combo in the game restarts DsRunner without switching the DS off |
| **Five color themes and music** | Dark, Light, OLED, Retro, Blue or your own colors; optional background music |
| **German and English** | switchable in the menu, takes effect immediately |
| **DsRunner Art** | PC program that downloads pictures and covers for all games automatically |
| **DsRunner Stream** | play browser games streamed from a Raspberry Pi over WiFi (experimental) |

Plus cheats via `usrcheat.dat`, clock, battery indicator and touchscreen control.

<img src="docs/images/views.png" alt="Console level in the 3×2 grid, the 4×3 grid and Coverflow" width="100%">

## Download

**[Download the latest version](https://github.com/HolziDape/DsRunner/releases/latest)**

| File | For |
|---|---|
| **`DsRunner_M3DSReal.zip`** | **M3 DS Real:** complete SD card package with autostart, emulators and DsRunner Stream |
| `DsRunner.zip` | other flashcards, see [Compatible flashcards](#compatible-flashcards) |
| `PicoLoader-DsRunner_<platform>.zip` | other flashcards: Pico Loader with "return from game", one file per card type |
| `DsRunnerArt.exe` | Windows program that puts the pictures for your games on the card |
| `DsRunnerStream-Host.zip` | Raspberry Pi side of [DsRunner Stream](#dsrunner-stream) (from version 0.4.0) |

What's new is in the **[changelog](CHANGELOG.md)** (German). The installed version is shown in DsRunner's menu (gear at the top right).

## Installation (M3 DS Real)

1. **Back up your SD card**, at least all saves (`.sav`), the `SYSTEM` folder and `SRESET.DAT`.
2. Unzip `DsRunner_M3DSReal.zip` and copy its **contents** to the root of the SD card, replacing existing files.
3. Put your games into one folder per console under `/Games` (see below).
4. Insert the card and switch the DS on. DsRunner starts by itself.

**Back to the old menu** (e.g. YSMenu): copy the saved `SYSTEM/g6dsload.1`, `SYSTEM/g6dsload.eng` and `SRESET.DAT` back, then delete `boot.*`, `loader.eng` and the `_system_` folder.

### Game folders

```
SD card
├── Games
│   ├── NDS     ← DS games (.nds)
│   ├── GBA     ← Game Boy Advance games (.gba)
│   ├── NES, GB, SNES, …   ← more consoles, see emulators
│   └── Apps    ← homebrew programs (DsRunnerStream.nds is here too)
├── _dsrunner   ← emulators, pictures, favorites, statistics
└── _pico       ← settings, themes, Pico Loader
```

Every subfolder of `/Games` is a console. `NDS`, `GBA`, `GB`, `GBC`, `NES`, `SNES`, `SMS`, `GG`, `MD`, `PCE` and `Apps` get a matching console and color, other folders a plain tile. Without `/Games` DsRunner shows the root of the card; choose another folder with `libraryPath` in the [settings](#settings).

## Emulators for other consoles

DsRunner starts DS games directly; other consoles need an emulator in **`/_dsrunner/emulators/`**. Without it, the folder shows "No games".

| Console | Folder | Files | Emulator |
|---|---|---|---|
| Game Boy Advance | `GBA` | `.gba` | GBARunner2: **included** |
| NES | `NES` | `.nes`, `.fds` | nesDS: **included** |
| Game Boy / Color | `GB`, `GBC` | `.gb`, `.gbc` | GameYob: **included** |
| Super Nintendo | `SNES` | `.sfc`, `.smc` | SNEmulDS: **included** |
| Master System / Game Gear | `SMS`, `GG` | `.sms`, `.gg` | download [S8DS](https://github.com/FluBBaOfWard/S8DS/releases), `S8DS.nds` |
| Mega Drive | `MD` | `.gen`, `.md` | `jEnesisDS.nds` from the [TWiLight Menu++ Virtual Console add-on](https://github.com/DS-Homebrew/TWiLightMenu/releases) |
| PC Engine | `PCE` | `.pce` | download [NitroGrafx](https://github.com/FluBBaOfWard/NitroGrafx/releases), `NitroGrafx.nds` |

- The file names must match the table. DsRunner also finds the emulators of TWiLight Menu++ (`/_nds/TWiLightMenu/emulators/`).
- **GBA BIOS:** optional and not included. If you own it, put it at `/_gba/bios.bin`; more games run then.
- If a Mega Drive game ending in `.md` doesn't load, rename it to `.gen`. Licenses: [licenses/emulators](licenses/emulators/LIZENZEN.txt), details: [docs/CONFIG.md](docs/CONFIG.md#emulatoren).

## Controls

| Key | Action |
|---|---|
| D-pad | move the selection |
| A | open console / start game |
| B | back to the consoles |
| L / R | next page; in search results one console / all; in the menu switch page |
| X | add / remove favorite |
| Y | cheats for the selected game |
| START | on the console level: continue the last played game |
| SELECT | open the search |
| Gear (top right) | menu; the brush in it opens the theme list |

The touchscreen works everywhere. The top screen shows each game's name without file extension and tags like "(Europe)", and below it the **play statistics** (starts and last start, counted in `/_dsrunner/stats.txt`, date from the DS clock).

### Returning to DsRunner from a game

In a running DS game **hold L + R + Down + B for two seconds**. The screen flashes bright, then DsRunner starts again.

- **Save first.** Unsaved progress is lost, but a running save is not interrupted.
- **Not every game supports it.** Then it keeps running normally, just without this feature.
- **Other keys**, e.g. with worn L/R buttons: choose SELECT+START+↓ or SELECT+↓+B in the menu, any other combo with `inGameResetKeys`.
- Only works with the Pico Loader from our packages, not with the official one. Details: [loader/README.md](loader/README.md).

## Search

SELECT or the **Search** tile opens an on-screen keyboard (touchscreen or D-pad + A; B deletes, X space, START searches). While you type it shows how many games per console match. Results appear like a folder, grouped by console. It searches the file names, ignoring case, accents and punctuation, in every console that has an emulator.

<img src="docs/images/search.png" alt="Search keyboard and search results" width="66%">

## Menu and language

The **gear** opens the menu with two pages (L / R switches). Changes are saved when it closes.

- **Display:** view (3×2, 4×3, Coverflow), sorting (A–Z, Z–A, last played), clock, installed version
- **Game & language:** return from game on/off, its keys, **Deutsch / English** (switches immediately; in English the keyboard is QWERTY)

<img src="docs/images/menu.png" alt="Menu page 1, menu page 2, English UI" width="100%">

## Color themes and music

**Themes:** gear → brush → choose DsRunner Dark (default), Hell (light), OLED, Retro or Blau (blue). For your own theme copy a folder in `/_pico/themes` and change the colors under `"colors"` in its `theme.json` ([all keys](docs/Themes.md#dsrunner-type)).

**Music:** off by default. Put a `.bcstm` file into `/_pico/themes/dsrunner/bgm/`, or rename the example folder `musik-beispiel` to `bgm` to try it. `tools/bgm/wav2bcstm.py` converts MP3/WAV. Turn it off with `"music": false`, make it quieter with `"musicVolume"` (0–100). Guide: [docs/CONFIG.md](docs/CONFIG.md#hintergrundmusik).

<img src="docs/images/themes.png" alt="The five color themes: Dark, Light, OLED, Retro, Blue" width="100%">

## Pictures for your games

Insert the SD card into your PC, start **`DsRunnerArt.exe`**, choose the drive and click **"Bilder aktualisieren"** (update pictures). The program recognizes your games and downloads title screens and covers from [libretro-thumbnails](https://github.com/libretro-thumbnails); with Python it also runs on Linux and macOS ([docs/ART-TOOL.md](docs/ART-TOOL.md)).

**Your own pictures:** a BMP with 256 × 192 pixels and 256 colors in `/_dsrunner/art`, in the same path as the game, e.g. `/_dsrunner/art/Games/NDS/My Game.bmp`. Your own pictures take priority and are never overwritten.

## Settings

Most settings are in the [menu](#menu-and-language), all of them in `/_pico/settings.json` (editable with any text editor, mind the commas):

| Setting | Meaning |
|---|---|
| `"libraryPath": "/Games"` | folder with the console folders |
| `"uiLanguage": "en"` | English UI (`"de"` = German, default) |
| `"cleanNames": false` | game names with region and tags, only without the extension |
| `"inGameReset": false` | turn off return from game |
| `"inGameResetKeys": "SELECT+START+DOWN"` | keys for it: at least 3 of A, B, SELECT, START, UP, DOWN, LEFT, RIGHT, L, R |
| `"music": false` / `"musicVolume": 50` | background music off / quieter |

All settings (German): **[docs/CONFIG.md](docs/CONFIG.md)**.

## DsRunner Stream

> **Experimental.** A Raspberry Pi 5 (or PC) opens a browser game and sends the picture over WiFi to the DS; the DS keys and touchscreen control the game.

On the DS start `Games/Apps/DsRunnerStream.nds`. On the Pi unzip `DsRunnerStream-Host.zip` and run `tools/install-pi.sh`: the installer sets up Chromium, the Python packages, an open WiFi network just for the DS and the start script `~/dsstream.sh`. Tested on DS Lite + Pi 5: about **18 to 25 frames per second** in 256 colors, no sound on the DS. Guide (German): **[stream/README.md](stream/README.md)**.

## Updating to a new version

1. Copy `_nds/TWiLightMenu/main.srldr` and the `_pico/themes` folder from the new `DsRunner_M3DSReal.zip` to the card.
2. If the [changelog](CHANGELOG.md) says **"Pico Loader geändert"** (Pico Loader changed), also copy the five `.bin` files from `_pico`.
3. If you don't have them yet: copy `_dsrunner/emulators`, `snemul.cfg` and `Games/Apps/DsRunnerStream.nds` once.

Games, saves, settings, pictures, favorites and statistics are kept.

## Compatible flashcards

DsRunner itself is a normal DS homebrew program. It starts games through **Pico Loader**, which exists for these cards ("platforms"). **Only the M3 DS Real has been tested so far.**

| Card | Platform |
|---|---|
| **M3 DS Real** (tested), M3i Zero (not GMP-Z003), iTouchDS | `M3DS` |
| M3i Zero GMP-Z003 | `G003` |
| Acekard 2, 2.1, 2i · Acekard RPG | `AK2` · `AKRPG` |
| Ace3DS+, Gateway 3DS (blue) | `ACE3DS` |
| DSTT, SuperCard DSONE SDHC | `DSTT` |
| R4iDSN · original R4DS (return from game only partly) | `R4iDSN` · `R4` |
| Stargate 3DS · DSpico · EZ-Flash Parallel · DATEL | `STARGATE` · `DSPICO` · `EZP` · `DATEL` |
| Slot 2: SuperCard, M3 CF, Max Media Dock, GBA Movie Player | `SUPERCARD` · `SUPERCARDCF` · `M3CF` · `MMCF` · `MPCF` |

**Only choose a platform that lists your card by name:** according to Pico Loader the wrong one can damage the card. Full table with all clones, autostart and DSi/3DS (German): **[docs/FLASHCARDS.md](docs/FLASHCARDS.md)**.

### Installing on other cards

1. Back up the card, unzip `DsRunner.zip` to the root of the SD card.
2. Unzip `PicoLoader-DsRunner_<platform>.zip` for **your** card to the root as well.
3. Start `DsRunner.nds` from the card's menu. Autostart: [docs/FLASHCARDS.md](docs/FLASHCARDS.md#autostart) (autoboot files from TWiLight Menu++; on the DSpico rename it to `/_picoboot.nds`).

## Troubleshooting

| Problem | Fix |
|---|---|
| A console folder shows "No games" | The emulator is missing or has another name, see [emulators](#emulators-for-other-consoles). |
| "Failed to load rom header" when starting a game | The emulator file is broken; download it again. |
| A game ignores "return from game" | `/_pico/igr-log.txt` says why after the game started ([loader/README.md](loader/README.md)). |
| Wrong date in the play statistics | Set the clock in the DS system settings. |
| DsRunner Stream doesn't connect | [Troubleshooting in the stream README](stream/README.md) |

Bugs and wishes: [issues](https://github.com/HolziDape/DsRunner/issues). Games that run worse than with other menus: [compatibility list](docs/compat/pico-vs-ysmenu.md).

## For developers

```sh
git clone --recursive https://github.com/HolziDape/DsRunner.git
cd DsRunner
./build.sh            # → DsRunner.nds (Docker with BlocksDS)
./build.sh package    # → SD package for the M3 DS Real in sd-package/
```

More (German): [development](docs/DEVELOPMENT.md) · [settings](docs/CONFIG.md) · [themes](docs/Themes.md) · [Pico Loader](loader/README.md) · [releases](docs/RELEASE.md) · [DsRunner Art](docs/ART-TOOL.md) · [stream protocol](stream/PROTOCOL.md) · [plans](docs/PLAN.md)

## Credits and license

DsRunner is a fork of **[Pico Launcher](https://github.com/LNH-team/pico-launcher)** and uses **[Pico Loader](https://github.com/LNH-team/pico-loader)**, both by the [LNH team](https://github.com/LNH-team) ([@Gericom](https://github.com/Gericom), [@XLuma](https://github.com/XLuma), [@Dartz150](https://github.com/Dartz150), [@lifehackerhansol](https://github.com/lifehackerhansol)). "Return from game" and the DLDI cache in Pico Loader are by [@danyocom](https://github.com/danyocom/pico-loader), autostart files and some emulators come from [TWiLight Menu++](https://github.com/DS-Homebrew/TWiLightMenu), icons by [icons8](https://icons8.com/).

License: Zlib (`LICENSE.txt`), more in the `licenses` folder.

<p align="right"><sub><a href="#top">↑ back to top</a></sub></p>
