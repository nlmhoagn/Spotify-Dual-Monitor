#ifndef LYRICS_MANAGER_H
#define LYRICS_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "globals.h"

// URL Encoder
String urlEncode(const String& str);

// Động cơ chuyển đổi Tiếng Việt bỏ dấu & Âm tiết Hàn Quốc (Hangul) sang Romaja Latin
String removeVietnameseAccents(String str);

// Lọc tiêu đề bài hát (bỏ phụ đề)
String cleanTrackNameForLyrics(String title);

// Phân tách câu dài thành nhiều đoạn ngắn vừa màn hình
std::vector<String> splitLyricLineToParts(const String& input, int maxChars = 25);

// Phân tích file LRC
void parseLRC(String lrcContent);

// Tải Lyrics qua LRCLIB API 6 tầng
bool fetchSingleLyrics(String url);
bool fetchSearchLyrics(String url);
void fetchLyrics(String track, String artist, long duration);

// Render Lyrics 3 dòng mượt mà trên Màn Lớn ILI9341
void updateLyricsSpotifyKaraoke3Line();

#endif // LYRICS_MANAGER_H
