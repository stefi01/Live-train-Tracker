# Live Trains

Official firmware for the **Live Trains** 7 inch desk display sold on Etsy.

This repo is the update feed the screen reads. Buyers do not need Arduino, Visual Studio, or a computer flash tool.

It is a desk gadget. It is not a timetable, not a railroad dispatcher tool, and not for navigation.

Live traffic is **Amtrak / VIA / Brightline** passenger trains. Freight and city commuter lines are not in this feed.

## If you bought one

The display must already be on Wi-Fi (2.4 GHz).

1. On the screen: **SETUP → ABOUT**
2. Tap **CHECK**
3. If a newer version is listed, tap **INSTALL**
4. Leave the USB power connected until it reboots

The screen downloads `ota/lt_7in.bin` from this repository and writes it by itself.

If CHECK says this is the latest, you are already current.

## Files the screen uses

| File | What it is |
| --- | --- |
| `ota/version.txt` | One line, the latest version number (example `1.15`) |
| `ota/lt_7in.bin` | The firmware image the chip flashes |

Those two files must stay on branch `main` under `ota/`.

## Publish a new version (shop)

1. Bump the version in the sketch (`LT_VER` / About) and in `ota/version.txt` so they match.
2. Arduino IDE: **Sketch → Export compiled Binary**.
3. Rename that `.bin` to `ota/lt_7in.bin` and replace the copy here.
4. Push `ota/version.txt` and `ota/lt_7in.bin` to `main`.

The screen looks up:

- `https://raw.githubusercontent.com/stefi01/live_trains/main/ota/version.txt`
- `https://raw.githubusercontent.com/stefi01/live_trains/main/ota/lt_7in.bin`

This repository must stay **public** or CHECK / INSTALL will fail on buyer units.

The file the chip wants is a `.bin`. An Intel `.hex` will not install.

Same board as Home Field Scope: Sunton / Jingcai **ESP32-8048S070C**, 7 in 800×480, capacitive touch.

## Support

Etsy message the shop if CHECK fails, INSTALL stops, or the glass stays black after an update. Have the version from **SETUP → ABOUT** ready.
