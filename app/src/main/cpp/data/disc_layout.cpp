#include "disc_layout.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "../platform/platform.h"

namespace data {

namespace {

std::string trim(const std::string& value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string uppercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return value;
}

std::string extractQuotedFile(const std::string& line) {
    const auto firstQuote = line.find('"');
    if (firstQuote == std::string::npos) {
        return {};
    }
    const auto secondQuote = line.find('"', firstQuote + 1);
    if (secondQuote == std::string::npos) {
        return {};
    }
    return line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

int parseTrackNumber(const std::string& line) {
    const auto pos = uppercase(line).find("TRACK");
    if (pos == std::string::npos) {
        return 0;
    }
    const auto number = trim(line.substr(pos + 5));
    if (number.empty()) {
        return 0;
    }
    try {
        return std::stoi(number.substr(0, 2));
    } catch (...) {
        return 0;
    }
}

std::string parseTrackMode(const std::string& line) {
    const auto pos = uppercase(line).find("TRACK");
    if (pos == std::string::npos) {
        return {};
    }
    const auto rest = trim(line.substr(pos + 5));
    const auto firstSpace = rest.find(' ');
    if (firstSpace == std::string::npos) {
        return {};
    }
    return trim(rest.substr(firstSpace + 1));
}

}  // namespace

bool DiscLayout::hasTrackOne() const {
    for (const auto& track : tracks) {
        if (track.trackNumber == 1) {
            return true;
        }
    }
    return false;
}

bool DiscLayout::hasTrackTwo() const {
    for (const auto& track : tracks) {
        if (track.trackNumber == 2) {
            return true;
        }
    }
    return false;
}

bool DiscLayout::isValid() const {
    return !cueFilePath.empty() && hasTrackOne() && hasTrackTwo();
}

DiscSectorReader::DiscSectorReader(const DiscLayout& layout) : layout_(layout) {}

bool DiscSectorReader::isOpen() const {
    return layout_.isValid();
}

bool DiscSectorReader::readSector(std::uint64_t lba, std::vector<std::uint8_t>& sector) const {
    if (!isOpen()) {
        return false;
    }

    const std::uint64_t sectorSize = 2352ULL;
    const auto trackOne = std::find_if(layout_.tracks.begin(), layout_.tracks.end(), [](const DiscTrack& track) {
        return track.trackNumber == 1;
    });
    if (trackOne == layout_.tracks.end() || trackOne->filePath.empty()) {
        return false;
    }

    std::uint64_t fileSize = 0;
    if (!platform::readOnlyFileSize(trackOne->filePath, fileSize) ||
        fileSize < static_cast<std::uint64_t>(lba * sectorSize) + sectorSize) {
        return false;
    }

    return platform::readOnlyFileRange(trackOne->filePath, lba * sectorSize, sectorSize, sector);
}

bool DiscSectorReader::readRange(std::uint64_t startLba, std::size_t sectorCount, std::vector<std::uint8_t>& buffer) const {
    buffer.clear();
    for (std::size_t i = 0; i < sectorCount; ++i) {
        std::vector<std::uint8_t> sector;
        if (!readSector(startLba + static_cast<std::uint64_t>(i), sector)) {
            return false;
        }
        buffer.insert(buffer.end(), sector.begin(), sector.end());
    }
    return !buffer.empty();
}

bool ExternalGameDataSource::parseCueFile(const std::string& cuePath, DiscLayout& layout, std::string& error) {
    layout = {};
    layout.cueFilePath = cuePath;
    layout.cueFileName = std::filesystem::path(cuePath).filename().string();

    std::ifstream cue(cuePath, std::ios::binary);
    if (!cue.is_open()) {
        error = "Unable to open cue file: " + cuePath;
        return false;
    }

    std::string currentFilePath;
    std::string currentMode;
    int currentTrackNumber = 0;
    std::string line;
    while (std::getline(cue, line)) {
        const auto trimmed = trim(line);
        if (trimmed.empty()) {
            continue;
        }

        const auto command = uppercase(trimmed);
        if (command.rfind("FILE ", 0) == 0) {
            const auto quotedFile = extractQuotedFile(trimmed);
            if (!quotedFile.empty()) {
                currentFilePath = (std::filesystem::path(cuePath).parent_path() / quotedFile).string();
                currentMode = "BINARY";
            }
            continue;
        }

        if (command.rfind("TRACK ", 0) == 0) {
            currentTrackNumber = parseTrackNumber(trimmed);
            currentMode = parseTrackMode(trimmed);
            if (currentTrackNumber > 0 && !currentFilePath.empty()) {
                DiscTrack track;
                track.trackNumber = currentTrackNumber;
                track.fileName = std::filesystem::path(currentFilePath).filename().string();
                track.filePath = currentFilePath;
                track.mode = currentMode;
                track.isAudio = currentMode.find("AUDIO") != std::string::npos;
                std::error_code ec;
                const auto fileSize = std::filesystem::file_size(currentFilePath, ec);
                track.fileSize = ec ? 0ULL : static_cast<std::uint64_t>(fileSize);
                layout.tracks.push_back(track);
            }
        }
    }

    if (layout.tracks.empty()) {
        error = "No valid track entries were found in cue file: " + cuePath;
        return false;
    }

    for (const auto& track : layout.tracks) {
        platform::logInfo("Detected cue track " + std::to_string(track.trackNumber) +
                          " -> " + track.filePath + " (size=" + std::to_string(track.fileSize) + ")");
    }

    return true;
}

std::vector<DiscLayout> ExternalGameDataSource::discover(const std::string& rootPath) {
    std::vector<DiscLayout> layouts;
    std::error_code ec;
    const std::filesystem::path root(rootPath);
    if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
        platform::logError("Game data directory does not exist or is not a directory: " + rootPath);
        return layouts;
    }

