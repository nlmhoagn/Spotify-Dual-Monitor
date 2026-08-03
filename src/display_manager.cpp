#include "display_manager.h"
#include <TJpg_Decoder.h>
#include <math.h>

// Buffer for 60x60 circular Album Cover label
static uint16_t coverLabelBuffer[60 * 60];
static uint16_t tempCover75[75 * 75];
static bool labelBufferValid = false;

// -------------------------------------------------------------
// DRAW SPOTIFY LOGO FROM PROGMEM IMAGE ARRAY
// -------------------------------------------------------------
void drawSpotifyLogo(Adafruit_GFX &gfx, int cx, int cy, int r, uint16_t bgCol) {
  const uint16_t* logoArray = NULL;
  int size = 0;

  if (r >= 30) {
    logoArray = spotify_logo_72x72;
    size = 72;
  } else if (r >= 14) {
    logoArray = spotify_logo_32x32;
    size = 32;
  } else if (r >= 10) {
    logoArray = spotify_logo_24x24;
    size = 24;
  } else {
    logoArray = spotify_logo_16x16;
    size = 16;
  }

  int x0 = cx - size / 2;
  int y0 = cy - size / 2;

  uint16_t lineBuffer[72];

  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      uint16_t p = pgm_read_word(&logoArray[y * size + x]);
      lineBuffer[x] = (p == 0x0000) ? bgCol : p;
    }
    gfx.drawRGBBitmap(x0, y0 + y, lineBuffer, size, 1);
  }
}

// -------------------------------------------------------------
// SCALE & ROTATE 76px VINYL DISC
// -------------------------------------------------------------
void prepareFallbackLabelBuffer() {
  for (int y = 0; y < 60; y++) {
    int sy = (y * 72) / 60;
    for (int x = 0; x < 60; x++) {
      int sx = (x * 72) / 60;
      uint16_t p = pgm_read_word(&spotify_logo_72x72[sy * 72 + sx]);
      coverLabelBuffer[y * 60 + x] = (p == 0x0000) ? 0x1082 : p;
    }
  }
  labelBufferValid = true;
}

bool store_cover_jpg_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  int ox = x - 42;
  int oy = y - 21;
  for (int j = 0; j < h; j++) {
    int py = oy + j;
    if (py < 0 || py >= 75) continue;
    for (int i = 0; i < w; i++) {
      int px = ox + i;
      if (px < 0 || px >= 75) continue;
      tempCover75[py * 75 + px] = bitmap[j * w + i];
    }
  }
  return 1;
}

void processDecodedCoverToLabel() {
  for (int y = 0; y < 60; y++) {
    int sy = (y * 75) / 60;
    for (int x = 0; x < 60; x++) {
      int sx = (x * 75) / 60;
      coverLabelBuffer[y * 60 + x] = tempCover75[sy * 75 + sx];
    }
  }
  labelBufferValid = true;
}

void renderRotatedVinylDisc(float angle) {
  if (!labelBufferValid) prepareFallbackLabelBuffer();

  int cos_fp = (int)(cos(angle) * 256.0f);
  int sin_fp = (int)(sin(angle) * 256.0f);

  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, LOW);

  static uint16_t lineBuf[77];

  for (int dy = -38; dy <= 38; dy++) {
    int py = 58 + dy;
    int dy_sin = dy * sin_fp;
    int dy_cos = dy * cos_fp;
    int idx = 0;

    for (int dx = -38; dx <= 38; dx++) {
      int distSq = dx * dx + dy * dy;

      if (distSq <= 9) {
        lineBuf[idx] = ST77XX_BLACK; // Spindle hole (r=3)
      } else if (distSq <= 900) { // Circular Album Cover Label (r=30, D=60px)
        int sx = 30 + ((dx * cos_fp + dy_sin) >> 8);
        int sy = 30 + ((-dx * sin_fp + dy_cos) >> 8);
        if (sx < 0) sx = 0; else if (sx >= 60) sx = 59;
        if (sy < 0) sy = 0; else if (sy >= 60) sy = 59;

        lineBuf[idx] = coverLabelBuffer[sy * 60 + sx];
      } else if (distSq <= 1444) { // Outer Black Vinyl Ring (r=38, D=76px)
        int dist = (int)sqrt((float)distSq);
        if (dist == 38 || dist == 35 || dist == 32) {
          lineBuf[idx] = 0x3186; // Groove line gray
        } else {
          lineBuf[idx] = 0x1082; // Black Vinyl body
        }
      } else {
        lineBuf[idx] = ST77XX_BLACK;
      }
      idx++;
    }
    tft2.drawRGBBitmap(42, py, lineBuf, 77, 1);
  }

  digitalWrite(TFT2_CS, HIGH);
}
