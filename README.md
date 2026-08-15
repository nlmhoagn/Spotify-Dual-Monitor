# ESP32-S3 Spotify Dual-Monitor & Karaoke Lyrics Player

A feature-rich, dual-display Spotify music player powered by ESP32-S3 and FreeRTOS. It streams real-time Spotify playback status, renders live synced Karaoke lyrics on a main ILI9341 display, and displays a rotating vinyl album cover on a secondary ST7735 display.

---

## Hardware & Components Required

| Component | Model / Specification | Description |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32-S3 Development Board | Dual-Core Xtensa LX7, 2.4GHz Wi-Fi, 8MB Flash (PSRAM recommended) |
| **Main Display (Display 1)** | ILI9341 2.8" SPI TFT LCD (320x240) | Renders Spotify UI, track info, progress bar, audio EQ & 3-line Karaoke lyrics |
| **Secondary Display (Display 2)** | ST7735 1.44" / 1.8" SPI TFT LCD (160x128) | Renders rotating 76px vinyl record with decoded album cover art |
| **Power Supply** | USB Type-C Cable (5V / 1A+) | Powers the ESP32-S3 board and both TFT LCD backlight displays |
| **Wiring** | DuPont Jumper Wires / Breadboard | For SPI bus and control pin interconnections |

---

## Features

- **Dual Display Architecture**: Driven over a shared SPI bus using chip-select multiplexing.
  - **ILI9341 (320x240)**: Main display showing track name, artist, progress bar, audio EQ visualizer, and 3-line synced karaoke lyrics.
  - **ST7735 (160x128)**: Secondary display featuring a rotating 76px vinyl record with decoded album cover art and song title marquee.
- **Dual-Core FreeRTOS Multitasking**:
  - **Core 0**: Asynchronous network tasks (Spotify OAuth2 token refresh, HTTPS album cover downloads, LRCLIB API queries).
  - **Core 1**: Real-time smooth UI rendering, time interpolation, and vinyl rotation math.
- **Synced Lyrics Engine**: 6-tier fallback fetcher powered by LRCLIB API. Includes Vietnamese accent removal and K-Pop Hangul-to-Romaja Latin converter.
- **Memory Efficient**: Direct JPEG scaling and custom line-by-line RGB bitmap rendering into micro-buffers to prevent RAM overflow.

---

## Hardware Pinout & Wiring Diagram

Both TFT displays share the hardware SPI data lines (`SCLK` and `MOSI`) as well as the `DC` (Data/Command) and `RST` (Reset) control pins, while using independent Chip Select (`CS`) pins (`GPIO 15` for Display 1 and `GPIO 12` for Display 2).

### Shared SPI & Control Pins
| ESP32-S3 Pin | Function | Display Pins |
| :--- | :--- | :--- |
| **GPIO 8** | `SPI_SCLK` (Clock) | `SCK` / `SCL` / `CLK` on both displays |
| **GPIO 9** | `SPI_MOSI` (Master Out Slave In) | `SDA` / `MOSI` on both displays |
| **GPIO 11** | `TFT_DC` (Data / Command) | `DC` / `RS` / `A0` on both displays |
| **GPIO 10** | `TFT_RST` (Reset) | `RST` / `RESET` on both displays |

### Display 1: Main Display (ILI9341 320x240)
| ESP32-S3 Pin | Function | ILI9341 Pin |
| :--- | :--- | :--- |
| **GPIO 15** | `TFT1_CS` | `CS` |
| **GPIO 11** | `TFT1_DC` (Shared) | `DC` / `RS` |
| **GPIO 10** | `TFT1_RST` (Shared) | `RST` / `RESET` |
| **3.3V / 5V** | Power | `VCC` & `LED` (Backlight) |
| **GND** | Ground | `GND` |

