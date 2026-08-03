#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <TJpg_Decoder.h>

#include "config.h"
#include "globals.h"
#include "display_manager.h"
#include "lyrics_manager.h"
#include "spotify_client.h"
#include "ui_renderer.h"

// -------------------------------------------------------------
// KHỞI TẠO HỆ THỐNG (CORE 1)
// -------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  dataMutex = xSemaphoreCreateMutex();

  pinMode(TFT1_CS, OUTPUT);
  pinMode(TFT2_CS, OUTPUT);
  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, HIGH);

  SPI.begin(SPI_SCLK, -1, SPI_MOSI, -1);

  tft1.begin(27000000);
  tft1.setRotation(1);

  tft2.initR(INITR_BLACKTAB);
  tft2.setRotation(1);

  TJpgDec.setJpgScale(4);
  TJpgDec.setSwapBytes(false);

  // Khởi tạo trạng thái sóng nhạc EQ
  for (int i = 0; i < 10; i++) {
    eqH[i] = random(4, 18);
    eqT[i] = random(4, 18);
  }

  WiFi.begin(ssid, password);
  runLoadingScreen();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Connected!");
    refreshSpotifyToken();

    // Khởi tạo Task mạng Spotify chạy nền trên Core 0
    xTaskCreatePinnedToCore(
      spotifyNetworkTask,
      "SpotifyTask",
      36864,
      NULL,
      1,
      NULL,
      0
    );
  }
}

// -------------------------------------------------------------
// VÒNG LẶP CHÍNH (CORE 1 RENDER CẢ 2 MÀN HÌNH MƯỢT MÀ)
// -------------------------------------------------------------
void loop() {
  unsigned long now = millis();

  // 1. Nội suy thời gian phát nhạc mượt mà trên Core 1
  xSemaphoreTake(dataMutex, portMAX_DELAY);
  if (is_playing) {
    unsigned long elapsed = now - last_progress_tick;
    progress_ms += elapsed;
    last_progress_tick = now;
    if (progress_ms > duration_ms && duration_ms > 0) {
      progress_ms = duration_ms;
    }
  }
  xSemaphoreGive(dataMutex);

  // 2. Render đĩa vinyl & metadata lên Màn Nhỏ ST7735
  renderAlbumCoverOnCore1();
  updateScreen2Dynamic();

  // 3. Render giao diện Spotify & Lyrics Karaoke lên Màn Lớn ILI9341
  updateScreen1();

  delay(15);
}