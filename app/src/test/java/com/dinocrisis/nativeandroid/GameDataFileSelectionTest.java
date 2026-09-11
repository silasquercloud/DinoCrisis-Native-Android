package com.dinocrisis.nativeandroid;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import org.junit.Test;

public class GameDataFileSelectionTest {
    @Test
    public void acceptsIndependentCueAndTrackSelections() throws IOException {
        assertEquals("disc.CUE", GameDataFileSelection.requireCueName("disc.CUE"));
        assertEquals("track1.BIN", GameDataFileSelection.requireBinName("track1.BIN", "track1.bin"));

        File selected = Files.createTempFile("track2", ".bin").toFile();
        Files.write(selected.toPath(), new byte[]{1, 2, 3});
        GameDataFileSelection.requireNonEmpty(selected, "track2.bin");
    }

    @Test
    public void rejectsWrongFileTypes() {
        assertInvalid(() -> GameDataFileSelection.requireCueName("disc.bin"));
        assertInvalid(() -> GameDataFileSelection.requireBinName("track1.iso", "track1.bin"));
    }

    private static void assertInvalid(Runnable action) {
        try {
            action.run();
            fail("Expected invalid file selection");
        } catch (IllegalArgumentException expected) {
            // Expected validation failure.
        }
    }
}