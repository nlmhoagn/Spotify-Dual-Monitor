#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <Arduino.h>
#include "globals.h"

// Main Display ILI9341 UI components
void drawHeader();
void drawSongInfoTop();
void updateProgressUI();
void drawControls();
void updateEQ();
void updateScreen1();

// Secondary Display ST7735 UI components
void renderAlbumCoverOnCore1();
void updateScreen2Dynamic();

// Loading screen while connecting to WiFi
void runLoadingScreen();

#endif // UI_RENDERER_H
