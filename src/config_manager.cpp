#include "config_manager.h"
#include "globals.h"
#include "ui_renderer.h"

#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ArduinoJson.h>

static Preferences prefs;
static WebServer server(80);
static DNSServer dnsServer;
static const byte DNS_PORT = 53;

String device_ip_str = "";

// Default fallback credentials if NVS is empty
static const char* DEFAULT_SSID                  = "HnP";
static const char* DEFAULT_PASS                  = "20072013";
static const char* DEFAULT_SPOTIFY_CLIENT_ID     = "39ba1e8f7b5d47888015e95e755cde67";
static const char* DEFAULT_SPOTIFY_CLIENT_SECRET = "6de5795fb49b44fa9555166f5bf6067f";
static const char* DEFAULT_SPOTIFY_REFRESH_TOKEN = "AQDAPd2ZxHqYNt6l1-Yb86wRWUITWTh9VGaz2zec7dj8FrBe1p6tAfg1P94MDcSgOlAfLUWBbrUee2ps16nlNR9XqaZcU7KlouWF1SyrAH8H4hYjabvglNXIXosZon6Dwqg";

// HTML Web Portal UI (Spotify Dark Theme with UI Customization Panel)
static const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Spotify Dual Monitor Dashboard</title>
  <style>
    :root {
      --bg-color: #121212;
      --card-bg: #181818;
      --accent: #1DB954;
      --accent-hover: #1ed760;
      --text-main: #FFFFFF;
      --text-sub: #B3B3B3;
      --input-bg: #282828;
      --border-color: #333333;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif; }
    body { background-color: var(--bg-color); color: var(--text-main); display: flex; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }
    .container { background-color: var(--card-bg); width: 100%; max-width: 480px; padding: 30px; border-radius: 16px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); border: 1px solid var(--border-color); }
    .header { text-align: center; margin-bottom: 25px; }
    .header h1 { font-size: 22px; font-weight: 700; color: var(--accent); margin-bottom: 8px; display: flex; align-items: center; justify-content: center; gap: 8px; }
    .header p { font-size: 13px; color: var(--text-sub); }
    .ip-badge { display: inline-block; background-color: var(--input-bg); color: var(--accent); padding: 4px 12px; border-radius: 12px; font-size: 12px; font-weight: 600; margin-top: 5px; border: 1px solid var(--border-color); }
    .section-title { font-size: 15px; font-weight: 700; color: var(--accent); margin: 25px 0 15px 0; border-top: 1px solid var(--border-color); padding-top: 18px; display: flex; align-items: center; gap: 6px; }
    .form-group { margin-bottom: 18px; }
    label { display: block; font-size: 13px; font-weight: 600; color: var(--text-sub); margin-bottom: 6px; text-transform: uppercase; letter-spacing: 0.5px; }
    input[type="text"], input[type="password"], select { width: 100%; padding: 12px 14px; background-color: var(--input-bg); border: 1px solid var(--border-color); border-radius: 8px; color: var(--text-main); font-size: 14px; outline: none; transition: border-color 0.2s; }
    input:focus, select:focus { border-color: var(--accent); }
    .btn-group { display: flex; flex-direction: column; gap: 10px; margin-top: 25px; }
    .btn { width: 100%; padding: 14px; border: none; border-radius: 25px; font-size: 14px; font-weight: 700; cursor: pointer; transition: transform 0.1s, background-color 0.2s; text-align: center; text-decoration: none; }
    .btn-primary { background-color: var(--accent); color: #000000; }
    .btn-primary:hover { background-color: var(--accent-hover); transform: scale(1.02); }
    .btn-secondary { background-color: transparent; color: var(--text-sub); border: 1px solid var(--border-color); }
    .btn-secondary:hover { color: var(--text-main); border-color: var(--text-sub); }
    .scan-row { display: flex; gap: 8px; }
    .btn-scan { padding: 0 16px; background-color: var(--input-bg); border: 1px solid var(--border-color); color: var(--text-main); border-radius: 8px; cursor: pointer; white-space: nowrap; font-size: 13px; }
    .btn-scan:hover { border-color: var(--accent); }
    .footer { margin-top: 20px; text-align: center; font-size: 11px; color: var(--text-sub); }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>🎵 Spotify Dual Monitor</h1>
      <p>Always-On Device Web Dashboard</p>
      <div class="ip-badge">IP: {{DEVICE_IP}} | spotify-display.local</div>
    </div>

    <form action="/save" method="POST">
      <div class="section-title">🎨 UI & Display Settings</div>

      <div class="form-group">
        <label for="theme_color">Theme Accent Color</label>
        <select id="theme_color" name="theme_color">
          <option value="0x06C4" {{SEL_THEME_0x06C4}}>🟢 Spotify Green</option>
          <option value="0x06BF" {{SEL_THEME_0x06BF}}>🔵 Electric Cyan</option>
          <option value="0xF81F" {{SEL_THEME_0xF81F}}>💗 Neon Pink</option>
          <option value="0xFD20" {{SEL_THEME_0xFD20}}>🟧 Sunset Orange</option>
          <option value="0x901F" {{SEL_THEME_0x901F}}>🟣 Deep Purple</option>
          <option value="0xFFFF" {{SEL_THEME_0xFFFF}}>⚪ Minimalist White</option>
        </select>
      </div>

      <div class="form-group">
        <label for="vinyl_mode">Secondary Display Mode (ST7735)</label>
        <select id="vinyl_mode" name="vinyl_mode">
          <option value="0" {{SEL_VMODE_0}}>💿 Spinning Vinyl Disc (Classic)</option>
          <option value="1" {{SEL_VMODE_1}}>🖼️ Full Square Album Cover</option>
        </select>
      </div>

      <div class="form-group">
        <label for="eq_anim">Audio EQ Visualizer</label>
        <select id="eq_anim" name="eq_anim">
          <option value="1" {{SEL_EQ_1}}>📊 Enabled (Animated Spectrum)</option>
          <option value="0" {{SEL_EQ_0}}>🛑 Disabled (Off)</option>
        </select>
      </div>

      <div class="section-title">📶 Wi-Fi & Credentials</div>

      <div class="form-group">
        <label for="ssid">Wi-Fi Network (SSID)</label>
        <div class="scan-row">
          <input type="text" id="ssid" name="ssid" value="{{SSID}}" placeholder="Enter or select SSID" required>
          <button type="button" class="btn-scan" onclick="scanWifi()">🔍 Scan</button>
        </div>
        <select id="wifi-select" style="display:none; margin-top: 8px;" onchange="selectWifi(this.value)">
          <option value="">-- Select Wi-Fi Network --</option>
        </select>
      </div>

      <div class="form-group">
        <label for="password">Wi-Fi Password</label>
        <input type="password" id="password" name="password" value="{{PASS}}" placeholder="Enter Wi-Fi password">
      </div>

      <div class="form-group">
        <label for="client_id">Spotify Client ID</label>
        <input type="text" id="client_id" name="client_id" value="{{CLIENT_ID}}" placeholder="Enter Spotify Client ID" required>
      </div>

      <div class="form-group">
        <label for="client_secret">Spotify Client Secret</label>
        <input type="password" id="client_secret" name="client_secret" value="{{CLIENT_SECRET}}" placeholder="Enter Spotify Client Secret" required>
      </div>

      <div class="form-group">
        <label for="refresh_token">Spotify Refresh Token</label>
        <input type="password" id="refresh_token" name="refresh_token" value="{{REFRESH_TOKEN}}" placeholder="Enter Spotify Refresh Token" required>
      </div>

      <div class="btn-group">
        <button type="submit" class="btn btn-primary">Save Settings</button>
        <a href="/reset" class="btn btn-secondary" onclick="return confirm('Are you sure you want to clear all saved configurations?')">Reset Default Configuration</a>
      </div>
    </form>

    <div class="footer">
      ESP32-S3 Dual-Core FreeRTOS | Spotify Karaoke Display
    </div>
  </div>

  <script>
    function scanWifi() {
      const select = document.getElementById('wifi-select');
      select.style.display = 'block';
      select.innerHTML = '<option value="">Scanning Wi-Fi networks...</option>';
      fetch('/scan')
        .then(res => res.json())
        .then(data => {
          select.innerHTML = '<option value="">-- Select discovered Wi-Fi network --</option>';
          data.forEach(item => {
            const opt = document.createElement('option');
            opt.value = item.ssid;
            opt.textContent = `${item.ssid} (${item.rssi} dBm)`;
            select.appendChild(opt);
          });
        })
        .catch(err => {
          select.innerHTML = '<option value="">Error scanning Wi-Fi networks!</option>';
        });
    }

    function selectWifi(val) {
      if (val) {
        document.getElementById('ssid').value = val;
      }
    }
  </script>
</body>
</html>
)rawliteral";

