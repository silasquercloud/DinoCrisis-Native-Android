# Dino Crisis Native Android

This repository is the foundation for a genuine native Android port project for the original PlayStation 1 game Dino Crisis (1999). It is intentionally not an emulator, and it does not include any commercial game ROM, disc image, extracted assets, textures, models, or other copyrighted data.

## Project scope

The current goal is only the native Android foundation:

- Android Gradle project and NDK/CMake build
- arm64-v8a APK/AAB target setup
- C++ runtime and platform abstraction layer
- Java Android glue activity
- OpenGL ES 3 verification renderer
- PS1 compatibility facade boundary without implementing a CPU emulator
- Data-loading interface that expects the user's legally supplied game files to live locally

This repository contains only original source code, reverse-engineering scaffolding, and legally redistributable build tooling.

## Architecture

- `app/src/main/java/com/dinocrisis/nativeandroid/MainActivity.java` loads the native library and owns the Android lifecycle.
- `app/src/main/cpp/native_bridge.cpp` exposes the Java-to-C++ JNI bridge.
- `app/src/main/cpp/engine/native_engine.cpp` contains the native engine and the initial OpenGL ES 3 renderer test screen.
- `app/src/main/cpp/platform/` contains the platform abstractions for filesystem, input, audio, timing, memory, and graphics.
- `app/src/main/cpp/data/` hosts the user data access layer and the local game-data root contract.
- `app/src/main/cpp/ps1/` contains a compatibility/facade boundary for PS1-oriented data access without emulating the MIPS CPU.

## How the native Android build works

The Android app is built with the Android Gradle Plugin and CMake through the NDK. The native library is named `dinocrisis_native` and is loaded at runtime by `MainActivity`. The CMake configuration compiles the library for `arm64-v8a` and links it to the Android logging and OpenGL ES libraries.

The native engine initializes a minimal GLES 3 shader program and draws a colored triangle on a full-screen surface so we can verify that the Android renderer, shader pipeline, and JNI bridge are working correctly.

## Where the user game data should be placed

The app expects a local external data root under the Android app sandbox:

- `Android/data/com.dinocrisis.nativeandroid/files/game_data/`

This directory is created automatically by `ExternalGameDataConfig` on first run. The application also accepts a configured override path if it is set explicitly in the future.

### Selecting files on an Android phone

The app uses Android's Storage Access Framework. The user does not need to browse to or manually copy files into `Android/data`:

1. Open the app and tap **Select Dino Crisis Disc**.
2. In the Android document picker, select the legally owned `.cue` file.
3. Select the corresponding Track 1 `.bin` file when prompted.
4. Select the corresponding Track 2 `.bin` file when prompted.
5. The app copies only those selected files into its private external data directory, rewrites the local CUE references to the imported filenames, and validates the complete set through the native bridge.

The app reports clear errors for cancelled selections, wrong file extensions, missing tracks, empty BIN files, unreadable files, invalid CUE metadata, and audio tracks supplied where data tracks are required. The native reader exposes read-only sector access; it does not write to or modify the selected source documents.

Expected disc layout for a legal local PS1 disc source:

```text
Android/data/com.dinocrisis.nativeandroid/files/game_data/
├── dino_crisis/
│   ├── disc1.cue
│   ├── disc1_track1.bin
│   ├── disc1_track2.bin
│   └── README.txt     # optional local notes, not a game asset
└── raw/
    └── (optional local copies or extracted metadata only)
```

For a typical original PS1 disc, the loader expects a CUE file plus at least Track 1 and Track 2 BIN files. These files remain user-owned and local only; they are not embedded, copied, or tracked in the repository. The loader is read-only and does not emulate the PS1 CPU.

## Android SAF import flow

The app provides a `Select Dino Crisis Disc` action. Use it to select the three legally owned files through Android's document picker:

1. Select the `.cue` file.
2. Select the matching Track 1 `.bin` file when prompted.
3. Select the matching Track 2 `.bin` file when prompted.

The app copies the selected streams into its private external directory as `disc.cue`, `track1.bin`, and `track2.bin`. It rewrites the local CUE references to those private filenames, then passes the resulting paths through JNI to the native C++ validator. The original files are never modified and no game data is packaged in the APK.

The native layer logs each detected track, validates that both BIN files exist, and exposes read-only sector/range access through the platform filesystem abstraction. Missing files, unreadable documents, malformed CUE metadata, and empty BIN selections are shown as game-data errors in the Activity.

## Git exclusion policy

The repository includes a strict `.gitignore` to prevent accidental commits of:

- ROM/disc images such as `.bin`, `.cue`, `.iso`, `.img`, `.mdf`, `.nrg`
- extracted commercial asset directories
- local game data directories under `game_data/`
- generated build artifacts and native cache files

The data is intentionally excluded from Git and must remain local only.

## Build instructions

Requirements:

- Android SDK with API 35
- Android NDK
- CMake 3.22.1+
- JDK 17+

From the repository root:

```bash
export ANDROID_HOME=/path/to/Android/Sdk
export ANDROID_SDK_ROOT=/path/to/Android/Sdk
./gradlew :app:assembleDebug
```

The resulting APK appears at:

```text
app/build/outputs/apk/debug/app-debug.apk
```

## Legal notice

This project is a reverse-engineering and porting foundation only. It is not a ROM, ISO, asset dump, or copyrighted content redistributor. All game data must be supplied by the user and used in compliance with their legal rights and local laws.
