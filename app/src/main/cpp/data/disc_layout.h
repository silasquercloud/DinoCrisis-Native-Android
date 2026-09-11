#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace data {

struct DiscTrack {
    int trackNumber = 0;
    std::string fileName;
    std::string filePath;
    std::string mode;
    bool isAudio = false;
    std::uint64_t fileSize = 0;
};

struct DiscLayout {
    std::string cueFilePath;
    std::string cueFileName;
    std::vector<DiscTrack> tracks;

    bool hasTrackOne() const;
    bool hasTrackTwo() const;
    bool isValid() const;
};

struct PsxExecutableInfo {
    bool found = false;
    std::string volumeId;
    std::string fileName;
    std::uint64_t lba = 0;
    std::uint64_t fileOffset = 0;
    std::uint32_t loadAddress = 0;
    std::uint32_t entryPoint = 0;
    std::uint32_t textSize = 0;
    std::string status;
};

class DiscSectorReader {
public:
    explicit DiscSectorReader(const DiscLayout& layout);
    ~DiscSectorReader() = default;

    bool isOpen() const;
    bool readSector(std::uint64_t lba, std::vector<std::uint8_t>& sector) const;
    bool readRange(std::uint64_t startLba, std::size_t sectorCount, std::vector<std::uint8_t>& buffer) const;

private:
    DiscLayout layout_;
};

class ExternalGameDataSource {
public:
    static std::vector<DiscLayout> discover(const std::string& rootPath);
    static bool validate(const DiscLayout& layout, std::string& error);
    static bool parseCueFile(const std::string& cuePath, DiscLayout& layout, std::string& error);
    static bool analyzePsxExecutable(const DiscLayout& layout, PsxExecutableInfo& info, std::string& error);
};

}  // namespace data
