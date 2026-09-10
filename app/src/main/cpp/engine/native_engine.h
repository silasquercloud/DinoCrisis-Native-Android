#pragma once

#include <string>

#include <GLES3/gl3.h>

namespace engine {

class NativeEngine {
public:
    NativeEngine() = default;
    ~NativeEngine();

    void initialize(const std::string& gameDataPath);
    void shutdown();

    void onSurfaceCreated();
    void onSurfaceChanged(int width, int height);
    void onDrawFrame();

private:
    std::string gameDataPath_;
    bool initialized_ = false;
    bool surfaceCreated_ = false;
    int width_ = 0;
    int height_ = 0;

    GLuint program_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
};

}  // namespace engine
