#include "gif_player.h"
#include "display_driver.h"
#include <LittleFS.h>

PreloadedGifState preloadedGifState;
File gifFile;

// Global variables for frame loading
static uint16_t* currentFrameBuffer = nullptr;
static int currentLoadingFrame = 0;

void initPreloadedGifPlayer(PreloadedGifState& state) {
    state.totalFrames = 0;
    state.currentFrame = 0;
    state.lastFrameTime = 0;
    state.playing = false;
    state.looping = true;
    
    // Initialize all frames as unallocated
    for (int i = 0; i < MAX_GIF_FRAMES; i++) {
        state.frames[i].pixels = nullptr;
        state.frames[i].delay = 100;
        state.frames[i].allocated = false;
    }
    
    // Initialize AnimatedGIF library for loading only
    state.gif.begin(LITTLE_ENDIAN_PIXELS);
}

void freeGifFrames(PreloadedGifState& state) {
    for (int i = 0; i < MAX_GIF_FRAMES; i++) {
        if (state.frames[i].allocated && state.frames[i].pixels != nullptr) {
            free(state.frames[i].pixels);
            state.frames[i].pixels = nullptr;
            state.frames[i].allocated = false;
        }
    }
    state.totalFrames = 0;
    Serial.println("Freed all GIF frames from memory");
}

void GIFDrawToMemory(GIFDRAW* pDraw) {
    if (!currentFrameBuffer) return;
    
    uint8_t* s = pDraw->pPixels;
    uint16_t* usPalette = pDraw->pPalette;
    int x, y, iWidth;
    
    y = pDraw->iY + pDraw->y;
    if (y >= MATRIX_HEIGHT) return;
    
    iWidth = pDraw->iWidth;
    if (iWidth + pDraw->iX > MATRIX_WIDTH)
        iWidth = MATRIX_WIDTH - pDraw->iX;
    if (iWidth <= 0) return;
    
    // Copy pixels to frame buffer
    for (x = 0; x < iWidth; x++) {
        int bufferIndex = (y * MATRIX_WIDTH) + (pDraw->iX + x);
        if (bufferIndex >= 0 && bufferIndex < (MATRIX_WIDTH * MATRIX_HEIGHT)) {
            if (s[x] != pDraw->ucTransparent) {
                currentFrameBuffer[bufferIndex] = usPalette[s[x]];
            }
            // Keep previous pixel for transparent areas
        }
    }
}


void* GIFOpenFile(const char *fname, int32_t *pSize) {
  gifFile = LittleFS.open(fname, "r");
  if (gifFile) {
    *pSize = gifFile.size();
    return (void*)&gifFile;
  }
  return NULL;
}

void GIFCloseFile(void *pHandle) {
  File *f = static_cast<File *>(pHandle);
  if (f != NULL) {
    f->close();
  }
}

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  int32_t iBytesRead;
  iBytesRead = iLen;
  File *f = static_cast<File *>(pFile->fHandle);
  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos - 1;
  if (iBytesRead <= 0)
    return 0;
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();
  return iBytesRead;
}

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
  return pFile->iPos;
}

bool loadGifIntoMemory(PreloadedGifState& state, const char* filename) {
    Serial.printf("Loading GIF into memory: %s\n", filename);
    
    freeGifFrames(state);
    
    if (!state.gif.open(filename, GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDrawToMemory)) {
        Serial.println("Failed to open GIF file");
        return false;
    }
    
    GIFINFO gi;
    if (!state.gif.getInfo(&gi)) {
        Serial.println("Failed to get GIF info");
        state.gif.close();
        return false;
    }
    
    Serial.printf("GIF Info - Size: %dx%d, Frames: %d, Duration: %dms\n", 
                  state.gif.getCanvasWidth(), state.gif.getCanvasHeight(),
                  gi.iFrameCount, gi.iDuration);
    
    int framesToLoad = min(gi.iFrameCount, MAX_GIF_FRAMES);
    
    // Load each frame into memory
    for (int frameIndex = 0; frameIndex < framesToLoad; frameIndex++) {
        Serial.printf("Loading frame %d/%d\n", frameIndex + 1, framesToLoad);
        
        state.frames[frameIndex].pixels = (uint16_t*)malloc(MATRIX_WIDTH * MATRIX_HEIGHT * sizeof(uint16_t));
        if (!state.frames[frameIndex].pixels) {
            Serial.printf("Failed to allocate memory for frame %d\n", frameIndex);
            freeGifFrames(state);
            state.gif.close();
            return false;
        }
        
        // Clear frame buffer (black background)
        memset(state.frames[frameIndex].pixels, 0, MATRIX_WIDTH * MATRIX_HEIGHT * sizeof(uint16_t));
        
        currentFrameBuffer = state.frames[frameIndex].pixels;
        currentLoadingFrame = frameIndex;
        
        // Play the frame to load it into memory
        int result = state.gif.playFrame(true, nullptr);
        if (result == -1) {
            Serial.printf("Failed to load frame %d, result: %d\n", frameIndex, result);
            if (frameIndex == 0) {
                // If we can't load the first frame, fail completely
                freeGifFrames(state);
                state.gif.close();
                return false;
            } else {
                // If we can't load later frames, just use what we have
                break;
            }
        } else if (result == 0) {
          state.gif.reset();
        }
        
        // Get frame delay (convert from 1/100th seconds to milliseconds)
        state.frames[frameIndex].delay = max(50, gi.iMinDelay * 10); // this seems a bit odd
        Serial.printf("Frame index %d, delay: %d\n", frameIndex, state.frames[frameIndex].delay);
        state.frames[frameIndex].allocated = true;
        state.totalFrames++;
        
        // Yield to prevent watchdog timeout
        yield();
    }
    
    currentFrameBuffer = nullptr;
    state.gif.close();
    
    Serial.printf("Successfully loaded %d frames into memory\n", state.totalFrames);
    Serial.printf("Total memory used: %d bytes\n", 
                  state.totalFrames * MATRIX_WIDTH * MATRIX_HEIGHT * sizeof(uint16_t));
    
    return state.totalFrames > 0;
}

