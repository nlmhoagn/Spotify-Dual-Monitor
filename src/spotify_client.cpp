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
    filter["item"]["artists"] = true;
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

    String new_artist_name = "";
    if (item["artists"].is<JsonArray>()) {
      JsonArray artistsArr = item["artists"].as<JsonArray>();
      for (size_t i = 0; i < artistsArr.size(); i++) {
        if (i > 0) new_artist_name += ", ";
        new_artist_name += artistsArr[i]["name"].as<String>();
      }
    } else if (!item["artists"][0]["name"].isNull()) {
      new_artist_name = item["artists"][0]["name"].as<String>();
    }

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
// SPOTIFY PLAYER CONTROL API (PAUSE, PLAY, SKIP NEXT, SKIP PREV)
// -------------------------------------------------------------
bool sendSpotifyPlayerCommand(PlayerCommand cmd) {
  if (cmd == CMD_NONE || WiFi.status() != WL_CONNECTED) return false;

  if (access_token == "" || millis() >= token_expires_at) {
    if (!refreshSpotifyToken()) return false;
  }

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(4000);

  String url = "";
  String method = "";

  switch (cmd) {
    case CMD_PAUSE:
      url = "https://api.spotify.com/v1/me/player/pause";
      method = "PUT";
      break;

    case CMD_PLAY:
      url = "https://api.spotify.com/v1/me/player/play";
      method = "PUT";
      break;

    case CMD_PLAY_PAUSE:
      xSemaphoreTake(dataMutex, portMAX_DELAY);
      if (is_playing) {
        url = "https://api.spotify.com/v1/me/player/pause";
        method = "PUT";
      } else {
        url = "https://api.spotify.com/v1/me/player/play";
        method = "PUT";
      }
      xSemaphoreGive(dataMutex);
      break;

    case CMD_NEXT:
      url = "https://api.spotify.com/v1/me/player/next";
      method = "POST";
      break;

    case CMD_PREV:
      url = "https://api.spotify.com/v1/me/player/previous";
      method = "POST";
      break;

    default:
      return false;
  }

  for (int attempt = 0; attempt < 2; attempt++) {
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setTimeout(4000);

    http.begin(client, url);
    http.addHeader("Authorization", "Bearer " + access_token);
    http.addHeader("Content-Length", "0");

    int httpCode = http.sendRequest(method.c_str(), (uint8_t*)NULL, 0);

    if (httpCode == 204 || httpCode == 200) {
      Serial.printf("[SPOTIFY CMD] %s success (%d)\n", method.c_str(), httpCode);
      http.end();
      client.stop();
      return true;
    } else if (httpCode == 401) {
      http.end();
      client.stop();

      if (attempt == 0) {
        Serial.println("[SPOTIFY CMD] 401 Unauthorized -> Refreshing token and retrying once...");
        refreshSpotifyToken();
        continue;
      } else {
        Serial.println("\n==========================================================================");
        Serial.println("[SPOTIFY ERROR] 401 UNAUTHORIZED! REFRESH TOKEN LACKS SCOPE PERMISSION.");
        Serial.println("Your current refresh token only has READ scopes (user-read-currently-playing).");
        Serial.println("To fix: Generate a new Spotify refresh token with scopes:");
        Serial.println("  -> user-read-currently-playing user-read-playback-state user-modify-playback-state");
        Serial.println("Then update 'spotify_refresh_token' in globals.cpp.");
        Serial.println("==========================================================================\n");
        return false;
      }
    } else {
      String res = http.getString();
      if (httpCode == 403) {
        Serial.printf("[SPOTIFY CMD] 403 Forbidden! Missing 'user-modify-playback-state' scope. Res: %s\n", res.c_str());
      } else if (httpCode == 404) {
        Serial.printf("[SPOTIFY CMD] 404 Not Found! No active device found. Open Spotify on phone/PC. Res: %s\n", res.c_str());
      } else {
        Serial.printf("[SPOTIFY CMD] Failed HTTP %d: %s\n", httpCode, res.c_str());
      }
      http.end();
      client.stop();
      return false;
    }
  }

  return false;
}

// -------------------------------------------------------------
// BACKGROUND NETWORK TASK ON CORE 0 (SPOTIFY API & LYRICS)
// -------------------------------------------------------------
void spotifyNetworkTask(void *pvParameters) {
  unsigned long last_poll_time = 0;

  for (;;) {
    // 1. Check for pending button command immediately (checked every 20ms!)
    PlayerCommand cmd_to_exec = CMD_NONE;
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    if (pending_player_cmd != CMD_NONE) {
      cmd_to_exec = pending_player_cmd;
      pending_player_cmd = CMD_NONE;
    }
    xSemaphoreGive(dataMutex);

    if (cmd_to_exec != CMD_NONE) {
      // Execute button command IMMEDIATELY with zero delay!
      sendSpotifyPlayerCommand(cmd_to_exec);
      vTaskDelay(50 / portTICK_PERIOD_MS);
      getCurrentlyPlaying();
      last_poll_time = millis();
    } else {
      // Periodic playback status poll every 2000ms
      if (last_poll_time == 0 || (millis() - last_poll_time >= 2000)) {
        getCurrentlyPlaying();
        last_poll_time = millis();
      }
    }

    String pending_cover_url = "";
    String pending_track_name = "";
    String pending_artist_name = "";
    long pending_duration_ms = 0;
    bool need_fetch = false;

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
      vTaskDelay(50 / portTICK_PERIOD_MS);
      downloadCoverImageToRAM(pending_cover_url);

      vTaskDelay(50 / portTICK_PERIOD_MS);
      fetchLyrics(pending_track_name, pending_artist_name, pending_duration_ms);
    }

    // Short sleep of 20ms so button press response is ultra-fast (< 20ms)
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}
