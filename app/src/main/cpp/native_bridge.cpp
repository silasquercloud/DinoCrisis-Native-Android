#include <jni.h>
#include <string>

#include <android/log.h>

#include "engine/native_engine.h"
#include "platform/platform.h"

namespace {

engine::NativeEngine* g_nativeEngine = nullptr;

}  // namespace

extern "C" {

JNIEXPORT void JNICALL
Java_com_dinocrisis_nativeandroid_MainActivity_nativeInitialize(JNIEnv* env, jclass /*clazz*/, jstring gameDataPath) {
    if (g_nativeEngine != nullptr) {
        return;
    }

    const char* utfPath = env->GetStringUTFChars(gameDataPath, nullptr);
    if (utfPath == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, "DinoCrisisNative", "nativeInitialize received null gameDataPath");
        return;
    }

    std::string path(utfPath);
    env->ReleaseStringUTFChars(gameDataPath, utfPath);

    g_nativeEngine = new engine::NativeEngine();
    g_nativeEngine->initialize(path);
    platform::logInfo("Java nativeInitialize bridge completed");
}

JNIEXPORT void JNICALL
Java_com_dinocrisis_nativeandroid_MainActivity_nativeShutdown(JNIEnv* env, jclass /*clazz*/) {
    if (g_nativeEngine == nullptr) {
        return;
    }

    g_nativeEngine->shutdown();
    delete g_nativeEngine;
    g_nativeEngine = nullptr;
    platform::logInfo("Java nativeShutdown bridge completed");
}

JNIEXPORT void JNICALL
Java_com_dinocrisis_nativeandroid_MainActivity_nativeOnSurfaceCreated(JNIEnv* /*env*/, jclass /*clazz*/) {
    if (g_nativeEngine == nullptr) {
        return;
    }
    g_nativeEngine->onSurfaceCreated();
}

JNIEXPORT void JNICALL
Java_com_dinocrisis_nativeandroid_MainActivity_nativeOnSurfaceChanged(JNIEnv* /*env*/, jclass /*clazz*/, jint width, jint height) {
    if (g_nativeEngine == nullptr) {
        return;
    }
    g_nativeEngine->onSurfaceChanged(static_cast<int>(width), static_cast<int>(height));
}

JNIEXPORT void JNICALL
Java_com_dinocrisis_nativeandroid_MainActivity_nativeOnDrawFrame(JNIEnv* /*env*/, jclass /*clazz*/) {
    if (g_nativeEngine == nullptr) {
        return;
    }
    g_nativeEngine->onDrawFrame();
}

}  // extern "C"
