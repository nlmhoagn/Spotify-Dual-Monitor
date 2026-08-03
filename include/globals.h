#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_ST7735.h>
#include <vector>
#include "config.h"

// Synced Lyrics structure
struct LyricLine {
  long time_ms;
  String text;
};

// Dual-Core Mutex declaration
extern SemaphoreHandle_t dataMutex;

// TFT Display Objects
extern Adafruit_ILI9341 tft1;
extern Adafruit_ST7735  tft2;

// Spotify & Track State Variables
extern String access_token;
extern unsigned long token_expires_at;

extern String current_track_id;
extern String current_track_name;
extern String current_artist_name;
extern String current_cover_url;
extern long progress_ms;
extern long duration_ms;
extern bool is_playing;
extern unsigned long last_progress_tick;

// Album cover download buffer
extern std::vector<uint8_t> rawCoverBuffer;
extern bool newCoverAvailable;
extern bool cover_loaded_tft2;
extern bool tft2_title_dirty;
extern bool pending_track_change;

// Lyrics state
extern LyricLine lyrics[MAX_LYRICS];
extern int lyrics_count;
extern int current_lyric_idx;
extern bool lyrics_available;

// EQ visualizer state
extern int eqH[10];
extern int eqT[10];
extern unsigned long last_eq_update;

// Redraw state tracking
extern String prev_title;
extern int prev_lyric_idx;
extern int prev_state_id;
extern long prev_progress_sec;
extern int prev_knob_x;

#endif // GLOBALS_H
