#include "platform.h"

#include <android/log.h>

namespace platform {
static std::string s_gameDataPath;

void setGameDataPath(const std::string& path) {
    s_gameDataPath = path;
}

std::string gameDataPath() {
    return s_gameDataPath;
}

void logInfo(const std::string& message) {
    __android_log_print(ANDROID_LOG_INFO, "DinoCrisis", "%s", message.c_str());
}

void logError(const std::string& message) {
    __android_log_print(ANDROID_LOG_ERROR, "DinoCrisis", "%s", message.c_str());
}
}  // namespace platform