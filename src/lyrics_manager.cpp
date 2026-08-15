#include "lyrics_manager.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <algorithm>

static LyricLine tempLyricsBuffer[MAX_LYRICS];

// -------------------------------------------------------------
// URL ENCODER FOR HTTP GET
// -------------------------------------------------------------
String urlEncode(const String& str) {
  String encoded = "";
  char code1;
  char code2;
  for (size_t i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else if (c == ' ') {
      encoded += "%20";
    } else {
      code1 = (c & 0xf0) >> 4;
      code2 = (c & 0x0f);
      encoded += '%';
      encoded += (char)(code1 < 10 ? code1 + '0' : code1 - 10 + 'A');
      encoded += (char)(code2 < 10 ? code2 + '0' : code2 - 10 + 'A');
    }
  }
  return encoded;
}

// -------------------------------------------------------------
// VIETNAMESE DIACRITICS REMOVAL & K-POP HANGUL TO ROMAJA LATIN ENGINE
// -------------------------------------------------------------
String removeVietnameseAccents(String str) {
  static const char* INITIALS[19] = {
    "g", "kk", "n", "d", "tt", "r", "m", "b", "pp", "s", "ss", "", "j", "jj", "ch", "k", "t", "p", "h"
  };
  static const char* MEDIALS[21] = {
    "a", "ae", "ya", "yae", "eo", "e", "yeo", "ye", "o", "wa", "wae", "oe", "yo", "u", "wo", "we", "wi", "yu", "eu", "ui", "i"
  };
  static const char* FINALS[28] = {
    "", "g", "kk", "gs", "n", "nj", "nh", "d", "l", "lg", "lm", "lb", "ls", "lt", "lp", "lh", "m", "b", "bs", "s", "ss", "ng", "j", "ch", "k", "t", "p", "h"
  };

  String out = "";
  size_t len = str.length();
  size_t i = 0;
  while (i < len) {
    unsigned char c1 = (unsigned char)str[i];
    if (c1 < 128) {
      out += (char)c1;
      i++;
    } else if ((c1 >= 0xEA && c1 <= 0xED) && i + 2 < len) {
      // Hangul Syllables U+AC00 .. U+D7A3 -> Romaja Latin
      unsigned char c2 = (unsigned char)str[i + 1];
      unsigned char c3 = (unsigned char)str[i + 2];
      uint32_t codePoint = ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);

      if (codePoint >= 0xAC00 && codePoint <= 0xD7A3) {
        uint32_t S = codePoint - 0xAC00;
        uint32_t I = S / 588;
        uint32_t M = (S % 588) / 28;
        uint32_t F = S % 28;

        out += INITIALS[I];
        out += MEDIALS[M];
        out += FINALS[F];
        i += 3;
      } else {
        out += ' ';
        i += 3;
      }
    } else if (c1 == 0xC3 && i + 1 < len) {
      unsigned char c2 = (unsigned char)str[i + 1];
      switch (c2) {
        case 0xA0: case 0xA1: case 0xA2: case 0xA3: out += 'a'; break;
        case 0x80: case 0x81: case 0x82: case 0x83: out += 'A'; break;
        case 0xA8: case 0xA9: case 0xAA: out += 'e'; break;
        case 0x88: case 0x89: case 0x8A: out += 'E'; break;
        case 0xAC: case 0xAD: out += 'i'; break;
        case 0x8C: case 0x8D: out += 'I'; break;
        case 0xB2: case 0xB3: case 0xB4: case 0xB5: out += 'o'; break;
        case 0x92: case 0x93: case 0x94: case 0x95: out += 'O'; break;
        case 0xB9: case 0xBA: out += 'u'; break;
        case 0x99: case 0x9A: out += 'U'; break;
        case 0xBD: out += 'y'; break;
        case 0x9D: out += 'Y'; break;
        default: out += ' '; break;
      }
      i += 2;
    } else if (c1 == 0xC4 && i + 1 < len) {
      unsigned char c2 = (unsigned char)str[i + 1];
      if (c2 == 0x83) out += 'a';
      else if (c2 == 0x82) out += 'A';
      else if (c2 == 0xA9) out += 'i';
      else if (c2 == 0xA8) out += 'I';
      else if (c2 == 0x91) out += 'd';
      else if (c2 == 0x90) out += 'D';
      else out += ' ';
      i += 2;
    } else if (c1 == 0xC5 && i + 1 < len) {
      unsigned char c2 = (unsigned char)str[i + 1];
      if (c2 == 0xA9) out += 'u';
      else if (c2 == 0xA8) out += 'U';
      else out += ' ';
      i += 2;
    } else if (c1 == 0xC6 && i + 1 < len) {
      unsigned char c2 = (unsigned char)str[i + 1];
      if (c2 == 0xA1) out += 'o';
      else if (c2 == 0xA0) out += 'O';
      else if (c2 == 0xB0) out += 'u';
      else if (c2 == 0xAF) out += 'U';
      else out += ' ';
      i += 2;
    } else if (c1 == 0xE1 && i + 2 < len) {
      unsigned char c2 = (unsigned char)str[i + 1];
      unsigned char c3 = (unsigned char)str[i + 2];
      if (c2 == 0xBA) {
        switch (c3) {
          case 0xA3: case 0xA1: case 0xAF: case 0xB1: case 0xB3: case 0xB5: case 0xB7:
          case 0xA5: case 0xA7: case 0xA9: case 0xAB: case 0xAD:
            out += 'a'; break;
          case 0xA2: case 0xA0: case 0xAE: case 0xB0: case 0xB2: case 0xB4: case 0xB6:
          case 0xA4: case 0xA6: case 0xA8: case 0xAA: case 0xAC:
            out += 'A'; break;
          case 0xBB: case 0xBD: case 0xB9: case 0xBF:
            out += 'e'; break;
          case 0xBA: case 0xBC: case 0xB8: case 0xBE:
            out += 'E'; break;
          default: out += ' '; break;
        }
      } else if (c2 == 0xBB) {
        switch (c3) {
          case 0x81: case 0x83: case 0x85: case 0x87:
            out += 'e'; break;
          case 0x80: case 0x82: case 0x84: case 0x86:
            out += 'E'; break;
          case 0x89: case 0x8B:
            out += 'i'; break;
          case 0x88: case 0x8A:
            out += 'I'; break;
          case 0x8F: case 0x8D: case 0x91: case 0x93: case 0x95: case 0x97: case 0x99:
          case 0x9B: case 0x9D: case 0x9F: case 0xA1: case 0xA3:
            out += 'o'; break;
          case 0x8E: case 0x8C: case 0x90: case 0x92: case 0x94: case 0x96: case 0x98:
          case 0x9A: case 0x9C: case 0x9E: case 0xA0: case 0xA2:
            out += 'O'; break;
          case 0xA7: case 0xA5: case 0xA9: case 0xAB: case 0xAD: case 0xAF: case 0xB1:
            out += 'u'; break;
          case 0xA6: case 0xA4: case 0xA8: case 0xAA: case 0xAC: case 0xAE: case 0xB0:
            out += 'U'; break;
          case 0xB3: case 0xB7: case 0xB9: case 0xB5:
            out += 'y'; break;
          case 0xB2: case 0xB6: case 0xB8: case 0xB4:
            out += 'Y'; break;
          default: out += ' '; break;
        }
      } else {
        out += ' ';
      }
      i += 3;
    } else {
      out += ' ';
      i++;
    }
  }
  return out;
}

