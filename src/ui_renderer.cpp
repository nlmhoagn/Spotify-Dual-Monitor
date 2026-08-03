#include "ui_renderer.h"
#include "display_manager.h"
#include "lyrics_manager.h"
#include <WiFi.h>
#include <TJpg_Decoder.h>

// -------------------------------------------------------------
// SECONDARY DISPLAY ST7735 (160x128) - ALBUM COVER & VINYL DISC
// -------------------------------------------------------------
void renderAlbumCoverOnCore1() {
  std::vector<uint8_t> localBuffer;
  bool renderNow = false;

  xSemaphoreTake(dataMutex, portMAX_DELAY);
  if (newCoverAvailable) {
    localBuffer = rawCoverBuffer;
    newCoverAvailable = false;
    renderNow = true;
  }
  xSemaphoreGive(dataMutex);

  if (renderNow) {
    digitalWrite(TFT1_CS, HIGH);
    digitalWrite(TFT2_CS, LOW);
    tft2.fillScreen(ST77XX_BLACK);

    // 1. Small display header: DARK background + NOW PLAYING (left) + username (right)
    tft2.fillRect(0, 0, 160, 18, DARK);
    tft2.setTextSize(1);
    tft2.setTextColor(SPGREEN, DARK);
    tft2.setCursor(6, 5);
    tft2.print("NOW PLAYING");

    tft2.setTextSize(1);
    tft2.setTextColor(0x7BEF, DARK);
    tft2.setCursor(100, 5);
    tft2.print("@nlmhoagn");

    // 2. Clear vinyl area before drawing
    tft2.fillRect(40, 20, 80, 78, ST77XX_BLACK);

    // 3. Decode & store Album Cover into circular Vinyl Label (Center X: 80, Y: 58)
    if (localBuffer.size() > 1000) {
      TJpgDec.setJpgScale(4);
      TJpgDec.setSwapBytes(false);
      TJpgDec.setCallback(store_cover_jpg_output);
      TJpgDec.drawJpg(42, 21, localBuffer.data(), localBuffer.size());
      processDecodedCoverToLabel();
    } else {
      prepareFallbackLabelBuffer();
    }

    renderRotatedVinylDisc(0.0f);

    digitalWrite(TFT1_CS, HIGH);
    digitalWrite(TFT2_CS, LOW);
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    cover_loaded_tft2 = true;
    tft2_title_dirty = true;
    String trackName = removeVietnameseAccents(current_track_name);
    String artistName = removeVietnameseAccents(current_artist_name);
    xSemaphoreGive(dataMutex);

    if (artistName.length() > 24) artistName = artistName.substring(0, 22) + "..";

    int16_t x1, y1; uint16_t w, h;
    tft2.setTextSize(1);

    tft2.getTextBounds(artistName.c_str(), 0, 0, &x1, &y1, &w, &h);
    int aX = (160 - w) / 2; if (aX < 2) aX = 2;
    tft2.setTextColor(SPGREEN, ST77XX_BLACK);
    tft2.setCursor(aX, 111);
    tft2.print(artistName);

    tft2.drawFastHLine(10, 123, 140, 0x3333);

    digitalWrite(TFT2_CS, HIGH);
    Serial.println("[COVER] Rendered 75x75 centered cover + UI safely on Core 1!");
  }
}

