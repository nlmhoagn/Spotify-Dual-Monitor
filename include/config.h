#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// -------------------------------------------------------------
// WI-FI & SPOTIFY CREDENTIALS
// -------------------------------------------------------------
extern String ssid;
extern String password;
extern String spotify_client_id;
extern String spotify_client_secret;
extern String spotify_refresh_token;

// -------------------------------------------------------------
// SPI & DUAL TFT DISPLAY PIN DEFINITIONS
// -------------------------------------------------------------
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

// -------------------------------------------------------------
// PHYSICAL CONTROL BUTTON PINS & DEBOUNCE
// -------------------------------------------------------------
#define BUTTON_PLAY_PAUSE_PIN 4  // GPIO 4: Play / Pause toggle button
#define BUTTON_NEXT_PIN       5  // GPIO 5: Skip Next track button
#define BUTTON_PREV_PIN       6  // GPIO 6: Skip Previous track button
#define DEBOUNCE_DELAY_MS    50  // Debounce threshold in milliseconds

// -------------------------------------------------------------
// SPOTIFY COLOR PALETTE (RGB565)
// -------------------------------------------------------------
#define BG      0x0000  // Pure Black
#define SPGREEN 0x06C4  // Spotify Green
#define WHITE   0xFFFF  // Pure White
#define GRAY    0x4228  // Medium Gray
#define DGRAY   0x1082  // Dark Gray
#define LGRAY   0x8C71  // Light Gray
#define DARK    0x0841  // Dark Header Bar

#define MAX_LYRICS 150

#endif // CONFIG_H
