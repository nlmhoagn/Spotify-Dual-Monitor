#include "globals.h"

// -------------------------------------------------------------
// WI-FI & SPOTIFY CREDENTIALS (DYNAMIC / LOADED FROM NVS)
// -------------------------------------------------------------
String ssid                  = "YOUR_WIFI_SSID";
String password              = "YOUR_WIFI_PASSWORD";

String spotify_client_id     = "YOUR_SPOTIFY_CLIENT_ID";
String spotify_client_secret = "YOUR_SPOTIFY_CLIENT_SECRET";
String spotify_refresh_token = "YOUR_SPOTIFY_REFRESH_TOKEN";

// System state & UI Settings
bool in_config_mode = false;
uint16_t theme_accent_color = SPGREEN; // Default Spotify Green (0x06C4)
uint8_t vinyl_render_mode   = 0;       // 0 = Spinning Vinyl, 1 = Full Square Cover
uint8_t eq_enabled          = 1;       // 1 = Enabled, 0 = Disabled

// Dual-Core Mutex
SemaphoreHandle_t dataMutex = NULL;

// Pending Spotify command
PlayerCommand pending_player_cmd = CMD_NONE;

// TFT Display Objects (RST is managed manually via GPIO 10 to prevent shared reset conflicts)
Adafruit_ILI9341 tft1 = Adafruit_ILI9341(&SPI, TFT1_DC, TFT1_CS, -1);
Adafruit_ST7735  tft2 = Adafruit_ST7735(&SPI, TFT2_CS, TFT2_DC, -1);

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