void updateScreen2Dynamic() {
  xSemaphoreTake(dataMutex, portMAX_DELAY);
  long prog = progress_ms;
  long dur = duration_ms;
  bool playing = is_playing;
  bool coverLoaded = cover_loaded_tft2;
  String fullTrackName = removeVietnameseAccents(current_track_name);
  xSemaphoreGive(dataMutex);

  static bool prev_cover_loaded = true;

  if (!coverLoaded) {
    digitalWrite(TFT1_CS, HIGH);
    digitalWrite(TFT2_CS, LOW);

    if (prev_cover_loaded) {
      prev_cover_loaded = false;
      tft2.fillRect(0, 18, 160, 110, ST77XX_BLACK);

      tft2.setTextSize(1);
      tft2.setTextColor(WHITE, ST77XX_BLACK);
      tft2.setCursor(52, 101);
      tft2.print("Loading...");

      tft2.setTextColor(SPGREEN, ST77XX_BLACK);
      tft2.setCursor(40, 111);
      tft2.print("Fetching cover");

      int leftX[4]  = { 8, 15, 22, 29 };
      int rightX[4] = { 127, 134, 141, 148 };
      for (int i = 0; i < 4; i++) {
        tft2.fillRect(leftX[i], 35, 4, 55, ST77XX_BLACK);
        tft2.fillRect(rightX[i], 35, 4, 55, ST77XX_BLACK);
      }
    }

    static float loading_angle = 0.0f;
    loading_angle += 0.05f;
    if (loading_angle >= 6.28318f) loading_angle -= 6.28318f;
    prepareFallbackLabelBuffer();
    renderRotatedVinylDisc(loading_angle);

    static int loadPulse = 0;
    loadPulse = (loadPulse + 4) % 140;
    tft2.drawFastHLine(10, 123, 140, 0x2222);
    tft2.drawFastHLine(10 + (loadPulse % 100), 123, 40, SPGREEN);

    digitalWrite(TFT2_CS, HIGH);
    return;
  }

  if (!prev_cover_loaded) {
    prev_cover_loaded = true;
  }

  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, LOW);

  if (dur > 0) {
    int fill = (prog * 140) / dur;
    if (fill > 140) fill = 140;
    static int prev_fill2 = -1;
    if (fill != prev_fill2) {
      prev_fill2 = fill;
      tft2.drawFastHLine(10, 123, 140, 0x2222);
      if (fill > 0) tft2.drawFastHLine(10, 123, fill, SPGREEN);
    }
  }

  if (fullTrackName.length() > 0) {
    int textW = fullTrackName.length() * 6;
    static String last_rendered_title = "";
    static int prev_offset = -1;

    bool force_draw = (fullTrackName != last_rendered_title || tft2_title_dirty);
    if (force_draw) {
      last_rendered_title = fullTrackName;
      tft2_title_dirty = false;
      prev_offset = -1;
    }

    tft2.setTextWrap(false);

    if (textW <= 152) {
      if (force_draw) {
        tft2.fillRect(4, 100, 152, 10, ST77XX_BLACK);
        int tX = (160 - textW) / 2; if (tX < 4) tX = 4;
        tft2.setTextColor(WHITE, ST77XX_BLACK);
        tft2.setTextSize(1);
        tft2.setCursor(tX, 101);
        tft2.print(fullTrackName);
      }
    } else {
      int maxShift = textW - 148;
      int cycle = (millis() / 50) % (maxShift * 2 + 40);
      int offset = 0;
      if (cycle < 20) { offset = 0; }
      else if (cycle < 20 + maxShift) { offset = cycle - 20; }
      else if (cycle < 20 + maxShift + 20) { offset = maxShift; }
      else { offset = maxShift - (cycle - (20 + maxShift + 20)); }

      if (offset != prev_offset || force_draw) {
        prev_offset = offset;

        tft2.fillRect(4, 100, 152, 10, ST77XX_BLACK);
        tft2.setTextColor(WHITE, ST77XX_BLACK);
        tft2.setTextSize(1);
        tft2.setCursor(6 - offset, 101);
        tft2.print(fullTrackName);

        tft2.fillRect(0, 100, 4, 10, ST77XX_BLACK);
        tft2.fillRect(156, 100, 4, 10, ST77XX_BLACK);
      }
    }
  }

  static float vinyl_angle = 0.0f;
  if (playing) {
    vinyl_angle += 0.04f;
    if (vinyl_angle >= 6.28318f) vinyl_angle -= 6.28318f;
    renderRotatedVinylDisc(vinyl_angle);
  }

  static unsigned long last_eq2 = 0;
  if (millis() - last_eq2 > 100) {
    last_eq2 = millis();

    int leftX[4]  = { 8, 15, 22, 29 };
    int rightX[4] = { 127, 134, 141, 148 };
    static int prevH_L[4] = {-1, -1, -1, -1};
    static int prevH_R[4] = {-1, -1, -1, -1};

    for (int i = 0; i < 4; i++) {
      int h = (coverLoaded && playing) ? random(4, 50) : 0;

      if (h != prevH_L[i]) {
        prevH_L[i] = h;
        tft2.fillRect(leftX[i], 35, 4, 55, ST77XX_BLACK);
        if (h > 0) tft2.fillRoundRect(leftX[i], 90 - h, 4, h, 1, SPGREEN);
      }

      if (h != prevH_R[i]) {
        prevH_R[i] = h;
        tft2.fillRect(rightX[i], 35, 4, 55, ST77XX_BLACK);
        if (h > 0) tft2.fillRoundRect(rightX[i], 90 - h, 4, h, 1, SPGREEN);
      }
    }
  }

  digitalWrite(TFT2_CS, HIGH);
}

