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
Java_com_dinocrisis_nativeandroid_MainActivity_nativeShutdown(JNIEnv* /*env*/, jclass /*clazz*/) {
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
    std::string status = "CUE valid | Track 1 valid | Track 2 valid | Disc validation: success";
    if (executableFound) {
        status += " | " + executable.status;
    } else {
        status += " | PS-X EXE discovery: " + error;
    }
    platform::logInfo("SAF disc validation succeeded: cue=" + cue + ", track1=" + trackOne + ", track2=" + trackTwo);
    return env->NewStringUTF(status.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_dinocrisis_nativeandroid_MainActivity_nativeLoadGame(JNIEnv* env, jclass /*clazz*/, jstring cuePath, jstring trackOnePath, jstring trackTwoPath) {
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
    g_nativeEngine->setRuntimeReady(true);
    const std::string runtimeStatus = status + " | Game runtime not yet ready";
    return env->NewStringUTF(runtimeStatus.c_str());
}

}  // extern "C"
