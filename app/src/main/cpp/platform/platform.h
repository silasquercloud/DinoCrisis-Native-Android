#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace platform {
struct InputState {
	float touchX = 0.0F;
	float touchY = 0.0F;
	float leftX = 0.0F;
	float leftY = 0.0F;
	float rightX = 0.0F;
	float rightY = 0.0F;
	std::uint32_t buttons = 0;
	bool touchDown = false;
};
void setGameDataPath(const std::string& path);
std::string gameDataPath();
uint64_t monotonicMilliseconds();
void logInfo(const std::string& message);
void logError(const std::string& message);
InputState currentInput();
void initializeAudio();
void shutdownAudio();
void clearGraphics(float red, float green, float blue);
bool readOnlyFileSize(const std::string& path, std::uint64_t& size);
bool readOnlyFileRange(const std::string& path, std::uint64_t offset, std::size_t length, std::vector<std::uint8_t>& data);
}