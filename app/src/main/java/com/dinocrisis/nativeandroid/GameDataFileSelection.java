package com.dinocrisis.nativeandroid;

import java.io.File;

public final class GameDataFileSelection {
    private GameDataFileSelection() {
    }

    public static String requireCueName(String name) {
        if (name == null || !name.toLowerCase().endsWith(".cue")) {
            throw new IllegalArgumentException("Select a CUE file for the disc layout");
        }
        return name;
    }

    public static String requireBinName(String name, String trackLabel) {
        if (name == null || !name.toLowerCase().endsWith(".bin")) {
            throw new IllegalArgumentException("Select a BIN file for " + trackLabel);
        }
        return name;
    }

    public static void requireNonEmpty(File file, String trackLabel) {
        if (file == null || file.length() == 0) {
            throw new IllegalArgumentException("Selected BIN file is empty: " + trackLabel);
        }
    }
}
