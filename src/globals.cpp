#include "globals.h"

// -------------------------------------------------------------
// THÔNG TIN WI-FI & SPOTIFY CREDENTIALS
// -------------------------------------------------------------
const char* ssid                  = "HnP";
const char* password              = "20072013";

const char* spotify_client_id     = "39ba1e8f7b5d47888015e95e755cde67";
const char* spotify_client_secret = "6de5795fb49b44fa9555166f5bf6067f";
const char* spotify_refresh_token = "AQA8CP94ImUVZKJLrcbgLvj9ylN_s-6Ln6xqmVQDfhv1KtS-O3GPaZY7YSOy-EISFt8ZLYloo_adyCEJkqKGwHp1CNBgNFkwom1FjeUrjE9RRy8aePhbBH6brtfvE7ivMfs";

// Mutex Dual-Core
SemaphoreHandle_t dataMutex = NULL;

// Đối tượng màn hình TFT
Adafruit_ILI9341 tft1 = Adafruit_ILI9341(&SPI, TFT1_DC, TFT1_CS, TFT1_RST);
Adafruit_ST7735  tft2 = Adafruit_ST7735(&SPI, TFT2_CS, TFT2_DC, TFT2_RST);

// Trạng thái Spotify & Track
String access_token = "";
unsigned long token_expires_at = 0;

String current_track_id = "";
String current_track_name = "";
String current_artist_name = "";
String current_cover_url = "";
long progress_ms = 0;
long duration_ms = 0;
bool is_playing = false;
unsigned long last_progress_tick = 0;

// Buffer tải ảnh bìa Album
std::vector<uint8_t> rawCoverBuffer;
bool newCoverAvailable = false;
bool cover_loaded_tft2 = false;
bool tft2_title_dirty = true;
bool pending_track_change = false;

// Lời bài hát
LyricLine lyrics[MAX_LYRICS];
int lyrics_count = 0;
int current_lyric_idx = -1;
bool lyrics_available = false;

// EQ visualizer state
int eqH[10];
int eqT[10];
unsigned long last_eq_update = 0;

// Redraw state tracking
String prev_title = "";
int prev_lyric_idx = -2;
int prev_state_id = -2;
long prev_progress_sec = -1;
int prev_knob_x = -1;
