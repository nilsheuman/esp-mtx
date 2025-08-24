#include <Arduino.h>
#include <WebServer.h>
#include "display_driver.h"
#include "network.h"
#include "web_server.h"

#include <LittleFS.h>

void setupFilesystem() {
    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
        
        if (LittleFS.format()) {
            Serial.println("LittleFS Formatted");
            LittleFS.begin();
        }
    } else {
        Serial.println("LittleFS Mounted");
    }
}

void setup() {

  Serial.begin(115200);
  
  setupFilesystem();
  setupWifi();
  setupDisplay();
  setupApi();

  showIp();
}

void loop() {
  serverLoop();
}