String cleanTrackNameForLyrics(String title) {
  int idxDash = title.indexOf(" - ");
  if (idxDash != -1) {
    title = title.substring(0, idxDash);
  }
  int idxParen = title.indexOf("(");
  if (idxParen != -1) {
    title = title.substring(0, idxParen);
  }
  title.trim();
  return title;
}

std::vector<String> splitLyricLineToParts(const String& input, int maxChars) {
  std::vector<String> parts;
  if ((int)input.length() <= maxChars) {
    parts.push_back(input);
    return parts;
  }
  int pos = 0;
  int len = input.length();
  while (pos < len) {
    int rem = len - pos;
    if (rem <= maxChars) {
      String sub = input.substring(pos);
      sub.trim();
      if (sub.length() > 0) parts.push_back(sub);
      break;
    }
    int splitIdx = -1;
    for (int i = pos + maxChars; i > pos + 4; i--) {
      if (input[i] == ' ') { splitIdx = i; break; }
    }
    if (splitIdx == -1) splitIdx = pos + maxChars;

    String sub = input.substring(pos, splitIdx);
    sub.trim();
    if (sub.length() > 0) parts.push_back(sub);
    pos = splitIdx;
    while (pos < len && input[pos] == ' ') pos++;
  }
  if (parts.empty()) parts.push_back(input);
  return parts;
}

