#pragma once
#include <cstdint>
#include <string>
namespace platform {
struct InputState { float touchX = 0.0F; float touchY = 0.0F; bool touchDown = false; };
void setGameDataPath(const std::string& path);
std::string gameDataPath();
uint64_t monotonicMilliseconds();
void logInfo(const std::string& message);
void logError(const std::string& message);
InputState currentInput();
void initializeAudio();
void shutdownAudio();
void clearGraphics(float red, float green, float blue);
}