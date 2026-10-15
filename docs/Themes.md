# Themes
Using themes, the look and feel of Pico Launcher can be customized. Themes are placed in subfolders of the `/_pico/themes` folder. For example `/_pico/themes/my_theme`.

## JSON file
Each theme has a `theme.json` file with information about the theme.

- **type** - Type of theme. Currently `material`, `custom` and `DsRunner` are supported. See below for information about each type.
- **name** - The name of the theme.
- **description** - Description of the theme.
- **author** - Author of the theme.
- **primaryColor** - Material Design 3 primary color to use. `r`, `g` and `b` are provided in range 0-255.
- **darkTheme** - When `true`, a dark Material Design 3 palette will be used.

### Example
```json
{
    "type": "material",
    "name": "Theme name",
    "description": "Theme description here.",
    "author": "Author Name",
    "primaryColor": {
        "r": 149,
        "g": 143,
        "b": 237
    },
    "darkTheme": true
}
```

## Material type
![Horizontal display mode with custom theme](images/Horizontal.png)

The `material` type theme is a pure Material Design 3 theme. It will be fully themed based on the `primaryColor` and `darkTheme` settings from the `theme.json`. In coverflow mode, this theme type uses a Material Design 3 style carousel.

## Custom type
![Horizontal display mode with custom theme](images/HorizontalCustom.png)

The `custom` type theme is much more customizable, compared to the `material` type theme.
Note that the `primaryColor` and `darkTheme` settings from the `theme.json` are still used to color some parts of the UI.

The following additional files are needed:
| Files                                                        | Size                 | Format                   | Description                                                    |
|--------------------------------------------------------------|----------------------|--------------------------|----------------------------------------------------------------|
| bannerListCell.bin<br>bannerListCellPltt.bin                 | 256x49 (209x49 used) | A3I5<br>32 color palette | Unselected item background for banner list mode.               |
| bannerListCellSelected.bin<br>bannerListCellSelectedPltt.bin | 256x49 (209x49 used) | A3I5<br>32 color palette | Selected item background for banner list mode.                 |
| bottombg.bin                                                 | 256x192              | 15 bpp bitmap            | Bottom screen background.                                      |
| gridcell.bin<br>gridcellPltt.bin                             | 64x48 (48x48 used)   | A3I5<br>32 color palette | Unselected item background for grid modes.                     |
| gridcellSelected.bin<br>gridcellPlttSelected.bin             | 64x48 (48x48 used)   | A3I5<br>32 color palette | Selected item background for grid modes.                       |
| scrim.bin<br>scrimPltt.bin                                   | 8x42                 | A5I3<br>8 color palette  | Background for the toolbar. Intended to be a translucent fade. |
| topbg.bin                                                    | 256x192              | 15 bpp bitmap            | Top screen background.                                         |

