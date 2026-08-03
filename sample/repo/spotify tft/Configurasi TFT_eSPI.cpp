//                            USER DEFINED SETTINGS
#define USER_SETUP_INFO "User_Setup"

// ##################################################################################
// Section 1. Driver
// ##################################################################################

// ? DIUBAH: ST7789 diaktifkan, ILI9341 di-comment
//#define ILI9341_DRIVER
#define ST7789_DRIVER

// ? DIUBAH: Ukuran layar 240x240
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// ##################################################################################
// Section 2. Pin definitions untuk ESP32
// ##################################################################################

// ? DIUBAH: Pin ESP32 diaktifkan dan disesuaikan
#define TFT_MOSI 23   // SDA
#define TFT_SCLK 18   // SCL
#define TFT_CS   -1   // Tidak ada pin CS di LCD kamu
#define TFT_DC    2   // DC
#define TFT_RST   4   // RES
#define TFT_BL   15   // BLK (backlight)

#define TFT_BACKLIGHT_ON HIGH

// ##################################################################################
// Section 3. Fonts
// ##################################################################################

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// ##################################################################################
// Section 4. SPI Speed
// ##################################################################################

// ? DIUBAH: Naikkan SPI frequency ke 40MHz untuk ST7789
#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
