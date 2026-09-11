#include "psx_native_runtime.h"

#include <algorithm>
#include <memory>

namespace runtime {

PsxNativeRuntime::~PsxNativeRuntime() {
    delete sectorReader_;
    sectorReader_ = nullptr;
}

bool PsxNativeRuntime::initialize(const data::DiscLayout& layout,
                                  const data::PsxExecutableInfo& executable,
                                  std::string& error) {
    diagnostics_ = {};
    executable_ = executable;
    if (!executable.found || executable.textSize == 0) {
        error = "PS-X EXE metadata is incomplete; native runtime cannot initialize";
        return false;
    }

    constexpr std::size_t kMinimumNativeMemory = 2U * 1024U * 1024U;
    const std::uint64_t declaredMemory = static_cast<std::uint64_t>(executable.textSize) + executable.bssSize;
    const std::size_t memorySize = static_cast<std::size_t>(std::max<std::uint64_t>(kMinimumNativeMemory, declaredMemory));
    nativeMemory_.assign(memorySize, 0);
    diagnostics_.memoryInitialized = true;

    auto reader = std::make_unique<data::DiscSectorReader>(layout);
    if (!reader->isOpen()) {
        error = "Read-only CD sector reader could not open the validated disc layout";
        return false;
    }
    std::vector<std::uint8_t> headerSector;
    if (!reader->readSector(executable.lba, headerSector) || headerSector.size() != 2352) {
        error = "Validated Track 1 could not be read through the sector interface";
        return false;
    }
    sectorReader_ = reader.release();
    diagnostics_.cdInitialized = true;
    diagnostics_.executableDiscovered = true;
    diagnostics_.audioReady = false;
    diagnostics_.controllerReady = true;
    diagnostics_.status = status();
    error.clear();
    return true;
}

void PsxNativeRuntime::setGraphicsReady(bool ready) {
    diagnostics_.graphicsReady = ready;
    diagnostics_.status = status();
}

void PsxNativeRuntime::setControllerState(const platform::InputState& state) {
    input_ = state;
    diagnostics_.controllerReady = true;
}

const RuntimeDiagnostics& PsxNativeRuntime::diagnostics() const {
    return diagnostics_;
}

std::string PsxNativeRuntime::status() const {
    std::string result = "Native compatibility runtime initialized";
    result += " | memory=ready";
    result += " | cd=sector-reader-ready";
    result += " | executable=discovered";
    result += diagnostics_.graphicsReady ? " | graphics=ready" : " | graphics=waiting";
    result += diagnostics_.audioReady ? " | audio=ready" : " | audio=not-implemented";
    result += diagnostics_.controllerReady ? " | gamepad=ready" : " | gamepad=waiting";
    result += " | execution=not-started";
    return result;
}

}  // namespace runtime