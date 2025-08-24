#ifndef CONFIG_H
#define CONFIG_H

// Matrix display configuration
#define PxMATRIX_COLOR_DEPTH 4
#define PxMATRIX_MAX_HEIGHT 32
#define PxMATRIX_MAX_WIDTH 64
#define PxMATRIX_SPI_FREQUENCY 10000000
#define PxMATRIX_double_buffer true

// Matrix dimensions
#define matrix_width 64
#define matrix_height 32

// Display timing - controls brightness (30-70 is usually fine)
#define DISPLAY_DRAW_TIME 60

// ESP32 Pin definitions for LED Matrix
#ifdef ESP32
#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 17
#define P_OE 16
#endif

// Color definitions
#define NUM_COLORS 8

// Wifi/Accesspoint
#define AP_SSID "esp-pxmtx"
#define AP_PASSWORD "password"

#endif // CONFIG_H