These files can be created, for example, using [NitroPaint](https://github.com/Garhoogin/NitroPaint).

The top screen background should include a box in which the banner text and icon of the selected item will be shown.

### Additional JSON properties
Custom themes support additional properties in the `theme.json` file to allow for more customization.

- **topIcon** - Properties of the icon displayed on the top screen.
- **topBannerTextLine0** - Properties of the first banner text line displayed on the top screen.
- **topBannerTextLine1** - Properties of the second banner text line displayed on the top screen.
- **topBannerTextLine2** - Properties of the third banner text line displayed on the top screen.
- **topFileNameText** - Properties of the file name text displayed on the top screen.
- **topCover** - Properties of the cover image displayed on the top screen.
- **gridIcon** - Properties of the icons displayed on the bottom screen in grid display modes.
- **bannerListIcon** - Properties of the icons displayed on the bottom screen in banner list display mode.
- **bannerListTextLine0** - Properties of the first banner text line displayed on the bottom screen in banner list display mode.
- **bannerListTextLine1** - Properties of the second banner text line displayed on the bottom screen in banner list display mode.
- **bannerListTextLine2** - Properties of the third banner text line displayed on the bottom screen in banner list display mode.

Blend colors are used to fake translucency. They should be set to an approximation of the background color.

```json
{
    "type": "custom",
    "name": "Raspberry",
    "description": "Theme based on raspberries.",
    "author": "Gericom",
    "primaryColor": { "r": 138, "g": 217, "b": 255 },
    "darkTheme": false,
    "topIcon": {
        "position": { "x": 24, "y": 132 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topBannerTextLine0": {
        "position": { "x": 70, "y": 126 },
        "width": 168,
        "textColor": { "r": 30, "g": 30, "b": 30 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topBannerTextLine1": {
        "position": { "x": 70, "y": 141 },
        "width": 168,
        "textColor": { "r": 30, "g": 30, "b": 30 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topBannerTextLine2": {
        "position": { "x": 70, "y": 155 },
        "width": 168,
        "textColor": { "r": 30, "g": 30, "b": 30 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topFileNameText": {
        "position": { "x": 18, "y": 170 },
        "width": 220,
        "textColor": { "r": 30, "g": 30, "b": 30 },
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "topCover": {
        "position": { "x": 75, "y": 18 }
    },
    "gridIcon": {
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "bannerListIcon": {
        "blendColor": { "r": 200, "g": 200, "b": 200 }
    },
    "bannerListTextLine0": {
        "textColor": { "r": 30, "g": 30, "b": 30 }
    },
    "bannerListTextLine1": {
        "textColor": { "r": 30, "g": 30, "b": 30 }
    },
    "bannerListTextLine2": {
        "textColor": { "r": 30, "g": 30, "b": 30 }
    }
}
```

## DsRunner type
The `DsRunner` type is the look of DsRunner: big console tiles, the built-in console images and the gradient on the top screen. It needs no image files: the console images come from the theme's own `consoles` folder or, when it has none, from `/_pico/themes/dsrunner/consoles`. The colors are set in `theme.json` under `"colors"`; without `"colors"` the theme looks exactly like DsRunner Dark. DsRunner ships `dsrunner` (Dark), `dsrunner-hell` (light), `dsrunner-oled`, `dsrunner-retro` and `dsrunner-blau` as examples.

`primaryColor` and `darkTheme` still color Pico's own parts (display settings sheet, cheats, theme list): use the accent color as `primaryColor` and `"darkTheme": false` for light themes.

Colors are `"#RRGGBB"`, `[r, g, b]` or `{ "r": .., "g": .., "b": .. }`. All keys are optional. When a theme has `"colors"`, missing keys are mixed from the others (column "Default with colors").

| Key | Used for | DsRunner Dark | Default with colors |
|---|---|---|---|
| `background` | Info band of the top screen, top of the bottom screen gradient, app bar | `#0F1117` | `#0F1117` |
| `backgroundBottom` | Bottom of the bottom screen gradient | `#181C26` | halfway `background` → `tile` |
| `tile` | Game tiles, search keys | `#232838` | `#232838` |
| `tileDim` | Bottom of unselected game tiles | `#1C202D` | `tile` 3/8 towards `background` |
| `tileTop` | Top of unselected game tiles | `#212635` | `tile` 1/8 towards `background` |
| `tileSelected` / `tileSelectedBottom` | Selected game tile (top / bottom) | `#3A425A` / `#282E40` | `tile` 1/8 resp. 1/32 towards `text` |
| `tileShade` | Unselected console tiles fade into it; gray level 0 of the tile and icon images | `background` | `background` |
| `artShade` | Gray level 0 of the top screen and cover console images | `background` | `background` |
| `accent` | Selection ring and glow, search key, cursor | `#4CC2FF` | `#4CC2FF` |
| `accentDim` | Bottom of the search key | `#3096D6` | `accent` 1/5 towards `background` |
| `onAccent` | Text on the search key, Pico's `onPrimary` | `background` | `background` |
| `text` | Titles, labels | `#EBEEF5` | `#EBEEF5` |
| `textDim` | Subtitles, hints | `#969EB2` | `text` 3/8 towards `background` |
| `textStrong` | Search text, selected key | `#FFFFFF` | `text` |
| `pill` | Status bar, buttons, special keys | `#2C3244` | `tile` 1/20 towards `text` |
| `favorite` | Star of favorites | `#FFC440` | `#FFC440` |
| `batteryGood` / `batteryLow` | Battery symbol | `#78E68C` / `#FF5050` | same |

Tips:
- Labels on console tiles are always white, the tiles keep the console colors. For a **light theme** set `tileShade` to a dark color (`dsrunner-hell` uses `#2B3140`), so unselected console tiles stay dark enough for the white labels and the screens in the console drawings stay dark. Keep `artShade` at the light background, the top screen images fade into it.
- The DS Lite screen loses contrast at the dark end: keep `textDim` clearly brighter than `background` in dark themes.
- Change `batteryGood` for light themes, the default green is meant for a dark status bar.

```json
{
    "type": "DsRunner",
    "name": "DsRunner Blau",
    "description": "Tiefblau mit Hellblau",
    "author": "HolziDape",
    "primaryColor": { "r": 142, "g": 211, "b": 255 },
    "darkTheme": true,
    "colors": {
        "background": "#0A1F44",
        "backgroundBottom": "#112C5A",
        "tile": "#1B3B6E",
        "pill": "#244985",
        "accent": "#8ED3FF",
        "text": "#EFF5FF",
        "textDim": "#9EB4D6"
    }
}
```

## Background music
All themes support background music by placing DSP-ADPCM encoded `.bcstm` files in a `bgm` folder inside the theme folder. Looping is supported. When multiple `.bcstm` files are provided, the background music will be selected at random each time Pico Launcher is started.

DsRunner additions: themes of type `DsRunner` without a `bgm` folder play the music of `/_pico/themes/dsrunner/bgm`; `"music": false` and `"musicVolume"` (0-100) in `settings.json` switch it off or make it quieter. `tools/bgm/wav2bcstm.py` converts WAV (or MP3 etc. with ffmpeg) to a looping DSP-ADPCM `.bcstm`, see [CONFIG.md](CONFIG.md#hintergrundmusik).

## Theme selector icon
A theme can have an `icon.bmp` file that is shown in the theme list when selecting a theme. It must be **32×32 pixels, 4 bpp (16 colors), uncompressed `.bmp`** file, with the first palette color treated as transparent.

## Theme selector preview image
A theme can have a `preview.bin` file that is shown on the top screen in the theme selection screen. It must be a 256x192 pixels 15 bpp bitmap (same format as `topbg.bin` and `bottombg.bin`). When it is not provided, `topbg.bin` is displayed instead. If that does not exist either, nothing is shown on the top screen.

For DsRunner themes, `tools/themes/make_theme_assets.py <theme folder> --screenshot shot.png` writes `icon.bmp` (drawn from the theme colors) and `preview.bin` (both screens of the screenshot, scaled down; the screenshot shows both screens stacked, 256x384 or a multiple of it, e.g. from `tools/desmume/screenshot.sh`).
