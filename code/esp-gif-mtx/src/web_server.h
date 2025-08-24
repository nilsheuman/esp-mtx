#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include "config.h"
#include <WiFiManager.h>

extern String displayText;

void serverLoop();
void setupApi();
extern WebServer server;

#endif // WEB_SERVER_H