void parseLRC(const char* lrcContent) {
  if (!lrcContent) return;
  int tempCount = 0;
  int len = (int)strlen(lrcContent);
  int pos = 0;

  while (pos < len && tempCount < MAX_LYRICS) {
    while (pos < len && lrcContent[pos] != '[') pos++;
    if (pos >= len) break;
    pos++;
    int timeStart = pos;
    while (pos < len && lrcContent[pos] != ']') pos++;
    if (pos >= len) break;
    int timeEnd = pos;
    pos++;

    int colon = -1;
    for (int j = timeStart; j < timeEnd; j++) {
      if (lrcContent[j] == ':') { colon = j; break; }
    }
    if (colon == -1) continue;

    int mins = 0;
    for (int j = timeStart; j < colon; j++) {
      if (lrcContent[j] >= '0' && lrcContent[j] <= '9') {
        mins = mins * 10 + (lrcContent[j] - '0');
      }
    }

    int secs = 0;
    int ms_part = 0;
    bool dot = false;
    int dot_digits = 0;
    for (int j = colon + 1; j < timeEnd; j++) {
      char ch = lrcContent[j];
      if (ch == '.' || ch == ',') { dot = true; continue; }
      if (ch < '0' || ch > '9') break;
      if (!dot) {
        secs = secs * 10 + (ch - '0');
      } else {
        if (dot_digits < 3) {
          ms_part = ms_part * 10 + (ch - '0');
          dot_digits++;
        }
      }
    }
    if (dot_digits == 1) ms_part *= 100;
    else if (dot_digits == 2) ms_part *= 10;

    long ms = (long)mins * 60000L + (long)secs * 1000L + (long)ms_part;

    int lineStart = pos;
    while (pos < len && lrcContent[pos] != '\n') pos++;
    int lineEnd = pos;
    pos++;
    while (lineEnd > lineStart &&
           (lrcContent[lineEnd-1] == '\r' || lrcContent[lineEnd-1] == ' ')) {
      lineEnd--;
    }

    char lineBuf[256];
    int ll = lineEnd - lineStart;
    if (ll > 255) ll = 255;
    if (ll > 0) {
      memcpy(lineBuf, lrcContent + lineStart, ll);
      lineBuf[ll] = '\0';
    } else {
      lineBuf[0] = '\0';
    }

    tempLyricsBuffer[tempCount].time_ms = ms;
    tempLyricsBuffer[tempCount].text = removeVietnameseAccents(String(lineBuf));
    tempCount++;
  }

  std::sort(tempLyricsBuffer, tempLyricsBuffer + tempCount, [](const LyricLine& a, const LyricLine& b) {
    return a.time_ms < b.time_ms;
  });

  xSemaphoreTake(dataMutex, portMAX_DELAY);
  lyrics_count = tempCount;
  for (int i = 0; i < tempCount; i++) lyrics[i] = tempLyricsBuffer[i];
  lyrics_available = (lyrics_count > 0);
  xSemaphoreGive(dataMutex);
  Serial.printf("[LYRICS] Parsed & sorted %d lines!\n", lyrics_count);
}