    std::vector<std::filesystem::path> cueFiles;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root, ec)) {
        if (entry.is_regular_file(ec) && entry.path().extension() == ".cue") {
            cueFiles.push_back(entry.path());
        }
    }

    for (const auto& cue : cueFiles) {
        std::string error;
        DiscLayout layout;
        if (parseCueFile(cue.string(), layout, error)) {
            if (validate(layout, error)) {
                layouts.push_back(layout);
            } else {
                platform::logError("Cue validation failed for " + cue.string() + ": " + error);
            }
        } else {
            platform::logError(error);
        }
    }

    return layouts;
}

bool ExternalGameDataSource::validate(const DiscLayout& layout, std::string& error) {
    if (!layout.isValid()) {
        error = "Disc layout missing track 1 or track 2 metadata";
        return false;
    }

    bool track1Exists = false;
    bool track2Exists = false;
    bool track1Data = false;
    for (const auto& track : layout.tracks) {
        if (track.trackNumber == 1) {
            track1Exists = std::filesystem::exists(track.filePath);
            track1Data = !track.isAudio;
        }
        if (track.trackNumber == 2) {
            track2Exists = std::filesystem::exists(track.filePath);
        }
    }

    if (!track1Exists || !track2Exists) {
        error = "Track 1 or Track 2 BIN file does not exist";
        return false;
    }

    if (!track1Data) {
        error = "Track 1 must be a data track; Track 2 may be AUDIO for the original disc layout";
        return false;
    }

    const auto hasCompleteSectors = [](const DiscTrack& track) {
        return track.fileSize > 0 && track.fileSize % 2352ULL == 0;
    };
    const auto trackOne = std::find_if(layout.tracks.begin(), layout.tracks.end(), [](const DiscTrack& track) {
        return track.trackNumber == 1;
    });
    const auto trackTwo = std::find_if(layout.tracks.begin(), layout.tracks.end(), [](const DiscTrack& track) {
        return track.trackNumber == 2;
    });
    if (trackOne == layout.tracks.end() || trackTwo == layout.tracks.end() ||
        !hasCompleteSectors(*trackOne) || !hasCompleteSectors(*trackTwo)) {
        error = "Track 1 and Track 2 must contain complete 2352-byte sectors";
        return false;
    }

    error.clear();
    return true;
}

bool ExternalGameDataSource::analyzePsxExecutable(const DiscLayout& layout, PsxExecutableInfo& info, std::string& error) {
    info = {};
    const auto trackOne = std::find_if(layout.tracks.begin(), layout.tracks.end(), [](const DiscTrack& track) {
        return track.trackNumber == 1;
    });
    if (trackOne == layout.tracks.end() || trackOne->filePath.empty()) {
        error = "Track 1 not available for PS-X EXE discovery";
        return false;
    }

    std::uint64_t fileSize = 0;
    if (!platform::readOnlyFileSize(trackOne->filePath, fileSize)) {
        error = "Unable to read Track 1 file size for executable discovery";
        return false;
    }

    constexpr std::uint64_t kSectorSize = 2352ULL;
    constexpr std::uint64_t kHeaderLba = 161099ULL;
    constexpr std::uint64_t kHeaderOffset = kHeaderLba * kSectorSize + 24ULL;
    if (fileSize < kHeaderOffset + 2048ULL) {
        error = "Track 1 is too small to contain the confirmed PS-X EXE header at LBA 161099";
        return false;
    }

    std::vector<std::uint8_t> header(2048);
    if (!platform::readOnlyFileRange(trackOne->filePath, kHeaderOffset, header.size(), header)) {
        error = "Unable to read the PS-X EXE header from Track 1";
        return false;
    }

    const std::string magic(reinterpret_cast<const char*>(header.data()), 8);
    if (magic != "PS-X EXE") {
        error = "Track 1 does not expose a PS-X EXE header at the expected LBA 161099";
        return false;
    }

    const auto read32 = [](const std::vector<std::uint8_t>& data, std::size_t offset) {
        return static_cast<std::uint32_t>(
            (static_cast<std::uint32_t>(data[offset + 0]) << 24) |
            (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
            (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
            static_cast<std::uint32_t>(data[offset + 3]));
    };

    const std::uint32_t loadAddress = read32(header, 0x10);
    const std::uint32_t entryPoint = read32(header, 0x14);
    const std::uint32_t textSize = read32(header, 0x18);

    info.found = true;
    info.fileName = trackOne->fileName;
    info.lba = kHeaderLba;
    info.fileOffset = kHeaderOffset;
    info.loadAddress = loadAddress;
    info.entryPoint = entryPoint;
    info.textSize = textSize;
    info.status = "PS-X EXE header confirmed at Track 1 LBA 161099; load=0x" +
        std::to_string(loadAddress) + ", entry=0x" + std::to_string(entryPoint) +
        ", text=" + std::to_string(textSize);
    error.clear();
    return true;
}

}  // namespace data
