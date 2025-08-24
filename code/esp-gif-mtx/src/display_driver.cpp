#include "display_driver.h"

// Timer and mutex for ESP32
hw_timer_t* timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// Display object
PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D);

// Color definitions
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 100, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myBLACK = display.color565(0, 0, 0);

// Color array
uint16_t myCOLORS[NUM_COLORS] = {
    myWHITE,    // 0
    myBLACK,    // 1
    myRED,      // 2
    myGREEN,    // 3
    myBLUE,     // 4
    myYELLOW,   // 5
    myCYAN,     // 6
    myMAGENTA   // 7
};

void IRAM_ATTR display_updater() {
    // Increment the counter and set the time of ISR
    portENTER_CRITICAL_ISR(&timerMux);
    display.display(DISPLAY_DRAW_TIME);
    portEXIT_CRITICAL_ISR(&timerMux);
}

void display_update_enable(bool is_enable) {
#ifdef ESP32
    if (is_enable) {
        timer = timerBegin(0, 80, true);
        timerAttachInterrupt(timer, &display_updater, true);
        timerAlarmWrite(timer, 4000, true);
        timerAlarmEnable(timer);
    } else {
        timerDetachInterrupt(timer);
        timerAlarmDisable(timer);
    }
#endif
}

void setupDisplay() {
  // Define your display layout here, e.g. 1/8 step, and optional SPI pins begin(row_pattern, CLK, MOSI, MISO, SS)
  display.begin(8);

  // Define multiplex implemention here {BINARY, STRAIGHT} (default is BINARY)
  display.setMuxPattern(BINARY);

  // Set the multiplex pattern {LINE, ZIGZAG,ZZAGG, ZAGGIZ, WZAGZIG, VZAG, ZAGZIG} (default is LINE)
  display.setScanPattern(LINE);

  // Set the number of panels that make up the display area width (default is 1)
  display.setPanelsWidth(2);

  display.setScanPattern(WZAGZIG2);
    
    // display.setRotation(0); // seems to be broken now

    // Set the brightness of the panels (default is 255)
    display.setBrightness(10);

    display_update_enable(true);
    display.clearDisplay();
    
    // Display startup message
    display.setTextColor(myWHITE, myBLACK);
    display.setCursor(6, 2);
    display.print("ESP PxMtx");
}

void showIp() {
  Serial.print("local ip: ");
  Serial.println(WiFi.localIP());
  
  display.setTextColor(myYELLOW, myBLACK);
  display.setCursor(0,12);
  display.print(WiFi.localIP());
}