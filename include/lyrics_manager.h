#ifndef LYRICS_MANAGER_H
#define LYRICS_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "globals.h"

// URL Encoder
String urlEncode(const String& str);

// Vietnamese diacritics removal & K-Pop Hangul to Romaja Latin converter
String removeVietnameseAccents(String str);

// Clean track title (strip subtitles/extra tags)
String cleanTrackNameForLyrics(String title);

// Split long lyric line into shorter parts to fit screen
std::vector<String> splitLyricLineToParts(const String& input, int maxChars = 25);

// Parse LRC file
void parseLRC(String lrcContent);

// Fetch lyrics via 6-tier LRCLIB API
bool fetchSingleLyrics(String url);
bool fetchSearchLyrics(String url);
void fetchLyrics(String track, String artist, long duration);

// Smooth 3-line Karaoke Lyrics renderer on ILI9341 Main Display
void updateLyricsSpotifyKaraoke3Line();

#endif // LYRICS_MANAGER_H
