#include "platform.h"

#include <cstdlib>

namespace platform {
void* allocate(std::size_t size) {
    return std::malloc(size);
}

void release(void* memory) {
    std::free(memory);
}
}  // namespace platform