static String getPageHTML() {
  String page = String(CONFIG_HTML);
  page.replace("{{DEVICE_IP}}", (device_ip_str.length() > 0) ? device_ip_str : "192.168.4.1");
  page.replace("{{SSID}}", ssid);
  page.replace("{{PASS}}", password);
  page.replace("{{CLIENT_ID}}", spotify_client_id);
  page.replace("{{CLIENT_SECRET}}", spotify_client_secret);
  page.replace("{{REFRESH_TOKEN}}", spotify_refresh_token);

  // Theme dropdown selected option
  page.replace("{{SEL_THEME_0x06C4}}", (theme_accent_color == 0x06C4) ? "selected" : "");
  page.replace("{{SEL_THEME_0x06BF}}", (theme_accent_color == 0x06BF) ? "selected" : "");
  page.replace("{{SEL_THEME_0xF81F}}", (theme_accent_color == 0xF81F) ? "selected" : "");
  page.replace("{{SEL_THEME_0xFD20}}", (theme_accent_color == 0xFD20) ? "selected" : "");
  page.replace("{{SEL_THEME_0x901F}}", (theme_accent_color == 0x901F) ? "selected" : "");
  page.replace("{{SEL_THEME_0xFFFF}}", (theme_accent_color == 0xFFFF) ? "selected" : "");

  // Vinyl mode selected option
  page.replace("{{SEL_VMODE_0}}", (vinyl_render_mode == 0) ? "selected" : "");
  page.replace("{{SEL_VMODE_1}}", (vinyl_render_mode == 1) ? "selected" : "");

  // EQ animated spectrum selected option
  page.replace("{{SEL_EQ_1}}", (eq_enabled == 1) ? "selected" : "");
  page.replace("{{SEL_EQ_0}}", (eq_enabled == 0) ? "selected" : "");

  return page;
}