// -------------------------------------------------------------
// LANDSCAPE UI FOR MAIN DISPLAY ILI9341 (320x240)
// -------------------------------------------------------------
void drawHeader() {
  tft1.fillRect(0, 0, 320, 32, DARK);
  drawSpotifyLogo(tft1, 18, 16, 11, DARK);

  tft1.setTextColor(WHITE, DARK);
  tft1.setTextSize(1);
  tft1.setCursor(34, 8);
  tft1.print("SPOTIFY");

  tft1.setTextColor(SPGREEN, DARK);
  tft1.setCursor(34, 18);
  tft1.print("Now Playing");

  tft1.setTextColor(LGRAY, DARK);
  tft1.setCursor(275, 12);
  tft1.print("LIVE");
}

void drawSongInfoTop() {
  tft1.fillRect(0, 34, 320, 48, BG);

  String cleanTitle = removeVietnameseAccents(current_track_name);
  tft1.setTextColor(WHITE, BG);
  tft1.setTextSize(2);

  int maxCharsPerLine = 24;
  int artistY = 60;

  if (cleanTitle.length() <= maxCharsPerLine) {
    tft1.setCursor(14, 42);
    tft1.print(cleanTitle);
    artistY = 60;
  } else {
    int splitIdx = cleanTitle.lastIndexOf(' ', maxCharsPerLine);
    if (splitIdx == -1 || splitIdx < 4) splitIdx = maxCharsPerLine;

    String line1 = cleanTitle.substring(0, splitIdx);
    String line2 = cleanTitle.substring(splitIdx);
    line1.trim(); line2.trim();

    if (line2.length() > maxCharsPerLine) {
      line2 = line2.substring(0, maxCharsPerLine - 2) + "..";
    }

    tft1.setCursor(14, 36);
    tft1.print(line1);
    tft1.setCursor(14, 54);
    tft1.print(line2);
    artistY = 72;
  }

  tft1.setTextColor(SPGREEN, BG);
  tft1.setTextSize(1);
  tft1.setCursor(14, artistY);
  String cleanArtist = removeVietnameseAccents(current_artist_name);
  if (cleanArtist.length() > 45) cleanArtist = cleanArtist.substring(0, 43) + "..";
  tft1.print(cleanArtist);
}

void updateEQ() {
  unsigned long now = millis();
  if (now - last_eq_update < 40) return;
  last_eq_update = now;

  int eqX = 10;
  int eqY = 185;
  int bw = 26;
  int gap = 5;

  for (int i = 0; i < 10; i++) {
    eqT[i] = is_playing ? random(4, 28) : 3;
    int bx = eqX + i * (bw + gap);

    if (eqH[i] != eqT[i]) {
      tft1.fillRect(bx, eqY - 30, bw, 30, BG);

      if (eqH[i] < eqT[i]) eqH[i] += 4;
      if (eqH[i] > eqT[i]) eqH[i] -= 4;
      if (eqH[i] < 3) eqH[i] = 3;

      tft1.fillRoundRect(bx, eqY - eqH[i], bw, eqH[i], 2, SPGREEN);
    }
  }
}

void updateProgressUI() {
  int x = 12;
  int y = 192;
  int w = 296;

  float progRatio = 0;
  if (duration_ms > 0) {
    progRatio = (float)progress_ms / (float)duration_ms;
  }
  if (progRatio > 1.0) progRatio = 1.0;

  int fill = progRatio * w;
  int knobX = x + fill;

  long current_sec = progress_ms / 1000;

  if (knobX != prev_knob_x) {
    if (prev_knob_x >= 0) {
      tft1.fillCircle(prev_knob_x, y + 1, 4, BG);
    }

    tft1.fillRoundRect(x, y, w, 3, 1, DGRAY);
    if (fill > 0) {
      tft1.fillRoundRect(x, y, fill, 3, 1, SPGREEN);
    }

    tft1.fillCircle(knobX, y + 1, 4, WHITE);
    prev_knob_x = knobX;
  }

  if (current_sec != prev_progress_sec) {
    prev_progress_sec = current_sec;

    tft1.setTextColor(LGRAY, BG);
    tft1.setTextSize(1);

    tft1.setCursor(12, 200);
    char curTime[10];
    sprintf(curTime, "%02ld:%02ld", (progress_ms / 1000) / 60, (progress_ms / 1000) % 60);
    tft1.print(curTime);

    tft1.setCursor(280, 200);
    char totalTime[10];
    sprintf(totalTime, "%02ld:%02ld", (duration_ms / 1000) / 60, (duration_ms / 1000) % 60);
    tft1.print(totalTime);
  }
}

