# ESP32-S3 Spotify Dual-Monitor & Karaoke Lyrics Player

A feature-rich, dual-display Spotify music player and smart desktop companion powered by ESP32-S3 and FreeRTOS. It streams real-time Spotify playback status, renders live synced Karaoke lyrics on a main ILI9341 display, displays a rotating vinyl album cover on a secondary ST7735 display, and includes physical button controls, an on-device Web Control Dashboard, and a Captive Portal Wi-Fi setup manager.

---

## Hardware & Components Required

| Component | Model / Specification | Description |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32-S3 Development Board | Dual-Core Xtensa LX7, 2.4GHz Wi-Fi, 8MB Flash (PSRAM recommended) |
| **Main Display (Display 1)** | ILI9341 2.8" SPI TFT LCD (320x240) | Renders Spotify UI, track info, progress bar, audio EQ visualizer & 3-line Karaoke lyrics |
| **Secondary Display (Display 2)** | ST7735 1.44" / 1.8" SPI TFT LCD (160x128) | Renders rotating 76px vinyl record with decoded album cover art OR full square cover |
| **Control Buttons** | 3x Tactile Push Buttons | Hardware buttons for Play/Pause, Skip Next, and Skip Previous track controls |
| **Power Supply** | USB Type-C Cable (5V / 1A+) | Powers the ESP32-S3 board, displays, and control logic |
| **Wiring** | DuPont Jumper Wires / Breadboard | For SPI bus, control pins, and button interconnections |

---

## Features

- **Dual Display Architecture**: Driven over a shared SPI bus using chip-select multiplexing.
  - **ILI9341 (320x240 Main Screen)**: Track title, artist name, progress bar with elapsed/duration time, dynamic audio EQ visualizer, and 3-line synced karaoke lyrics.
  - **ST7735 (160x128 Secondary Screen)**: Supports 2 display modes:
    - *Spinning Vinyl Mode*: Rotating 76px vinyl disc with album cover center label.
    - *Full Cover Mode*: Full-screen 108x108 square album cover art.
- **Physical Button Control**: 3 hardware push buttons with non-blocking software debouncing for instant playback control (Play/Pause, Skip Next, Skip Previous).
- **On-Device Web Control Dashboard**: When connected to Wi-Fi, access `http://spotify-monitor.local` (mDNS) or the device's IP address from any browser to:
  - Remotely control Spotify playback (Play/Pause, Skip Next, Skip Previous).
  - Customize UI Accent Color themes (Spotify Green, Cyan, Gold, Purple, Orange, White).
  - Toggle Display Modes (Spinning Vinyl vs Full Square Cover).
  - Toggle Audio EQ visualizer on/off.
  - Reset saved Wi-Fi & Spotify credentials.
- **Captive Portal AP Setup Manager**: On first boot (or if Wi-Fi fails), ESP32 launches an Access Point (`ESP32-Spotify-Config`) with a captive portal web setup interface to easily configure Wi-Fi and Spotify credentials without re-flashing the firmware. Credentials are saved permanently in ESP32 NVS (Non-Volatile Storage).
- **Dual-Core FreeRTOS Multitasking**:
  - **Core 0**: Asynchronous network tasks (Spotify OAuth2 token refresh, HTTPS album cover downloads, LRCLIB API queries, Web Dashboard server, Spotify API commands).
  - **Core 1**: Real-time smooth UI rendering, time interpolation, vinyl rotation math, and button debouncing.
- **Synced Lyrics Engine**: 6-tier fallback fetcher powered by LRCLIB API. Includes automatic Vietnamese accent removal and K-Pop Hangul-to-Romaja Latin converter.
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

### Physical Control Buttons
| ESP32-S3 Pin | Function | Button Connection |
| :--- | :--- | :--- |
| **GPIO 4** | Play / Pause Toggle | Push button to GND (`INPUT_PULLUP`) |
| **GPIO 5** | Skip Next Track | Push button to GND (`INPUT_PULLUP`) |
| **GPIO 6** | Skip Previous Track | Push button to GND (`INPUT_PULLUP`) |

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

