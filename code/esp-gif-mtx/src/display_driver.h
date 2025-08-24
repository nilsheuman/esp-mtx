#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include <Arduino.h>
#include "config.h"
#include <PxMatrix.h>
#include <WiFiManager.h>

enum ColorIndex {
    WHITE = 0,
    BLACK = 1,
    RED = 2,
    GREEN = 3,
    BLUE = 4,
    YELLOW = 5,
    CYAN = 6,
    MAGENTA = 7
};

extern PxMATRIX display;
extern uint16_t myCOLORS[NUM_COLORS];
extern hw_timer_t* timer;
extern portMUX_TYPE timerMux;

void setupDisplay();
void showIp();
void display_update_enable(bool is_enable);
void IRAM_ATTR display_updater();

#endif // DISPLAY_DRIVER_H