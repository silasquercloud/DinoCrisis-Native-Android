#include "native_engine.h"

#include <android/log.h>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <vector>

#include "../data/game_data.h"
#include "../platform/platform.h"

namespace {

constexpr char kLogTag[] = "DinoCrisisNative";

static const char* kVertexShader = R"(
    #version 300 es
    precision highp float;
    layout(location = 0) in vec2 aPosition;
    layout(location = 1) in vec4 aColor;
    out vec4 vColor;
    void main() {
        gl_Position = vec4(aPosition, 0.0, 1.0);
        vColor = aColor;
    }
)";

static const char* kFragmentShader = R"(
    #version 300 es
    precision mediump float;
    in vec4 vColor;
    out vec4 outColor;
    void main() {
        outColor = vColor;
    }
)";

bool compileShader(GLenum type, const char* source, GLuint* shader) {
    *shader = glCreateShader(type);
    if (*shader == 0) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "glCreateShader failed for type %d", type);
        return false;
    }
    glShaderSource(*shader, 1, &source, nullptr);
    glCompileShader(*shader);

    GLint status = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length = 0;
        glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(static_cast<size_t>(length > 0 ? length : 1));
        glGetShaderInfoLog(*shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Shader compile error: %s", log.data());
        glDeleteShader(*shader);
        *shader = 0;
        return false;
    }
    return true;
}

bool linkProgram(GLuint vertexShader, GLuint fragmentShader, GLuint* program) {
    *program = glCreateProgram();
    if (*program == 0) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "glCreateProgram failed");
        return false;
    }

    glAttachShader(*program, vertexShader);
    glAttachShader(*program, fragmentShader);
    glBindAttribLocation(*program, 0, "aPosition");
    glBindAttribLocation(*program, 1, "aColor");
    glLinkProgram(*program);

    GLint status = 0;
    glGetProgramiv(*program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint length = 0;
        glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(static_cast<size_t>(length > 0 ? length : 1));
        glGetProgramInfoLog(*program, static_cast<GLsizei>(log.size()), nullptr, log.data());
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "Program link error: %s", log.data());
        glDeleteProgram(*program);
        *program = 0;
        return false;
    }
    return true;
}

}  // namespace

namespace engine {

NativeEngine::~NativeEngine() {
    shutdown();
}

void NativeEngine::initialize(const std::string& gameDataPath) {
    gameDataPath_ = gameDataPath;
    if (initialized_) {
        return;
    }

    platform::setGameDataPath(gameDataPath_);
    platform::logInfo("Native engine initialized: " + gameDataPath_);
    if (!data::GameData::initialize(gameDataPath_)) {
        platform::logError("GameData initialization failed; install or supply legal local game data.");
    }
    initialized_ = true;
}

void NativeEngine::setRuntimeReady(bool ready) {
    runtimeReady_ = ready;
    platform::logInfo(ready ? "Runtime data accepted; renderer may initialize" :
                              "Runtime data rejected; renderer remains disabled");
}

void NativeEngine::shutdown() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    surfaceCreated_ = false;
    runtimeReady_ = false;
    initialized_ = false;
    platform::logInfo("Native engine shutdown complete");
}

void NativeEngine::onSurfaceCreated() {
    if (!runtimeReady_) {
        platform::logInfo("OpenGL ES surface created while runtime is waiting for valid game data");
        return;
    }
    glClearColor(0.08f, 0.10f, 0.12f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
    if (!compileShader(GL_VERTEX_SHADER, kVertexShader, &vertexShader) ||
        !compileShader(GL_FRAGMENT_SHADER, kFragmentShader, &fragmentShader) ||
        !linkProgram(vertexShader, fragmentShader, &program_)) {
        platform::logError("Failed to create native OpenGL test program");
        if (vertexShader != 0) glDeleteShader(vertexShader);
        if (fragmentShader != 0) glDeleteShader(fragmentShader);
        return;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    struct Vertex {
        GLfloat x;
        GLfloat y;
        GLfloat r;
        GLfloat g;
        GLfloat b;
        GLfloat a;
    };

    static const Vertex kVertices[] = {
        {-0.6f, -0.4f, 1.0f, 0.2f, 0.3f, 1.0f},
        { 0.6f, -0.4f, 0.2f, 1.0f, 0.3f, 1.0f},
        { 0.0f,  0.7f, 0.3f, 0.5f, 1.0f, 1.0f},
    };

    glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, x)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, r)));

    glBindVertexArray(0);
    surfaceCreated_ = true;
    platform::logInfo("OpenGL ES 3 test screen initialized successfully");
}

void NativeEngine::onSurfaceChanged(int width, int height) {
    width_ = width;
    height_ = height;
    if (height <= 0) {
        height = 1;
    }
    glViewport(0, 0, width, height);
    platform::logInfo("Surface changed to " + std::to_string(width) + "x" + std::to_string(height));
}

void NativeEngine::onDrawFrame() {
    if (runtimeReady_ && !surfaceCreated_) {
        onSurfaceCreated();
    }
    if (!surfaceCreated_ || program_ == 0) {
        return;
    }

    glClearColor(0.05f, 0.06f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

}  // namespace engine
