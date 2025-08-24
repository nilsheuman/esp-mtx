#ifndef GIF_PLAYER_H
#define GIF_PLAYER_H

#include <Arduino.h>
#include <AnimatedGIF.h>

#define MAX_GIF_FRAMES 50
#define MATRIX_WIDTH 64
#define MATRIX_HEIGHT 32

struct GifFrame {
    uint16_t* pixels;
    uint16_t delay;
    uint16_t overrideDelay;  // 0 = use original delay
    bool allocated;
};

struct PreloadedGifState {
    GifFrame frames[MAX_GIF_FRAMES];
    int totalFrames;
    int currentFrame;
    unsigned long lastFrameTime;
    bool playing;
    bool looping;
    bool drawn;
    uint16_t globalDelayOverride;  // 0 = no override, >0 = ms per frame
    uint16_t lastFrameDelayOverride;   // 0 = use normal delay, >0 = ms for last frame
    uint16_t rotation;
    AnimatedGIF gif;
};

extern PreloadedGifState preloadedGifState;

void initPreloadedGifPlayer(PreloadedGifState& state);
bool loadGifIntoMemory(PreloadedGifState& state, const char* filename);
void startPreloadedGifPlayer(PreloadedGifState& state);
void stopPreloadedGifPlayer(PreloadedGifState& state);
void updatePreloadedGifPlayer(PreloadedGifState& state);
void freeGifFrames(PreloadedGifState& state);
void drawCurrentFrame(PreloadedGifState& state);
void setFrameDelay(int frameIndex, uint16_t delayMs);
void setGlobalFrameDelay(uint16_t delayMs);
void setLastFrameDelay(uint16_t delayMs);
uint16_t getCurrentFrameDelay(PreloadedGifState& state);
void GIFDrawToMemory(GIFDRAW* pDraw);

#endif