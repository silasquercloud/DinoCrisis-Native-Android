#include "platform.h"

#include <GLES3/gl3.h>

namespace platform {
void clearGraphics(float red, float green, float blue) {
    glClearColor(red, green, blue, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
}
}  // namespace platform