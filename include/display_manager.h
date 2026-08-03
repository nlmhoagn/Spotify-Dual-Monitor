#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "globals.h"
#include "spotify_logo.h"

// Vẽ Logo Spotify
void drawSpotifyLogo(Adafruit_GFX &gfx, int cx, int cy, int r, uint16_t bgCol);

// Quản lý đĩa Vinyl tròn 76px & Album Cover
void prepareFallbackLabelBuffer();
bool store_cover_jpg_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);
void processDecodedCoverToLabel();
void renderRotatedVinylDisc(float angle);

#endif // DISPLAY_MANAGER_H