void drawControls() {
  int cy = 222;

  tft1.fillTriangle(95, cy, 108, cy - 7, 108, cy + 7, LGRAY);

  tft1.fillCircle(160, cy, 13, WHITE);
  if (is_playing) {
    tft1.fillRect(155, cy - 5, 4, 10, BG);
    tft1.fillRect(161, cy - 5, 4, 10, BG);
  } else {
    tft1.fillTriangle(156, cy - 5, 156, cy + 5, 166, cy, BG);
  }

  tft1.fillTriangle(225, cy, 212, cy - 7, 212, cy + 7, LGRAY);

  tft1.setTextSize(1);
  tft1.setTextColor(LGRAY, BG);
  tft1.setCursor(256, 226);
  tft1.print("@nlmhoagn");
}

void updateScreen1() {
  xSemaphoreTake(dataMutex, portMAX_DELAY);
  String track_name_snap = current_track_name;
  bool is_playing_snap = is_playing;
  xSemaphoreGive(dataMutex);

  if (track_name_snap == "") return;

  digitalWrite(TFT2_CS, HIGH);
  digitalWrite(TFT1_CS, LOW);

  static bool first_run = true;
  static bool prev_is_playing_state = !is_playing_snap;

  if (track_name_snap != prev_title || is_playing_snap != prev_is_playing_state) {
    bool track_changed = (track_name_snap != prev_title);
    prev_title = track_name_snap;
    prev_is_playing_state = is_playing_snap;

    if (first_run) {
      first_run = false;
      tft1.fillScreen(BG);
      drawHeader();
      drawSongInfoTop();
      prev_lyric_idx = -2;
      prev_progress_sec = -1;
      prev_knob_x = -1;
    } else if (track_changed) {
      tft1.fillRect(0, 28, 320, 52, BG);
      drawSongInfoTop();
      prev_lyric_idx = -2;
      prev_progress_sec = -1;

      if (prev_knob_x >= 0) {
        tft1.fillCircle(prev_knob_x, 193, 4, BG);
        tft1.fillRoundRect(12, 192, 296, 3, 1, DGRAY);
      }
      prev_knob_x = -1;
    }

    drawControls();
  }

  updateLyricsSpotifyKaraoke3Line();
  updateEQ();
  updateProgressUI();

  digitalWrite(TFT1_CS, HIGH);
}

void runLoadingScreen() {
  digitalWrite(TFT2_CS, HIGH);
  digitalWrite(TFT1_CS, LOW);

  tft1.fillScreen(BG);
  drawSpotifyLogo(tft1, 160, 78, 30, BG);

  tft1.setTextSize(2);
  tft1.setTextColor(WHITE, BG);
  tft1.setCursor(118, 122);
  tft1.print("SPOTIFY");

  tft1.setTextSize(1);
  tft1.setTextColor(SPGREEN, BG);
  tft1.setCursor(110, 148);
  tft1.print("Connecting WiFi..");

  tft1.drawRoundRect(100, 168, 120, 6, 2, GRAY);
  digitalWrite(TFT1_CS, HIGH);

  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, LOW);

  tft2.fillScreen(ST77XX_BLACK);
  drawSpotifyLogo(tft2, 80, 36, 14, ST77XX_BLACK);

  tft2.setTextSize(1);
  tft2.setTextColor(WHITE, ST77XX_BLACK);
  tft2.setCursor(59, 60);
  tft2.print("SPOTIFY");

  tft2.setTextSize(1);
  tft2.setTextColor(SPGREEN, ST77XX_BLACK);
  tft2.setCursor(34, 78);
  tft2.print("Connecting WiFi..");

  tft2.drawRoundRect(35, 94, 90, 6, 2, GRAY);
  digitalWrite(TFT2_CS, HIGH);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 60) {
    int filled1 = (attempts * 118) / 60;
    int filled2 = (attempts * 88) / 60;

    digitalWrite(TFT2_CS, HIGH);
    digitalWrite(TFT1_CS, LOW);
    tft1.fillRoundRect(101, 169, filled1, 4, 1, SPGREEN);
    digitalWrite(TFT1_CS, HIGH);

    digitalWrite(TFT1_CS, HIGH);
    digitalWrite(TFT2_CS, LOW);
    tft2.fillRoundRect(36, 95, filled2, 4, 1, SPGREEN);
    digitalWrite(TFT2_CS, HIGH);

    delay(200);
    attempts++;
  }
}