String extractSyncedLyrics(const String& payload, int startFromIndex = 0, int* nextIndex = nullptr) {
  if (nextIndex) *nextIndex = -1;
  int keyIdx = payload.indexOf("\"syncedLyrics\":", startFromIndex);
  if (keyIdx == -1) {
    keyIdx = payload.indexOf("\"syncedLyrics\" :", startFromIndex);
  }
  if (keyIdx == -1) return "";

  int valIdx = keyIdx + 15;
  while (valIdx < (int)payload.length() &&
        (payload[valIdx] == ' ' || payload[valIdx] == '\t' || payload[valIdx] == '\r' || payload[valIdx] == '\n')) {
    valIdx++;
  }

  if (valIdx >= (int)payload.length() || payload[valIdx] != '"') {
    if (nextIndex) *nextIndex = valIdx + 4;
    return "";
  }

  int startQuote = valIdx;
  String result = "";
  result.reserve(4096);

  size_t len = payload.length();
  size_t i = startQuote + 1;
  while (i < len) {
    char c = payload[i];
    if (c == '\\' && i + 1 < len) {
      char nextChar = payload[i + 1];
      if (nextChar == 'n') { result += '\n'; i += 2; }
      else if (nextChar == 'r') { result += '\r'; i += 2; }
      else if (nextChar == 't') { result += '\t'; i += 2; }
      else if (nextChar == '"') { result += '"'; i += 2; }
      else if (nextChar == '\\') { result += '\\'; i += 2; }
      else if (nextChar == '/') { result += '/'; i += 2; }
      else {
        result += c;
        i++;
      }
    } else if (c == '"') {
      if (nextIndex) *nextIndex = (int)i + 1;
      return result;
    } else {
      result += c;
      i++;
    }
  }

  if (nextIndex) *nextIndex = (int)len;
  return result;
}

bool fetchSingleLyrics(String url) {
  WiFiClientSecure c;
  c.setInsecure();
  HTTPClient h;
  h.setTimeout(4000);
  h.begin(c, url);
  h.addHeader("User-Agent", "ESP32-Spotify-Display/1.0");

  int code = h.GET();
  Serial.printf("[LRC-GET] %d url=%s\n", code, url.c_str());
  if (code != 200) {
    h.end();
    c.stop();
    return false;
  }

  String payload = h.getString();
  h.end();
  c.stop();

  if (payload.length() < 20) return false;

  String lrc = extractSyncedLyrics(payload);
  if (lrc.length() > 10) {
    parseLRC(lrc.c_str());
    return true;
  }

  Serial.println("[LRC-GET] No syncedLyrics in response object");
  return false;
}

bool fetchSearchLyrics(String url) {
  WiFiClientSecure c;
  c.setInsecure();
  HTTPClient h;
  h.setTimeout(4000);
  h.begin(c, url);
  h.addHeader("User-Agent", "ESP32-Spotify-Display/1.0");

  int code = h.GET();
  Serial.printf("[LRC-SRCH] %d url=%s\n", code, url.c_str());
  if (code != 200) {
    h.end();
    c.stop();
    return false;
  }

  String payload = h.getString();
  h.end();
  c.stop();

  if (payload.length() < 20) return false;

  int currIdx = 0;
  while (currIdx < (int)payload.length()) {
    int nextIdx = -1;
    String lrc = extractSyncedLyrics(payload, currIdx, &nextIdx);
    if (lrc.length() > 10) {
      Serial.println("[LRC-SRCH] Found syncedLyrics directly from search payload!");
      parseLRC(lrc.c_str());
      return true;
    }
    if (nextIdx == -1 || nextIdx <= currIdx) break;
    currIdx = nextIdx;
  }

  Serial.println("[LRC-SRCH] No syncedLyrics in search result array");
  return false;
}

