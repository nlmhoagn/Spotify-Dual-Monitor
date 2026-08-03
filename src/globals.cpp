#include "globals.h"

// -------------------------------------------------------------
// WI-FI & SPOTIFY CREDENTIALS
// -------------------------------------------------------------
const char* ssid                  = "YOUR_WIFI_SSID";
const char* password              = "YOUR_WIFI_PASSWORD";

const char* spotify_client_id     = "YOUR_SPOTIFY_CLIENT_ID";
const char* spotify_client_secret = "YOUR_SPOTIFY_CLIENT_SECRET";
const char* spotify_refresh_token = "YOUR_SPOTIFY_REFRESH_TOKEN";

// Dual-Core Mutex
SemaphoreHandle_t dataMutex = NULL;

// TFT Display Objects
Adafruit_ILI9341 tft1 = Adafruit_ILI9341(&SPI, TFT1_DC, TFT1_CS, TFT1_RST);
Adafruit_ST7735  tft2 = Adafruit_ST7735(&SPI, TFT2_CS, TFT2_DC, TFT2_RST);

// Spotify & Track State
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

// Album cover download buffer
std::vector<uint8_t> rawCoverBuffer;
bool newCoverAvailable = false;
bool cover_loaded_tft2 = false;
bool tft2_title_dirty = true;
bool pending_track_change = false;

// Synced Lyrics
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