bool loadCredentialsFromNVS() {
  prefs.begin("spotify_cfg", true); // Read-only mode
  String saved_ssid = prefs.getString("ssid", "");
  String saved_pass = prefs.getString("pass", "");
  String saved_cid  = prefs.getString("cid", "");
  String saved_csec = prefs.getString("csec", "");
  String saved_rtok = prefs.getString("rtoken", "");

  theme_accent_color = prefs.getUShort("theme", 0x06C4);
  vinyl_render_mode   = prefs.getUChar("vmode", 0);
  eq_enabled          = prefs.getUChar("eq", 1);
  prefs.end();

  if (saved_ssid.length() > 0 && saved_cid.length() > 0 && saved_rtok.length() > 0) {
    ssid                  = saved_ssid;
    password              = saved_pass;
    spotify_client_id     = saved_cid;
    spotify_client_secret = saved_csec;
    spotify_refresh_token = saved_rtok;

    Serial.println("[NVS] Credentials loaded successfully from NVS Flash!");
    Serial.printf("  SSID: %s\n", ssid.c_str());
    Serial.printf("  Theme Color: 0x%04X, Vinyl Mode: %d, EQ: %d\n", theme_accent_color, vinyl_render_mode, eq_enabled);
    return true;
  }

  // Fallback to default hardcoded credentials if NVS is empty
  ssid                  = DEFAULT_SSID;
  password              = DEFAULT_PASS;
  spotify_client_id     = DEFAULT_SPOTIFY_CLIENT_ID;
  spotify_client_secret = DEFAULT_SPOTIFY_CLIENT_SECRET;
  spotify_refresh_token = DEFAULT_SPOTIFY_REFRESH_TOKEN;

  Serial.println("[NVS] NVS is empty. Loaded default fallback credentials!");
  return true;
}

void saveCredentialsToNVS(const String& new_ssid, const String& new_password,
                          const String& new_client_id, const String& new_client_secret,
                          const String& new_refresh_token) {
  prefs.begin("spotify_cfg", false); // Read-write mode
  prefs.putString("ssid", new_ssid);
  prefs.putString("pass", new_password);
  prefs.putString("cid", new_client_id);
  prefs.putString("csec", new_client_secret);
  prefs.putString("rtoken", new_refresh_token);

  prefs.putUShort("theme", theme_accent_color);
  prefs.putUChar("vmode", vinyl_render_mode);
  prefs.putUChar("eq", eq_enabled);
  prefs.end();

  ssid                  = new_ssid;
  password              = new_password;
  spotify_client_id     = new_client_id;
  spotify_client_secret = new_client_secret;
  spotify_refresh_token = new_refresh_token;

  Serial.println("[NVS] Saved WiFi, Spotify & UI Settings to NVS!");
}