// Physical Control Buttons
#define BUTTON_PLAY_PAUSE_PIN 4  // Play / Pause toggle
#define BUTTON_NEXT_PIN       5  // Skip Next track
#define BUTTON_PREV_PIN       6  // Skip Previous track
```

---

## Configuration & Setup

### Method 1: On-Device Web Captive Portal (Recommended)
1. Power on the device. If no Wi-Fi credentials are saved, it creates a Wi-Fi Access Point named **`ESP32-Spotify-Config`**.
2. Connect your phone or laptop to `ESP32-Spotify-Config`.
3. A captive portal page will open automatically (or navigate to `192.168.4.1`).
4. Enter your Wi-Fi SSID, Password, Spotify Client ID, Client Secret, and Refresh Token.
5. Click **Save & Restart**. The credentials are stored permanently in ESP32 NVS.

### Method 2: Manual Credentials in Code
Before compiling the firmware, you can also set credentials directly in [`src/globals.cpp`](src/globals.cpp):

```cpp
// WiFi Configuration
String ssid                  = "YOUR_WIFI_SSID";
String password              = "YOUR_WIFI_PASSWORD";

// Spotify Developer Credentials
String spotify_client_id     = "YOUR_SPOTIFY_CLIENT_ID";
String spotify_client_secret = "YOUR_SPOTIFY_CLIENT_SECRET";
String spotify_refresh_token = "YOUR_SPOTIFY_REFRESH_TOKEN";
```

### How to get Spotify Credentials:
1. Go to the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard) and create an App.
2. Obtain your **Client ID** and **Client Secret**.
3. Set `http://localhost:8888/callback` as a Redirect URI in your app settings.
4. Generate a **Refresh Token** with `user-read-currently-playing`, `user-read-playback-state`, and `user-modify-playback-state` scopes.

---

## Web Control Dashboard

Once connected to Wi-Fi, open any web browser on your network and navigate to:
- **`http://spotify-monitor.local`** (mDNS) or the device's IP address.

From the web dashboard, you can:
- **Remote Control Playback**: Play/Pause, Skip Next, Skip Previous.
- **Theme Color Customization**: Select accent colors (Spotify Green, Cyan, Gold, Purple, Orange, White).
- **Display Mode**: Switch Display 2 between Rotating Vinyl Disc and Full Square Album Cover.
- **Visualizer**: Toggle Audio EQ Visualizer ON / OFF.
- **Wi-Fi Reset**: Wipe stored NVS credentials to re-enter setup mode.

---

## Building & Flashing

1. Install **Visual Studio Code** and the **PlatformIO IDE** extension.
2. Clone or download this repository.
3. Open the project folder in PlatformIO.
4. Set up credentials via Web Captive Portal or `src/globals.cpp`.
5. Connect your ESP32-S3 board via USB.
6. Click **Build** (`✓`) and **Upload** (`→`) in PlatformIO.

---

## Project Structure

```text
├── include/
│   ├── button_manager.h    # Physical GPIO button handling & debouncing
│   ├── config.h            # Pinout definitions, Spotify colors & parameters
│   ├── config_manager.h    # NVS storage, Captive Portal AP & Web Dashboard
│   ├── display_manager.h   # Vinyl rotation math & Spotify header logo
│   ├── globals.h           # Global state variables, mutexes & structs
│   ├── lyrics_manager.h    # LRCLIB API lyrics fetcher & text converters
│   ├── spotify_client.h    # Spotify HTTPS API, tokens & cover downloader
│   ├── spotify_logo.h      # PROGMEM logo bitmaps
│   └── ui_renderer.h       # UI rendering loops for ILI9341 & ST7735
├── src/
│   ├── button_manager.cpp  # Debounced button interrupt & command dispatch
│   ├── config_manager.cpp  # WebServer endpoints, Captive Portal & NVS
│   ├── display_manager.cpp # Vinyl disc buffer processing
│   ├── globals.cpp         # System credentials, state initialization
│   ├── lyrics_manager.cpp  # LRC parser & K-Pop/Vietnamese text converter
│   ├── main.cpp            # Core 0 / Core 1 FreeRTOS setup & loop entry
│   ├── spotify_client.cpp  # Network task & Spotify REST API calls
│   └── ui_renderer.cpp     # Dual screen rendering implementation
├── platformio.ini          # PlatformIO build configuration & libraries
├── .gitignore              # Git ignore rules
└── README.md               # Documentation
```

---

## License

This project is open-source and available under the [MIT License](LICENSE).
