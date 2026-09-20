# ESP32 Web-Control

A small ESP32 project that turns your board into a web-controlled gadget: it opens its **own WiFi access point**, hosts a phone-friendly web page, and lets you control the onboard LED (switch, blink, or "breathe") as well as **scan every WiFi network** in range.

Built with [PlatformIO](https://platformio.org/) on the Arduino framework.

## What it does

- Opens a WiFi access point named `ESP32-Control` (password `12345678`) — no router, no internet needed
- Serves a dark-themed web page at `http://192.168.4.1`
- **LED control** via PWM on GPIO2:
  - `ON` / `OFF`
  - `BLINK` at a set speed
  - `BREATHE` — smooth sine-wave fade
  - Brightness slider (0–255) that works across all modes
  - Speed slider (100–3000 ms) for blink rate / breathe cycle
- **WiFi scanner**: lists every network in range with name, signal strength (dBm), channel and security type, strongest first
- Settings apply instantly and the page re-syncs from the board every 2 seconds

## What you need

| Hardware | Notes |
|---|---|
| Any classic ESP32 board (DevKitC / DevKit V1 style) | Uses the **onboard** LED, so no wiring at all |
| Micro-USB cable | For flashing and power |

> The onboard LED is on **GPIO2** for the classic ESP32 DevKitC. If your board uses a different pin (ESP32-S3, C3, etc.), change `LED_PIN` in `src/main.cpp`.

Software: [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html) or the PlatformIO IDE extension for VS Code.

## Getting started

```bash
git clone https://github.com/yourname/esp-webcontrol.git   # or use your own repo
cd esp-webcontrol
pio run -t upload        # build & flash to the ESP32 on /dev/ttyUSB0
```

If your board is on a different port:

```bash
pio run -t upload --upload-port /dev/ttyUSB1
```

To watch the serial output (115200 baud):

```bash
pio device monitor
```

## Using it

1. Power the ESP32 — after boot it broadcasts **`ESP32-Control`**
2. On your phone: **Settings → WiFi → join `ESP32-Control`** (password `12345678`)
3. Open a browser and go to **http://192.168.4.1**
4. Try the buttons and sliders, then hit **Scan WiFi networks** to see everything broadcasting around you

## API

The same endpoints the web page uses, useful if you want to drive it from scripts or home automation.

| Method & path | Description | Example |
|---|---|---|
| `GET /` | The web UI | — |
| `GET /state` | Current mode, brightness and speed | `{"mode":"blink","dim":80,"speed":250}` |
| `GET /led?mode=<m>` | `on`, `off`, `blink` or `breathe` | `/led?mode=breathe` |
| `GET /set?dim=<0-255>` | Set brightness | `/set?dim=128` |
| `GET /set?speed=<100-3000>` | Set blink/breathe speed in ms | `/set?speed=1000` |
| `GET /scan` | WiFi scan. Returns `{"status":"running"}` while scanning, then a JSON array | see below |
| `GET /style.css` | The page's stylesheet (static asset) | — |
| `GET /app.js` | The page's front-end logic (static asset) | — |

`/scan` result example:

```json
[
  {"ssid":"MyRouter","bssid":"aa:bb:cc:dd:ee:ff","rssi":-42,"channel":6,"security":"WPA2"},
  {"ssid":"","bssid":"11:22:33:44:55:66","rssi":-67,"channel":1,"security":"WPA3"}
]
```

## Project structure

```
esp-webcontrol/
├── platformio.ini      # board & framework setup
└── src/
    ├── main.cpp        # AP, web server, LED logic, scanner
    └── webpage.h       # the web UI, split into three assets so the
                        # markup stays readable: PAGE_HTML + STYLE_CSS
                        # + APP_JS (all served from flash, no RAM cost)
```

## Things worth knowing

- **Brownout on weak USB supplies**: turning on the WiFi radio pulls a lot of current, and on a flaky cable/port the ESP32 browns out and reboots in a loop. The firmware disables the brownout detector and caps TX power to keep it stable. If you use a proper power source, you can raise `WiFi.setTxPower(...)` for more range.
- **Short AP range**: TX power is intentionally capped at 11 dBm, so the access point reaches a few meters — plenty for testing from your phone.
- **Scanning briefly pauses the AP**: during a scan the board isn't sending beacons for a moment, so laptops with aggressive roaming may jump to another network. Phones usually don't notice.
- **`pio device monitor` holds the serial port**: close it (`Ctrl+C`) before running `pio run -t upload`, or the flash will fail with "port busy".

## License

MIT — do whatever you like with it.