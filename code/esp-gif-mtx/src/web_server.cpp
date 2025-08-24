#include "web_server.h"
#include <LittleFS.h>
#include "display_driver.h"
#include "gif_player.h"
#include "index_html.h"

WebServer server(80);
String displayText = "";

void serverLoop() {
    server.handleClient();
    
    updatePreloadedGifPlayer(preloadedGifState);
    if (preloadedGifState.drawn) {
        // screen was redrawn

        if (displayText.length() > 0) {
            // display.clearDisplay();
            display.setTextColor(myCOLORS[0], myCOLORS[1]); // white text / black background
            display.setTextSize(1);
            display.setCursor(0, 0);
            display.print(displayText);
            display.display(DISPLAY_DRAW_TIME);
        }
    }
    
    yield();
}

void getRoot() {
    Serial.println("Serving root page");
    server.send(200, "text/html", htmlCode);
}

void handleStart() {
    startPreloadedGifPlayer(preloadedGifState);
    server.send(200, "text/plain", "GIF playback started");
}

void handleStop() {
    stopPreloadedGifPlayer(preloadedGifState);
    server.send(200, "text/plain", "GIF playback stopped");
}

void handleInfo() {
    String info = "Memory Info:\n";
    info += "Free heap: " + String(ESP.getFreeHeap()) + " bytes\n";
    info += "Min free heap: " + String(ESP.getMinFreeHeap()) + " bytes\n";
    info += "Loaded frames: " + String(preloadedGifState.totalFrames) + "\n";
    info += "Current frame: " + String(preloadedGifState.currentFrame) + "\n";
    info += "Playing: " + String(preloadedGifState.playing ? "Yes" : "No") + "\n";
    
    server.send(200, "text/plain", info);
}

void handleSetGlobalDelay() {
    if (server.hasArg("delay")) {
        uint16_t rate = server.arg("delay").toInt();
        setGlobalFrameDelay(rate);
        server.send(200, "text/plain", "Global delay set to " + String(rate) + "ms");
    } else {
        server.send(400, "text/plain", "Missing delay parameter");
    }
}

void handleSetLastDelay() {
    if (server.hasArg("delay")) {
        uint16_t rate = server.arg("delay").toInt();
        setLastFrameDelay(rate);
        server.send(200, "text/plain", "Last frame delay set to " + String(rate) + "ms");
    } else {
        server.send(400, "text/plain", "Missing delay parameter");
    }
}

void handleSetFrameDelay() {
    if (server.hasArg("index") && server.hasArg("delay")) {
        int index = server.arg("index").toInt();
        uint16_t rate = server.arg("delay").toInt();
        setFrameDelay(index, rate);
        server.send(200, "text/plain", "Frame " + String(index) + " delay set to " + String(rate) + "ms");
    } else {
        server.send(400, "text/plain", "Missing index or delay parameter");
    }
}

void handleSetBrightness() {
    if (server.hasArg("brightness")) {
        uint8_t brightness = server.arg("brightness").toInt();
        if (brightness >= 0 && brightness <= 255) {   
            display.setBrightness(brightness);
            Serial.printf("Brightness set to %d\n", brightness);
            server.send(200, "text/plain", "Brightness set to " + String(brightness));
        } else {
            server.send(400, "text/plain", "Invalid brightness value. Must be 0-255.");
        }
    } else {
        server.send(400, "text/plain", "Missing brightness parameter");
    }
}

void handleSetRotation() {
    if (server.hasArg("rotation")) {
        uint8_t rotation = server.arg("rotation").toInt();
        if (rotation >= 0 && rotation <= 3) {
            // display.setRotation(rotation);
            // display.setRotate(rotation);
            // preloadedGifState.rotation = rotation;
            // Serial.printf("Rotation set to %d\n", rotation);
            // server.send(200, "text/plain", "Rotation set to " + String(rotation));
            Serial.printf("Rotation not supported\n");
            server.send(200, "text/plain", "Rotation not supported");
        } else {
            server.send(400, "text/plain", "Invalid rotation value. Must be 0-3.");
        }
    } else {
        server.send(400, "text/plain", "Missing rotation parameter");
    }
}

