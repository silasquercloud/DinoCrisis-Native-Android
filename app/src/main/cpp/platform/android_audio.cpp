#include "platform.h"

#include <android/log.h>

namespace platform {
void initializeAudio() {
    logInfo("Audio platform placeholder initialized");
}

void shutdownAudio() {
    logInfo("Audio platform placeholder shutdown");
}
}  // namespace platform