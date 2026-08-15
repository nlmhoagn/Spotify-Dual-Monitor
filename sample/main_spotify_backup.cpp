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
// SYSTEM INITIALIZATION (CORE 1 - SPOTIFY DUAL MONITOR)
// -------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n==========================================");
  Serial.println("   ESP32-S3 SPOTIFY DUAL MONITOR START    ");
  Serial.println("==========================================");

  dataMutex = xSemaphoreCreateMutex();

  // 1. Configure CS & Control Pins
  pinMode(TFT1_CS, OUTPUT);
  pinMode(TFT2_CS, OUTPUT);
  pinMode(TFT1_DC, OUTPUT);
  pinMode(TFT1_RST, OUTPUT);

  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, HIGH);

  // 2. Hardware Reset pulse for both displays on shared RST (GPIO 10)
  digitalWrite(TFT1_RST, LOW);
  delay(100);
  digitalWrite(TFT1_RST, HIGH);
  delay(200);

  // 3. Initialize SPI bus at 10 MHz
  SPI.begin(SPI_SCLK, -1, SPI_MOSI, -1);

  // 4. Initialize ST7735 (Display 2 - Album Cover / Vinyl) FIRST
  digitalWrite(TFT1_CS, HIGH);
  digitalWrite(TFT2_CS, LOW);
  tft2.initR(INITR_BLACKTAB);
  tft2.setRotation(1);
  digitalWrite(TFT2_CS, HIGH);

  // 5. Initialize ILI9341 (Display 1 - Spotify UI / Lyrics) SECOND
  digitalWrite(TFT2_CS, HIGH);
  digitalWrite(TFT1_CS, LOW);
  tft1.begin(10000000);
  tft1.setRotation(1);
  digitalWrite(TFT1_CS, HIGH);

  TJpgDec.setJpgScale(4);
  TJpgDec.setSwapBytes(false);

  // Initialize EQ visualizer state
  for (int i = 0; i < 10; i++) {
    eqH[i] = random(4, 18);
    eqT[i] = random(4, 18);
  }

  WiFi.begin(ssid, password);
  runLoadingScreen();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Connected!");
    refreshSpotifyToken();

    // Initialize background Spotify network task on Core 0
    xTaskCreatePinnedToCore(
      spotifyNetworkTask,
      "SpotifyTask",
      36864,
      NULL,
      1,
      NULL,
      0
    );
  } else {
    Serial.println("\n[WIFI] Not connected. Running offline/waiting UI...");
  }
}

// -------------------------------------------------------------
// MAIN LOOP (CORE 1 RENDERS BOTH DISPLAYS SMOOTHLY)
// -------------------------------------------------------------
void loop() {
  unsigned long now = millis();

  // 1. Smooth playback time interpolation on Core 1
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

  // 2. Render vinyl disc & metadata on ST7735 display
  renderAlbumCoverOnCore1();
  updateScreen2Dynamic();

  // 3. Render Spotify UI & Karaoke Lyrics on ILI9341 display
  updateScreen1();

  delay(15);
}