void fetchLyrics(String track, String artist, long duration) {
  xSemaphoreTake(dataMutex, portMAX_DELAY);
  lyrics_count = 0;
  current_lyric_idx = -1;
  lyrics_available = false;
  String expectedTrackId = current_track_id;
  xSemaphoreGive(dataMutex);

  if (WiFi.status() != WL_CONNECTED) return;

  auto isAborted = [&]() -> bool {
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    bool aborted = (current_track_id != expectedTrackId);
    xSemaphoreGive(dataMutex);
    return aborted;
  };

  if (isAborted()) return;

  String cleanTrack = cleanTrackNameForLyrics(track);
  String cleanArtist = artist;
  cleanArtist.trim();

  String asciiTrack = removeVietnameseAccents(cleanTrack);
  String asciiArtist = removeVietnameseAccents(cleanArtist);

  Serial.println("[LYRICS] Fetching: " + cleanTrack + " / " + cleanArtist);

  vTaskDelay(20 / portTICK_PERIOD_MS);
  if (isAborted()) return;

  if (fetchSingleLyrics(
      "https://lrclib.net/api/get?track_name=" + urlEncode(cleanTrack) +
      "&artist_name=" + urlEncode(cleanArtist) +
      "&duration=" + String(duration / 1000))) return;

  vTaskDelay(20 / portTICK_PERIOD_MS);
  if (isAborted()) return;

  if (fetchSingleLyrics(
      "https://lrclib.net/api/get?track_name=" + urlEncode(cleanTrack) +
      "&artist_name=" + urlEncode(cleanArtist))) return;

  vTaskDelay(20 / portTICK_PERIOD_MS);
  if (isAborted()) return;

  if (fetchSingleLyrics(
      "https://lrclib.net/api/get?track_name=" + urlEncode(asciiTrack) +
      "&artist_name=" + urlEncode(asciiArtist))) return;

  vTaskDelay(20 / portTICK_PERIOD_MS);
  if (isAborted()) return;

  if (fetchSearchLyrics(
      "https://lrclib.net/api/search?track_name=" + urlEncode(cleanTrack) +
      "&artist_name=" + urlEncode(cleanArtist))) return;

  vTaskDelay(20 / portTICK_PERIOD_MS);
  if (isAborted()) return;

  if (fetchSearchLyrics(
      "https://lrclib.net/api/search?q=" + urlEncode(asciiTrack + " " + asciiArtist))) return;

  vTaskDelay(20 / portTICK_PERIOD_MS);
  if (isAborted()) return;

  if (fetchSearchLyrics(
      "https://lrclib.net/api/search?q=" + urlEncode(asciiTrack))) return;

  Serial.println("[LYRICS] No synced lyrics found after 6 tiers.");
}

