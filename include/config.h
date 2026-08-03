#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// -------------------------------------------------------------
// THÔNG TIN WI-FI & SPOTIFY CREDENTIALS
// -------------------------------------------------------------
extern const char* ssid;
extern const char* password;
extern const char* spotify_client_id;
extern const char* spotify_client_secret;
extern const char* spotify_refresh_token;

// -------------------------------------------------------------
// ĐỊNH NGHĨA CHÂN SPI & 2 MÀN HÌNH TFT
// -------------------------------------------------------------
#define SPI_SCLK 8  // SCK/SCLK dùng chung
#define SPI_MOSI 9  // MOSI/SDA dùng chung

// Màn 1: ILI9341 (320x240 - Màn lớn hiển thị Spotify UI)
#define TFT1_CS   12
#define TFT1_DC   11
#define TFT1_RST  10

// Màn 2: ST7735 (160x128 - Màn nhỏ hiển thị Bìa Album)
#define TFT2_CS   16
#define TFT2_DC   15
#define TFT2_RST  17

// -------------------------------------------------------------
// BẢNG MÀU SPOTIFY (COLOR PALETTE RGB565)
// -------------------------------------------------------------
#define BG      0x0000  // Đen tuyền
#define SPGREEN 0x06C4  // Xanh lá đặc trưng Spotify
#define WHITE   0xFFFF  // Trắng
#define GRAY    0x4228  // Xám vừa
#define DGRAY   0x1082  // Xám đậm
#define LGRAY   0x8C71  // Xám sáng
#define DARK    0x0841  // Tối / Thanh Header

#define MAX_LYRICS 150

#endif // CONFIG_H