void startPreloadedGifPlayer(PreloadedGifState& state) {
    if (state.totalFrames == 0) {
        Serial.println("No frames loaded - cannot start playback");
        return;
    }
    
    state.currentFrame = 0;
    state.lastFrameTime = millis();
    state.playing = true;
    
    drawCurrentFrame(state);
    
    Serial.printf("Started preloaded GIF playback with %d frames\n", state.totalFrames);
}

void stopPreloadedGifPlayer(PreloadedGifState& state) {
    state.playing = false;
    display.clearDisplay();
    Serial.println("Stopped preloaded GIF playback");
}

void drawCurrentFrame(PreloadedGifState& state) {
    if (state.currentFrame >= state.totalFrames || !state.frames[state.currentFrame].allocated) {
        return;
    }
    
    uint16_t* framePixels = state.frames[state.currentFrame].pixels;
    if (!framePixels) return;
    
    // Fast bulk pixel drawing - disable interrupts briefly
    portENTER_CRITICAL(&timerMux);
    
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            int pixelIndex = (y * MATRIX_WIDTH) + x;
            uint16_t color = framePixels[pixelIndex];
            
            display.drawPixel(x, y, color);
        }
    }
    
    portEXIT_CRITICAL(&timerMux);
}

void updatePreloadedGifPlayer(PreloadedGifState& state) {
    state.drawn = false;
    if (!state.playing || state.totalFrames == 0) {
        return;
    }
    
    unsigned long currentTime = millis();
    uint16_t frameDelay = getCurrentFrameDelay(state);
    
    if (currentTime - state.lastFrameTime >= frameDelay) {
        state.currentFrame++;
        
        if (state.currentFrame >= state.totalFrames) {
          state.currentFrame = 0;
        }
        
        drawCurrentFrame(state);
        state.drawn = true;
        
        state.lastFrameTime = currentTime;
    }
}

void setFrameDelay(int frameIndex, uint16_t delayMs) {
    if (frameIndex >= 0 && frameIndex < preloadedGifState.totalFrames) {
      Serial.printf("setFrameDelay: index: %d delay: %d ms\n", frameIndex, delayMs);
      preloadedGifState.frames[frameIndex].overrideDelay = delayMs;
    }
}

void setGlobalFrameDelay(uint16_t delayMs) {
  Serial.printf("setGlobalFrameDelay: delay: %d ms\n", delayMs);
  preloadedGifState.globalDelayOverride = delayMs;
}

void setLastFrameDelay(uint16_t delayMs) {
  Serial.printf("setLastFrameDelay: delay: %d ms\n", delayMs);
  preloadedGifState.lastFrameDelayOverride = delayMs;
}

uint16_t getCurrentFrameDelay(PreloadedGifState& state) {
    if (state.currentFrame >= state.totalFrames) {
        return 100;
    }
    
    bool isLastFrame = (state.currentFrame == state.totalFrames - 1);
    
    if (isLastFrame && state.lastFrameDelayOverride > 0) {
        return state.lastFrameDelayOverride;
    }
    
    if (state.frames[state.currentFrame].overrideDelay > 0) {
        return state.frames[state.currentFrame].overrideDelay;
    }

    if (state.globalDelayOverride > 0) {
        return state.globalDelayOverride;
    }
    
    return state.frames[state.currentFrame].delay;
}

void printMemoryInfo() {
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Min free heap: %d bytes\n", ESP.getMinFreeHeap());
    Serial.printf("Max alloc heap: %d bytes\n", ESP.getMaxAllocHeap());
}