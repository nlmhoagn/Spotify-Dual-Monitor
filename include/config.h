#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// -------------------------------------------------------------
// WI-FI & SPOTIFY CREDENTIALS
// -------------------------------------------------------------
extern const char* ssid;
extern const char* password;
extern const char* spotify_client_id;
extern const char* spotify_client_secret;
extern const char* spotify_refresh_token;

// -------------------------------------------------------------
// SPI & DUAL TFT DISPLAY PIN DEFINITIONS
// -------------------------------------------------------------
#define SPI_SCLK 8  // Shared SCK/SCLK pin
#define SPI_MOSI 9  // Shared MOSI/SDA pin

// Display 1: ILI9341 (320x240 - Main display for Spotify UI)
#define TFT1_CS   12
#define TFT1_DC   11
#define TFT1_RST  10

// Display 2: ST7735 (160x128 - Secondary display for Album Cover)
#define TFT2_CS   16
#define TFT2_DC   15
#define TFT2_RST  17

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
