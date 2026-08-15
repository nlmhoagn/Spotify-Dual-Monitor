#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>

// Load WiFi & Spotify credentials from NVS (Preferences)
// Returns true if valid credentials exist in NVS, false otherwise.
bool loadCredentialsFromNVS();

// Save new credentials to NVS
void saveCredentialsToNVS(const String& new_ssid, const String& new_password,
                          const String& new_client_id, const String& new_client_secret,
                          const String& new_refresh_token);

// Clear NVS saved credentials
void clearCredentialsNVS();

// Start Access Point, Captive Portal DNS Server, and WebServer for WiFi / Spotify Setup
void startConfigPortal();

// Handle web server requests and DNS requests in loop() while in config mode
void handleConfigPortal();

// Start Web Server & mDNS when connected to Home Wi-Fi (STA mode)
void startWebDashboard();

// Handle web server requests while running in normal playback mode
void handleWebDashboard();

extern String device_ip_str;

#endif // CONFIG_MANAGER_H
