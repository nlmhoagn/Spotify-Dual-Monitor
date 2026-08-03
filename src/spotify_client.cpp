#include "spotify_client.h"
#include "lyrics_manager.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// -------------------------------------------------------------
// REFRESH SPOTIFY TOKEN (BACKGROUND TASK ON CORE 0)
// -------------------------------------------------------------
bool refreshSpotifyToken() {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, "https://accounts.spotify.com/api/token");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String httpRequestData = "grant_type=refresh_token&refresh_token=" + String(spotify_refresh_token) +
                           "&client_id=" + String(spotify_client_id) +
                           "&client_secret=" + String(spotify_client_secret);

  int httpResponseCode = http.POST(httpRequestData);

  if (httpResponseCode == 200) {
    String response = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);

    access_token = doc["access_token"].as<String>();
    int expires_in = doc["expires_in"].as<int>();
    token_expires_at = millis() + (expires_in - 60) * 1000UL;

    Serial.println("[SPOTIFY] Token refreshed!");
    http.end();
    client.stop();
    return true;
  } else {
    Serial.printf("[SPOTIFY] Token refresh failed: %d\n", httpResponseCode);
    http.end();
    client.stop();
    return false;
  }
}

// -------------------------------------------------------------
// DOWNLOAD ALBUM COVER TO RAM (HTTPS WIFICLIENTSECURE)
// -------------------------------------------------------------
void downloadCoverImageToRAM(String imageUrl) {
  if (imageUrl.length() == 0 || WiFi.status() != WL_CONNECTED) {
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    rawCoverBuffer.clear();
    newCoverAvailable = true;
    xSemaphoreGive(dataMutex);
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(4000);
  http.begin(client, imageUrl);
  int httpCode = http.GET();

  if (httpCode == 200) {
    WiFiClient *stream = http.getStreamPtr();
    std::vector<uint8_t> tempBuffer;
    tempBuffer.reserve(25000);

    unsigned long startTime = millis();
    while (http.connected() && (millis() - startTime < 4000)) {
      size_t avail = stream->available();
      if (avail > 0) {
        size_t oldSize = tempBuffer.size();
        tempBuffer.resize(oldSize + avail);
        stream->readBytes((char*)&tempBuffer[oldSize], avail);
      } else {
        vTaskDelay(5 / portTICK_PERIOD_MS);
      }
    }

    http.end();
    client.stop();

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    rawCoverBuffer = std::move(tempBuffer);
    newCoverAvailable = true;
    xSemaphoreGive(dataMutex);
    Serial.printf("[COVER] Downloaded Album Cover HTTPS (%d bytes)!\n", rawCoverBuffer.size());
  } else {
    http.end();
    client.stop();

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    rawCoverBuffer.clear();
    newCoverAvailable = true;
    xSemaphoreGive(dataMutex);
    Serial.printf("[COVER] Download failed HTTP %d, rendering fallback UI\n", httpCode);
  }
}

// -------------------------------------------------------------
// FETCH SPOTIFY CURRENTLY PLAYING STATE (CORE 0)
// -------------------------------------------------------------
bool getCurrentlyPlaying() {
  if (WiFi.status() != WL_CONNECTED) return false;

  if (access_token == "" || millis() >= token_expires_at) {
    if (!refreshSpotifyToken()) return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(4000);
  unsigned long reqStart = millis();
  http.begin(client, "https://api.spotify.com/v1/me/player/currently-playing");
  http.addHeader("Authorization", "Bearer " + access_token);

  int httpCode = http.GET();
  unsigned long rtt = millis() - reqStart;

  if (httpCode == 200) {
    String payload = http.getString();

    StaticJsonDocument<256> filter;
    filter["is_playing"] = true;
    filter["progress_ms"] = true;
    filter["item"]["id"] = true;
    filter["item"]["name"] = true;
    filter["item"]["duration_ms"] = true;
    filter["item"]["artists"][0]["name"] = true;
    filter["item"]["album"]["images"] = true;

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

    if (err) {
      http.end();
      client.stop();
      return false;
    }

    bool new_is_playing = doc["is_playing"].as<bool>();
    long new_progress_ms = doc["progress_ms"].as<long>();

    JsonObject item = doc["item"];
    if (item.isNull() || !item["id"].is<String>()) {
      http.end();
      client.stop();
      return true;
    }

    long new_duration_ms = item["duration_ms"].as<long>();
    String new_track_id = item["id"].as<String>();
    String new_track_name = item["name"].as<String>();
    String new_artist_name = item["artists"][0]["name"].as<String>();

    String new_cover_url = "";
    if (item["album"]["images"].is<JsonArray>()) {
      JsonArray imgs = item["album"]["images"].as<JsonArray>();
      if (imgs.size() > 1 && !imgs[1]["url"].isNull()) {
        new_cover_url = imgs[1]["url"].as<String>();
      } else if (imgs.size() > 0 && !imgs[0]["url"].isNull()) {
        new_cover_url = imgs[0]["url"].as<String>();
      }
    }

    if (new_is_playing) {
      new_progress_ms += (rtt / 2);
      if (new_progress_ms > new_duration_ms && new_duration_ms > 0) {
        new_progress_ms = new_duration_ms;
      }
    }

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    is_playing = new_is_playing;
    progress_ms = new_progress_ms;
    duration_ms = new_duration_ms;
    last_progress_tick = millis();

    if (new_track_id.length() > 0 && new_track_id != current_track_id) {
      current_track_id = new_track_id;
      current_track_name = new_track_name;
      current_artist_name = new_artist_name;
      current_cover_url = new_cover_url;
      cover_loaded_tft2 = false;
      tft2_title_dirty = true;

      lyrics_available = false;
      lyrics_count = 0;
      current_lyric_idx = -1;

      pending_track_change = true;
    }
    xSemaphoreGive(dataMutex);

    http.end();
    client.stop();

    return true;
  } else if (httpCode == 204) {
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    is_playing = false;
    xSemaphoreGive(dataMutex);
    http.end();
    client.stop();
    return true;
  } else if (httpCode == 401) {
    Serial.println("[SPOTIFY] Token 401 Unauthorized -> Forcing token refresh!");
    token_expires_at = 0;
    http.end();
    client.stop();
    return false;
  }

  http.end();
  client.stop();
  return false;
}

// -------------------------------------------------------------
// BACKGROUND NETWORK TASK ON CORE 0 (SPOTIFY API & LYRICS)
// -------------------------------------------------------------
void spotifyNetworkTask(void *pvParameters) {
  for (;;) {
    String pending_cover_url = "";
    String pending_track_name = "";
    String pending_artist_name = "";
    long pending_duration_ms = 0;
    bool need_fetch = false;

    getCurrentlyPlaying();

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    if (pending_track_change) {
      pending_track_change = false;
      pending_cover_url = current_cover_url;
      pending_track_name = current_track_name;
      pending_artist_name = current_artist_name;
      pending_duration_ms = duration_ms;
      need_fetch = true;
    }
    xSemaphoreGive(dataMutex);

    if (need_fetch) {
      vTaskDelay(100 / portTICK_PERIOD_MS);
      downloadCoverImageToRAM(pending_cover_url);

      vTaskDelay(100 / portTICK_PERIOD_MS);
      fetchLyrics(pending_track_name, pending_artist_name, pending_duration_ms);
    }

    vTaskDelay(1500 / portTICK_PERIOD_MS);
  }
}
