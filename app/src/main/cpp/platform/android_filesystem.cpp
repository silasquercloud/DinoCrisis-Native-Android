#include "platform.h"

#include <android/log.h>
#include <filesystem>
#include <fstream>

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

bool readOnlyFileSize(const std::string& path, std::uint64_t& size) {
    std::error_code error;
    const auto fileSize = std::filesystem::file_size(path, error);
    if (error) {
        return false;
    }
    size = static_cast<std::uint64_t>(fileSize);
    return true;
}

bool readOnlyFileRange(const std::string& path, std::uint64_t offset, std::size_t length, std::vector<std::uint8_t>& data) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }
    input.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!input.good()) {
        return false;
    }
    data.resize(length);
    input.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(length));
    if (input.gcount() != static_cast<std::streamsize>(length)) {
        data.clear();
        return false;
    }
    return true;
}
}  // namespace platform