void updateLyricsSpotifyKaraoke3Line() {
  xSemaphoreTake(dataMutex, portMAX_DELAY);
  bool lrc_avail = lyrics_available;
  int lrc_count = lyrics_count;
  long prog = progress_ms;
  bool playing = is_playing;
  xSemaphoreGive(dataMutex);

  int new_lyric_idx = -1;
  xSemaphoreTake(dataMutex, portMAX_DELAY);
  if (lrc_avail && lrc_count > 0) {
    for (int i = 0; i < lrc_count; i++) {
      if (prog >= lyrics[i].time_ms) {
        new_lyric_idx = i;
      } else {
        break;
      }
    }
  }
  xSemaphoreGive(dataMutex);

  static bool prev_lrc_avail = false;
  static int prev_lrc_count = -1;
  if (lrc_avail != prev_lrc_avail || lrc_count != prev_lrc_count) {
    prev_lrc_avail = lrc_avail;
    prev_lrc_count = lrc_count;
    tft1.setTextColor(LGRAY, DARK);
    tft1.setTextSize(1);
    tft1.setCursor(220, 12);
    if (lrc_avail) {
      char dbg[16]; sprintf(dbg, "LRC:%d", lrc_count);
      tft1.print(dbg);
    } else {
      tft1.print("LRC:---");
    }
  }

  if (lrc_avail && new_lyric_idx >= 0) {
    String currText = "";
    long line_start_ms = 0;
    long line_end_ms = 0;

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    currText = lyrics[new_lyric_idx].text;
    line_start_ms = lyrics[new_lyric_idx].time_ms;
    if (new_lyric_idx + 1 < lrc_count) {
      line_end_ms = lyrics[new_lyric_idx + 1].time_ms;
    } else {
      line_end_ms = line_start_ms + 4000;
    }
    xSemaphoreGive(dataMutex);

    std::vector<String> parts = splitLyricLineToParts(currText, 25);
    int partCount = parts.size();
    long line_dur = line_end_ms - line_start_ms;
    if (line_dur <= 0) line_dur = 4000;

    int current_part_idx = 0;
    if (partCount > 1) {
      current_part_idx = (prog - line_start_ms) * partCount / line_dur;
      if (current_part_idx < 0) current_part_idx = 0;
      if (current_part_idx >= partCount) current_part_idx = partCount - 1;
    }

    int current_state_id = new_lyric_idx * 100 + current_part_idx;

    if (current_state_id != prev_state_id) {
      prev_state_id = current_state_id;

      String pStr = "";
      if (current_part_idx > 0) {
        pStr = parts[current_part_idx - 1];
      } else if (new_lyric_idx > 0) {
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        String pText = lyrics[new_lyric_idx - 1].text;
        xSemaphoreGive(dataMutex);
        std::vector<String> prevParts = splitLyricLineToParts(pText, 25);
        pStr = prevParts.back();
      }

      String cStr = parts[current_part_idx];

      String nStr = "";
      if (current_part_idx + 1 < partCount) {
        nStr = parts[current_part_idx + 1];
      } else if (new_lyric_idx + 1 < lrc_count) {
        xSemaphoreTake(dataMutex, portMAX_DELAY);
        String nText = lyrics[new_lyric_idx + 1].text;
        xSemaphoreGive(dataMutex);
        std::vector<String> nextParts = splitLyricLineToParts(nText, 25);
        nStr = nextParts.front();
      }

      tft1.fillRect(0, 80, 320, 72, BG);

      int16_t x1, y1; uint16_t w, h;

      if (pStr.length() > 0) {
        if (pStr.length() > 48) pStr = pStr.substring(0, 46) + "..";
        tft1.setTextColor(LGRAY, BG);
        tft1.setTextSize(1);
        tft1.getTextBounds(pStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        int px = (320 - w) / 2; if (px < 4) px = 4;
        tft1.setCursor(px, 84);
        tft1.print(pStr);
      }

      tft1.setTextColor(WHITE, BG);
      tft1.setTextSize(2);
      tft1.getTextBounds(cStr.c_str(), 0, 0, &x1, &y1, &w, &h);
      int cx = (320 - w) / 2; if (cx < 4) cx = 4;
      tft1.setCursor(cx, 104);
      tft1.print(cStr);

      if (nStr.length() > 0) {
        if (nStr.length() > 48) nStr = nStr.substring(0, 46) + "..";
        tft1.setTextColor(LGRAY, BG);
        tft1.setTextSize(1);
        tft1.getTextBounds(nStr.c_str(), 0, 0, &x1, &y1, &w, &h);
        int nx = (320 - w) / 2; if (nx < 4) nx = 4;
        tft1.setCursor(nx, 130);
        tft1.print(nStr);
      }
    }
  } else if (!lrc_avail) {
    if (prev_state_id != -1) {
      prev_state_id = -1;
      tft1.fillRect(0, 80, 320, 72, BG);
      String noLrcText = playing ? "Playing Music (No Lyrics)" : "Paused";
      tft1.setTextColor(LGRAY, BG);
      tft1.setTextSize(2);
      int16_t x1, y1; uint16_t w, h;
      tft1.getTextBounds(noLrcText.c_str(), 0, 0, &x1, &y1, &w, &h);
      int nx = (320 - w) / 2; if (nx < 4) nx = 4;
      tft1.setCursor(nx, 106);
      tft1.print(noLrcText);
    }
  }
}
