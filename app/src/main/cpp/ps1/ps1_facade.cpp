#include "ps1_facade.h"

#include <filesystem>
#include <fstream>

namespace ps1 {

Facade::Facade(std::string basePath) : basePath_(std::move(basePath)) {}

bool Facade::loadManifest() {
    const std::filesystem::path manifestPath = std::filesystem::path(basePath_) / "manifest.txt";
    std::ifstream file(manifestPath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line.rfind('#', 0) == 0) {
            continue;
        }
        std::filesystem::path assetPath = std::filesystem::path(basePath_) / line;
        std::error_code ec;
        const auto size = std::filesystem::file_size(assetPath, ec);
        if (ec) {
            continue;
        }
        assets_.push_back({line, "user_asset", static_cast<std::size_t>(size)});
    }
    return true;
}

std::vector<AssetDescriptor> Facade::assets() const {
    return assets_;
}

bool Facade::containsAsset(const std::string& relativePath) const {
    for (const auto& asset : assets_) {
        if (asset.path == relativePath) {
            return true;
        }
    }
    return false;
}

}  // namespace ps1
