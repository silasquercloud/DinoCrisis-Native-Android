#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../data/disc_layout.h"
#include "../platform/platform.h"

namespace runtime {

struct RuntimeDiagnostics {
    bool memoryInitialized = false;
    bool cdInitialized = false;
    bool executableDiscovered = false;
    bool graphicsReady = false;
    bool audioReady = false;
    bool controllerReady = false;
    bool executableExecuted = false;
    std::string status;
};

class PsxNativeRuntime {
public:
    ~PsxNativeRuntime();
    bool initialize(const data::DiscLayout& layout, const data::PsxExecutableInfo& executable, std::string& error);
    void setGraphicsReady(bool ready);
    void setControllerState(const platform::InputState& state);
    const RuntimeDiagnostics& diagnostics() const;
    std::string status() const;

private:
    std::vector<std::uint8_t> nativeMemory_;
    data::DiscSectorReader* sectorReader_ = nullptr;
    data::PsxExecutableInfo executable_;
    platform::InputState input_;
    RuntimeDiagnostics diagnostics_;
};

}  // namespace runtime