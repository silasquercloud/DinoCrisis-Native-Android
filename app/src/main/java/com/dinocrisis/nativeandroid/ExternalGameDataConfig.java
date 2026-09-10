package com.dinocrisis.nativeandroid;

import android.content.Context;
import java.io.File;

public final class ExternalGameDataConfig {
    public static final String GAME_DATA_DIR_NAME = "game_data";

    private ExternalGameDataConfig() {
    }

    public static File defaultDirectory(Context context) {
        File externalRoot = context.getExternalFilesDir(null);
        if (externalRoot == null) {
            throw new IllegalStateException("External files directory is unavailable on this device");
        }
        File gameDataDir = new File(externalRoot, GAME_DATA_DIR_NAME);
        if (!gameDataDir.exists() && !gameDataDir.mkdirs()) {
            throw new IllegalStateException("Unable to create game data directory: " + gameDataDir.getAbsolutePath());
        }
        return gameDataDir;
    }

    public static File resolveDirectory(Context context, String overridePath) {
        if (overridePath != null && !overridePath.trim().isEmpty()) {
            File resolved = new File(overridePath);
            if (!resolved.exists() && !resolved.mkdirs()) {
                throw new IllegalStateException("Unable to create configured game data directory: " + resolved.getAbsolutePath());
            }
            return resolved;
        }
        return defaultDirectory(context);
    }
}