### Display 2: Secondary Display (ST7735 160x128)
| ESP32-S3 Pin | Function | ST7735 Pin |
| :--- | :--- | :--- |
| **GPIO 12** | `TFT2_CS` | `CS` |
| **GPIO 11** | `TFT2_DC` (Shared with Display 1) | `DC` / `A0` |
| **GPIO 10** | `TFT2_RST` (Shared with Display 1) | `RST` / `RESET` |
| **3.3V** | Power | `VCC` & `LED` (Backlight) |
| **GND** | Ground | `GND` |

---

## Customizing GPIO Pins

If you want to use different GPIO pins on your ESP32-S3 board, open [`include/config.h`](include/config.h) and modify the pin macro definitions:

```cpp
// SPI Bus (Shared)
#define SPI_SCLK 8  // Shared SCK/SCLK pin
#define SPI_MOSI 9  // Shared MOSI/SDA pin

// Display 1: ILI9341 (320x240 - Main display for Spotify UI)
#define TFT1_CS   15  // Main display Chip Select
#define TFT1_DC   11  // Shared DC/RS pin with Display 2
#define TFT1_RST  10  // Shared RST pin with Display 2

// Display 2: ST7735 (160x128 - Secondary display for Album Cover)
#define TFT2_CS   12  // Secondary display Chip Select
#define TFT2_DC   11  // Shared DC/A0 pin with TFT1_DC
#define TFT2_RST  10  // Shared RST pin with TFT1_RST
```

---

## Configuration (WiFi & Spotify Credentials)

Before compiling the firmware, update your credentials in [`src/globals.cpp`](src/globals.cpp):

```cpp
// WiFi Configuration
const char* ssid                  = "YOUR_WIFI_SSID";
const char* password              = "YOUR_WIFI_PASSWORD";

// Spotify Developer Credentials
const char* spotify_client_id     = "YOUR_SPOTIFY_CLIENT_ID";
const char* spotify_client_secret = "YOUR_SPOTIFY_CLIENT_SECRET";
const char* spotify_refresh_token = "YOUR_SPOTIFY_REFRESH_TOKEN";
```

### How to get Spotify Credentials:
1. Go to the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard) and create an App.
2. Obtain your **Client ID** and **Client Secret**.
3. Set `http://localhost:8888/callback` as a Redirect URI in your app settings.
4. Generate a **Refresh Token** with the `user-read-currently-playing` and `user-read-playback-state` scopes.

---

## Building & Flashing

1. Install **Visual Studio Code** and the **PlatformIO IDE** extension.
2. Clone or download this repository.
3. Open the project folder in PlatformIO.
4. Fill in your WiFi & Spotify credentials in `src/globals.cpp`.
5. Connect your ESP32-S3 board via USB.
6. Click **Build** (`✓`) and **Upload** (`→`) in PlatformIO.

---

## Project Structure

```text
├── include/
│   ├── config.h            # Pinout definitions & Spotify color palette
│   ├── globals.h           # Global state variables & structs
│   ├── display_manager.h   # Vinyl rotation & Spotify logo header
│   ├── lyrics_manager.h    # LRCLIB API lyrics fetcher & parser
│   ├── spotify_client.h    # Spotify API & HTTPS cover downloader
│   ├── spotify_logo.h      # PROGMEM logo bitmaps
│   └── ui_renderer.h       # UI rendering logic for both screens
├── src/
│   ├── globals.cpp         # Wifi/Spotify credentials & state init
│   ├── display_manager.cpp # Vinyl disc buffer processing
│   ├── lyrics_manager.cpp  # LRC parser & K-Pop/Vietnamese text converter
│   ├── main.cpp            # setup() and loop() entry point
│   ├── spotify_client.cpp  # Network task & HTTPS API calls
│   └── ui_renderer.cpp     # Screen 1 & Screen 2 rendering loops
├── platformio.ini          # PlatformIO build & library config
├── .gitignore              # Git ignore rules
└── README.md               # Documentation
```

---

## License

This project is open-source and available under the [MIT License](LICENSE).