void handleSetText() {
    if (server.hasArg("text")) {
        String text = server.arg("text");
        displayText = text; // Store the text in global variable
        server.send(200, "text/plain", "Text set to: " + text);
    } else {
        displayText = ""; // Clear text if no parameter provided
        server.send(200, "text/plain", "Text cleared");
    }
}

void handleFileUpload() {
    HTTPUpload& upload = server.upload();
    static File uploadFile;
    
    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Upload Start: %s\n", upload.filename.c_str());
        
        if (!upload.filename.endsWith(".gif")) {
            server.send(400, "text/plain", "Only GIF files allowed");
            return;
        }
        
        display_update_enable(false);
        stopPreloadedGifPlayer(preloadedGifState);
        freeGifFrames(preloadedGifState);
        
        if (LittleFS.exists("/uploaded.gif")) {
            LittleFS.remove("/uploaded.gif");
        }
        
        uploadFile = LittleFS.open("/uploaded.gif", FILE_WRITE);
        if (!uploadFile) {
            server.send(500, "text/plain", "Failed to open file for writing");
            return;
        }
    }
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            size_t written = uploadFile.write(upload.buf, upload.currentSize);
            if (written != upload.currentSize) {
                Serial.println("Write size mismatch during upload!");
            }
        }
    }
    else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
            Serial.printf("Upload Complete: %s (%u bytes)\n", 
                         upload.filename.c_str(), upload.totalSize);
            
            File verifyFile = LittleFS.open("/uploaded.gif", FILE_READ);
            if (verifyFile) {
                uint8_t header[6];
                verifyFile.read(header, 6);
                verifyFile.close();
                
                if (strncmp((char*)header, "GIF87a", 6) != 0 && strncmp((char*)header, "GIF89a", 6) != 0) {
                    Serial.println("Invalid GIF header!");
                    server.send(400, "text/plain", "Invalid GIF file");
                    return;
                }
            }
            
            Serial.println("Loading GIF into memory...");
            bool loadSuccess = loadGifIntoMemory(preloadedGifState, "/uploaded.gif");
            
            if (loadSuccess) {
                Serial.println("GIF loaded successfully - starting playback");
                startPreloadedGifPlayer(preloadedGifState);
                
                String response = "GIF loaded successfully!\n";
                response += "Frames loaded: " + String(preloadedGifState.totalFrames) + "\n";
                response += "Free heap: " + String(ESP.getFreeHeap()) + " bytes";
                
                server.send(200, "text/plain", response);
                display_update_enable(true);
            } else {
                Serial.println("Failed to load GIF into memory");
                server.send(500, "text/plain", "Failed to load GIF into memory. File may be too large or corrupted.");
            }
        }
    }
    else if (upload.status == UPLOAD_FILE_ABORTED) {
        Serial.println("Upload aborted");
        if (uploadFile) {
            uploadFile.close();
        }
        server.send(400, "text/plain", "Upload aborted");
    }
}

void sendCrossOriginHeader() {
    server.send(204);
}

void setupApi() {
    server.enableCORS();
    
    server.on("/", HTTP_OPTIONS, sendCrossOriginHeader);
    server.on("/", getRoot);
    
    // gif

    server.on("/upload", HTTP_OPTIONS, sendCrossOriginHeader);
    server.on("/upload", HTTP_POST, 
        []() { 
            // This lambda is called after upload is complete
        },
        handleFileUpload
    );
    
    server.on("/start", HTTP_OPTIONS, sendCrossOriginHeader);
    server.on("/start", HTTP_GET, handleStart);
    
    server.on("/stop", HTTP_OPTIONS, sendCrossOriginHeader);  
    server.on("/stop", HTTP_GET, handleStop);
    
    server.on("/info", HTTP_OPTIONS, sendCrossOriginHeader);
    server.on("/info", HTTP_GET, handleInfo);
    
    server.on("/set-global-delay", HTTP_GET, handleSetGlobalDelay);
    server.on("/set-last-delay", HTTP_GET, handleSetLastDelay);
    server.on("/set-frame-delay", HTTP_GET, handleSetFrameDelay);

    // setup

    server.on("/set-brightness", HTTP_GET, handleSetBrightness);
    server.on("/set-rotation", HTTP_GET, handleSetRotation);
    
    // drawing

    server.on("/set-text", HTTP_GET, handleSetText);

    server.begin();
    Serial.println("Web server started with preloaded GIF support");
}