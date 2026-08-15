#include "button_manager.h"

struct ButtonInfo {
  uint8_t pin;
  PlayerCommand command;
  bool lastReading;
  bool stableState;
  unsigned long lastDebounceTime;
  const char* name;
};

static ButtonInfo buttons[] = {
  { BUTTON_PLAY_PAUSE_PIN, CMD_PLAY_PAUSE, HIGH, HIGH, 0, "PLAY/PAUSE" },
  { BUTTON_NEXT_PIN,       CMD_NEXT,       HIGH, HIGH, 0, "SKIP NEXT" },
  { BUTTON_PREV_PIN,       CMD_PREV,       HIGH, HIGH, 0, "SKIP PREV" }
};

static const size_t NUM_BUTTONS = sizeof(buttons) / sizeof(buttons[0]);

void initButtons() {
  for (size_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);
    buttons[i].lastReading = digitalRead(buttons[i].pin);
    buttons[i].stableState = buttons[i].lastReading;
    buttons[i].lastDebounceTime = millis();
  }
  Serial.println("[BUTTONS] Initialized GPIO pins (INPUT_PULLUP) for Play/Pause, Next, Prev!");
}

void handleButtons() {
  unsigned long now = millis();

  for (size_t i = 0; i < NUM_BUTTONS; i++) {
    bool reading = digitalRead(buttons[i].pin);

    // If noise or button press changed reading, reset debounce timer
    if (reading != buttons[i].lastReading) {
      buttons[i].lastDebounceTime = now;
      buttons[i].lastReading = reading;
    }

    // Check if reading remained stable longer than DEBOUNCE_DELAY_MS
    if ((now - buttons[i].lastDebounceTime) > DEBOUNCE_DELAY_MS) {
      if (reading != buttons[i].stableState) {
        buttons[i].stableState = reading;

        // Button press action triggered on HIGH -> LOW transition (Active LOW)
        if (buttons[i].stableState == LOW) {
          Serial.printf("[BUTTON] %s Pressed (GPIO %d)\n", buttons[i].name, buttons[i].pin);

          xSemaphoreTake(dataMutex, portMAX_DELAY);
          
          if (buttons[i].command == CMD_PLAY_PAUSE) {
            if (is_playing) {
              pending_player_cmd = CMD_PAUSE;
              is_playing = false;
            } else {
              pending_player_cmd = CMD_PLAY;
              is_playing = true;
            }
            last_progress_tick = millis();
          } else {
            pending_player_cmd = buttons[i].command;
          }

          xSemaphoreGive(dataMutex);
        }
      }
    }
  }
}
