#ifndef SPOTIFY_CLIENT_H
#define SPOTIFY_CLIENT_H

#include <Arduino.h>
#include "globals.h"

// OAuth2 Token Refresh
bool refreshSpotifyToken();

// Download Album Cover Image via HTTPS
void downloadCoverImageToRAM(String imageUrl);

// Spotify Player Currently Playing API
bool getCurrentlyPlaying();

// Spotify Player Control API (Pause, Play, Next, Prev)
bool sendSpotifyPlayerCommand(PlayerCommand cmd);

// Background Network Task on Core 0
void spotifyNetworkTask(void *pvParameters);

#endif // SPOTIFY_CLIENT_H