void clearCredentialsNVS() {
  prefs.begin("spotify_cfg", false);
  prefs.clear();
  prefs.end();

  // Reset in-memory credentials & UI settings back to defaults
  ssid                  = DEFAULT_SSID;
  password              = DEFAULT_PASS;
  spotify_client_id     = DEFAULT_SPOTIFY_CLIENT_ID;
  spotify_client_secret = DEFAULT_SPOTIFY_CLIENT_SECRET;
  spotify_refresh_token = DEFAULT_SPOTIFY_REFRESH_TOKEN;

  theme_accent_color    = 0x06C4;
  vinyl_render_mode     = 0;
  eq_enabled            = 1;

  Serial.println("[NVS] Cleared all NVS credentials & reset memory to defaults!");
}

void startConfigPortal() {
  in_config_mode = true;

  WiFi.mode(WIFI_AP_STA);
  IPAddress local_ip(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);

  WiFi.softAPConfig(local_ip, gateway, subnet);
  const char* ap_name = "Spotify-Display-Setup";
  WiFi.softAP(ap_name);

  Serial.println("\n==========================================");
  Serial.println("     SPOTIFY DISPLAY - CONFIG AP MODE     ");
  Serial.println("==========================================");
  Serial.printf("AP SSID: %s\n", ap_name);
  Serial.printf("AP IP  : %s\n", WiFi.softAPIP().toString().c_str());

  dnsServer.start(DNS_PORT, "*", local_ip);

  // Serve Main HTML Page with pre-filled existing values
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", getPageHTML());
  });

  // Handle Form Submission with Partial Update Support & UI Settings
  server.on("/save", HTTP_POST, []() {
    String new_ssid  = server.arg("ssid");
    String new_pass  = server.arg("password");
    String new_cid   = server.arg("client_id");
    String new_csec  = server.arg("client_secret");
    String new_rtok  = server.arg("refresh_token");

    String theme_str = server.arg("theme_color");
    String vmode_str = server.arg("vinyl_mode");
    String eq_str    = server.arg("eq_anim");

    // Partial update: retain current memory value if submitted parameter is empty
    if (new_ssid.length() == 0) new_ssid = ssid;
    if (new_cid.length() == 0)  new_cid = spotify_client_id;
    if (new_csec.length() == 0) new_csec = spotify_client_secret;
    if (new_rtok.length() == 0) new_rtok = spotify_refresh_token;

    if (theme_str.length() > 0) {
      theme_accent_color = (uint16_t)strtoul(theme_str.c_str(), NULL, 0);
    }
    if (vmode_str.length() > 0) {
      vinyl_render_mode = (uint8_t)vmode_str.toInt();
    }
    if (eq_str.length() > 0) {
      eq_enabled = (uint8_t)eq_str.toInt();
    }

    saveCredentialsToNVS(new_ssid, new_pass, new_cid, new_csec, new_rtok);

    String respHtml = "<html><head><meta charset='utf-8'><title>Configuration Saved</title>"
                      "<style>body{background:#121212;color:#1DB954;font-family:sans-serif;"
                      "display:flex;justify-content:center;align-items:center;height:100vh;text-align:center;}</style></head>"
                      "<body><div><h1>✅ Configuration Saved!</h1><p style='color:#ccc'>Device is restarting...</p></div>"
                      "<script>setTimeout(function(){window.location.href='/';}, 5000);</script></body></html>";
    server.send(200, "text/html", respHtml);
    delay(2000);
    ESP.restart();
  });

  // WiFi Scan Endpoint (JSON)
  server.on("/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks();
    DynamicJsonDocument doc(2048);
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < n; ++i) {
      JsonObject item = arr.createNestedObject();
      item["ssid"] = WiFi.SSID(i);
      item["rssi"] = WiFi.RSSI(i);
    }

    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
  });

  // Reset NVS Endpoint
  server.on("/reset", HTTP_GET, []() {
    clearCredentialsNVS();
    String respHtml = "<html><body style='background:#121212;color:#ff5555;font-family:sans-serif;text-align:center;padding-top:20%;'>"
                      "<h1>⚠️ NVS Configuration Cleared!</h1><p>Device is restarting...</p></body></html>";
    server.send(200, "text/html", respHtml);
    delay(2000);
    ESP.restart();
  });

  // Captive Portal Redirect
  server.onNotFound([]() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();
  Serial.println("[WEB] Config Web Server & Captive Portal started on port 80");
}

