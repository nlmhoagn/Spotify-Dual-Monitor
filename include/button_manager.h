#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "globals.h"

// Initialize GPIO pins for physical buttons
void initButtons();

// Check physical button states with non-blocking software debouncing
void handleButtons();

#endif // BUTTON_MANAGER_H
