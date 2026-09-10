#include <jni.h>
#include <string>

#include <android/log.h>

#include "data/disc_layout.h"
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

JNIEXPORT jstring JNICALL
Java_com_dinocrisis_nativeandroid_MainActivity_nativeValidateDiscFiles(JNIEnv* env, jclass /*clazz*/, jstring cuePath, jstring trackOnePath, jstring trackTwoPath) {
    auto toString = [env](jstring value) {
        const char* utfValue = env->GetStringUTFChars(value, nullptr);
        std::string result = utfValue == nullptr ? std::string() : std::string(utfValue);
        if (utfValue != nullptr) {
            env->ReleaseStringUTFChars(value, utfValue);
        }
        return result;
    };

    data::DiscLayout layout;
    std::string error;
    const auto cue = toString(cuePath);
    const auto trackOne = toString(trackOnePath);
    const auto trackTwo = toString(trackTwoPath);
    if (!data::ExternalGameDataSource::parseCueFile(cue, layout, error)) {
        return env->NewStringUTF(error.c_str());
    }

    for (auto& track : layout.tracks) {
        if (track.trackNumber == 1) {
            track.filePath = trackOne;
            track.fileName = trackOne;
        } else if (track.trackNumber == 2) {
            track.filePath = trackTwo;
            track.fileName = trackTwo;
        }
    }
    if (!data::ExternalGameDataSource::validate(layout, error)) {
        return env->NewStringUTF(error.c_str());
    }

    platform::logInfo("SAF disc validation succeeded: cue=" + cue + ", track1=" + trackOne + ", track2=" + trackTwo);
    return nullptr;
}

}  // extern "C"