void handleConfigPortal() {
  dnsServer.processNextRequest();
  server.handleClient();
}

void startWebDashboard() {
  in_config_mode = false;
  device_ip_str = WiFi.localIP().toString();

  if (MDNS.begin("spotify-display")) {
    Serial.println("[mDNS] Responder started at http://spotify-display.local");
  }

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", getPageHTML());
  });

  server.on("/save", HTTP_POST, []() {
    String new_ssid  = server.arg("ssid");
    String new_pass  = server.arg("password");
    String new_cid   = server.arg("client_id");
    String new_csec  = server.arg("client_secret");
    String new_rtok  = server.arg("refresh_token");

    String theme_str = server.arg("theme_color");
    String vmode_str = server.arg("vinyl_mode");
    String eq_str    = server.arg("eq_anim");

    bool wifi_changed = (new_ssid.length() > 0 && new_ssid != ssid) || (new_pass.length() > 0 && new_pass != password);
    bool spotify_changed = (new_cid.length() > 0 && new_cid != spotify_client_id) ||
                           (new_csec.length() > 0 && new_csec != spotify_client_secret) ||
                           (new_rtok.length() > 0 && new_rtok != spotify_refresh_token);

    if (new_ssid.length() == 0) new_ssid = ssid;
    if (new_cid.length() == 0)  new_cid = spotify_client_id;
    if (new_csec.length() == 0) new_csec = spotify_client_secret;
    if (new_rtok.length() == 0) new_rtok = spotify_refresh_token;

    if (theme_str.length() > 0) {
      theme_accent_color = (uint16_t)strtoul(theme_str.c_str(), NULL, 0);
    }
    if (vmode_str.length() > 0) {
      vinyl_render_mode = (uint8_t)vmode_str.toInt();
    }
    if (eq_str.length() > 0) {
      eq_enabled = (uint8_t)eq_str.toInt();
    }

    saveCredentialsToNVS(new_ssid, new_pass, new_cid, new_csec, new_rtok);

    // Force UI Redraw
    prev_title = "";
    tft2_title_dirty = true;

    if (wifi_changed || spotify_changed) {
      String respHtml = "<html><head><meta charset='utf-8'><title>Restarting...</title>"
                        "<style>body{background:#121212;color:#1DB954;font-family:sans-serif;"
                        "display:flex;justify-content:center;align-items:center;height:100vh;text-align:center;}</style></head>"
                        "<body><div><h1>✅ WiFi/Spotify Updated!</h1><p style='color:#ccc'>Device is restarting to apply new network settings...</p></div>"
                        "<script>setTimeout(function(){window.location.href='/';}, 5000);</script></body></html>";
      server.send(200, "text/html", respHtml);
      delay(2000);
      ESP.restart();
    } else {
      String respHtml = "<html><head><meta charset='utf-8'><title>UI Updated</title>"
                        "<style>body{background:#121212;color:#1DB954;font-family:sans-serif;"
                        "display:flex;justify-content:center;align-items:center;height:100vh;text-align:center;}</style></head>"
                        "<body><div><h1>🎨 UI Theme Updated!</h1><p style='color:#ccc'>Changes applied live on displays without restarting.</p></div>"
                        "<script>setTimeout(function(){window.location.href='/';}, 2000);</script></body></html>";
      server.send(200, "text/html", respHtml);
    }
  });

  server.on("/scan", HTTP_GET, []() {
    int n = WiFi.scanNetworks();
    DynamicJsonDocument doc(2048);
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < n; ++i) {
      JsonObject item = arr.createNestedObject();
      item["ssid"] = WiFi.SSID(i);
      item["rssi"] = WiFi.RSSI(i);
    }

    String jsonStr;
    serializeJson(doc, jsonStr);
    server.send(200, "application/json", jsonStr);
  });

  server.on("/reset", HTTP_GET, []() {
    clearCredentialsNVS();
    String respHtml = "<html><body style='background:#121212;color:#ff5555;font-family:sans-serif;text-align:center;padding-top:20%;'>"
                      "<h1>⚠️ NVS Configuration Cleared!</h1><p>Device is restarting...</p></body></html>";
    server.send(200, "text/html", respHtml);
    delay(2000);
    ESP.restart();
  });

  server.begin();
  Serial.println("\n==========================================================================");
  Serial.printf("[WEB] Always-On Web Dashboard started on http://%s or http://spotify-display.local\n", device_ip_str.c_str());
  Serial.println("==========================================================================\n");
}

void handleWebDashboard() {
  server.handleClient();
}
