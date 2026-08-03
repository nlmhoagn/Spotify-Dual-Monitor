#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <Arduino.h>
#include "globals.h"

// Màn lớn ILI9341 UI components
void drawHeader();
void drawSongInfoTop();
void updateProgressUI();
void drawControls();
void updateEQ();
void updateScreen1();

// Màn nhỏ ST7735 UI components
void renderAlbumCoverOnCore1();
void updateScreen2Dynamic();

// Loading screen chờ kết nối WiFi
void runLoadingScreen();

#endif // UI_RENDERER_H
