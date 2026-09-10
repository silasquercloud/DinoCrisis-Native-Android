#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace ps1 {

struct AssetDescriptor {
    std::string path;
    std::string type;
    std::size_t size = 0;
};

class Facade {
public:
    explicit Facade(std::string basePath);
    ~Facade() = default;

    bool loadManifest();
    std::vector<AssetDescriptor> assets() const;
    bool containsAsset(const std::string& relativePath) const;

private:
    std::string basePath_;
    std::vector<AssetDescriptor> assets_;
};

}  // namespace ps1
