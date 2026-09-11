#include <jni.h>
#include <string>

#include <android/log.h>

#include "data/disc_layout.h"
#include "engine/native_engine.h"
#include "platform/platform.h"
#include "runtime/psx_native_runtime.h"

namespace {

engine::NativeEngine* g_nativeEngine = nullptr;
runtime::PsxNativeRuntime* g_nativeRuntime = nullptr;

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
Java_com_dinocrisis_nativeandroid_MainActivity_nativeShutdown(JNIEnv* /*env*/, jclass /*clazz*/) {
    if (g_nativeEngine == nullptr) {
        return;
    }

    g_nativeEngine->shutdown();
    delete g_nativeEngine;
    g_nativeEngine = nullptr;
    delete g_nativeRuntime;
    g_nativeRuntime = nullptr;
    platform::logInfo("Java nativeShutdown bridge completed");
}

JNIEXPORT void JNICALL
Java_com_dinocrisis_nativeandroid_MainActivity_nativeOnSurfaceCreated(JNIEnv* /*env*/, jclass /*clazz*/) {
    if (g_nativeEngine == nullptr) {
        return;
    }
    g_nativeEngine->onSurfaceCreated();
    if (g_nativeRuntime != nullptr) {
        g_nativeRuntime->setGraphicsReady(true);
    }
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
    if (g_nativeRuntime != nullptr) {
        g_nativeRuntime->setGraphicsReady(true);
    }
}

JNIEXPORT void JNICALL
Java_com_dinocrisis_nativeandroid_MainActivity_nativeSetGamepadState(
    JNIEnv* /*env*/, jclass /*clazz*/, jint buttons, jfloat leftX, jfloat leftY, jfloat rightX, jfloat rightY) {
    if (g_nativeRuntime == nullptr) {
        return;
    }
    platform::InputState state;
    state.buttons = static_cast<std::uint32_t>(buttons);
    state.leftX = leftX;
    state.leftY = leftY;
    state.rightX = rightX;
    state.rightY = rightY;
    g_nativeRuntime->setControllerState(state);
    platform::logInfo("Gamepad input updated: buttons=" + std::to_string(buttons));
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
        return env->NewStringUTF((std::string("ERROR:") + error).c_str());
    }

    for (auto& track : layout.tracks) {
        if (track.trackNumber == 1) {
            track.filePath = trackOne;
            track.fileName = trackOne;
            platform::readOnlyFileSize(track.filePath, track.fileSize);
        } else if (track.trackNumber == 2) {
            track.filePath = trackTwo;
            track.fileName = trackTwo;
            platform::readOnlyFileSize(track.filePath, track.fileSize);
        }
    }
    if (!data::ExternalGameDataSource::validate(layout, error)) {
        return env->NewStringUTF((std::string("ERROR:") + error).c_str());
    }

    data::PsxExecutableInfo executable;
    const bool executableFound = data::ExternalGameDataSource::analyzePsxExecutable(layout, executable, error);
    if (!executableFound) {
        return env->NewStringUTF((std::string("ERROR:") + error).c_str());
    }
    std::string status = "CUE valid | Track 1 valid | Track 2 valid | Disc validation: success";
    status += " | " + executable.status;
    platform::logInfo("SAF disc validation succeeded: cue=" + cue + ", track1=" + trackOne + ", track2=" + trackTwo);
    return env->NewStringUTF(status.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_dinocrisis_nativeandroid_MainActivity_nativeLoadGame(JNIEnv* env, jclass /*clazz*/, jstring cuePath, jstring trackOnePath, jstring trackTwoPath) {
    auto toString = [env](jstring value) {
        const char* utfValue = env->GetStringUTFChars(value, nullptr);
        std::string result = utfValue == nullptr ? std::string() : std::string(utfValue);
        if (utfValue != nullptr) {
            env->ReleaseStringUTFChars(value, utfValue);
        }
        return result;
    };
    const auto cue = toString(cuePath);
    const auto trackOne = toString(trackOnePath);
    const auto trackTwo = toString(trackTwoPath);
    const jstring validation = Java_com_dinocrisis_nativeandroid_MainActivity_nativeValidateDiscFiles(
        env, nullptr, cuePath, trackOnePath, trackTwoPath);
    const char* result = env->GetStringUTFChars(validation, nullptr);
    const std::string status = result == nullptr ? "ERROR:Unable to read native validation result" : result;
    if (result != nullptr) {
        env->ReleaseStringUTFChars(validation, result);
    }
    env->DeleteLocalRef(validation);
    if (status.rfind("ERROR:", 0) == 0) {
        if (g_nativeEngine != nullptr) {
            g_nativeEngine->setRuntimeReady(false);
        }
        return env->NewStringUTF(status.c_str());
    }
    if (g_nativeEngine == nullptr) {
        return env->NewStringUTF("ERROR:Native engine is not initialized");
    }
    data::DiscLayout layout;
    std::string runtimeError;
    if (!data::ExternalGameDataSource::parseCueFile(cue, layout, runtimeError)) {
        return env->NewStringUTF((std::string("ERROR:") + runtimeError).c_str());
    }
    for (auto& track : layout.tracks) {
        if (track.trackNumber == 1) {
            track.filePath = trackOne;
            track.fileName = trackOne;
            platform::readOnlyFileSize(track.filePath, track.fileSize);
        } else if (track.trackNumber == 2) {
            track.filePath = trackTwo;
            track.fileName = trackTwo;
            platform::readOnlyFileSize(track.filePath, track.fileSize);
        }
    }
    data::PsxExecutableInfo executable;
    if (!data::ExternalGameDataSource::validate(layout, runtimeError)) {
        return env->NewStringUTF((std::string("ERROR:") + runtimeError).c_str());
    }
    if (!data::ExternalGameDataSource::analyzePsxExecutable(layout, executable, runtimeError)) {
        return env->NewStringUTF((std::string("ERROR:") + runtimeError).c_str());
    }
    delete g_nativeRuntime;
    g_nativeRuntime = new runtime::PsxNativeRuntime();
    if (!g_nativeRuntime->initialize(layout, executable, runtimeError)) {
        delete g_nativeRuntime;
        g_nativeRuntime = nullptr;
        return env->NewStringUTF((std::string("ERROR:") + runtimeError).c_str());
    }
    g_nativeEngine->setRuntimeReady(true);
    const std::string runtimeStatus = status + " | " + g_nativeRuntime->status() +
                                      " | Game runtime not yet ready: original MIPS code is not executed";
    return env->NewStringUTF(runtimeStatus.c_str());
}

}  // extern "C"
