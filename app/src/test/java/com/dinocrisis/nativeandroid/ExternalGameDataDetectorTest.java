package com.dinocrisis.nativeandroid;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import org.junit.Test;

public class ExternalGameDataDetectorTest {
    @Test
    public void detectsCueAndTrackBinsFromSyntheticDiskLayout() throws IOException {
        File tempRoot = Files.createTempDirectory("dinocrisis-game-data").toFile();
        File discDir = new File(tempRoot, "disc");
        assertTrue(discDir.mkdirs());

        File cueFile = new File(discDir, "game.cue");
        File track1 = new File(discDir, "game_track1.bin");
        File track2 = new File(discDir, "game_track2.bin");

        writeFile(cueFile,
            "FILE \"game_track1.bin\" BINARY\n"
                + "  TRACK 01 MODE1/2352\n"
                + "FILE \"game_track2.bin\" BINARY\n"
                + "  TRACK 02 MODE1/2352\n");
        writeFile(track1, "TRACK1-DUMMY-BYTES-123456");
        writeFile(track2, "TRACK2-DUMMY-BYTES-654321");

        ExternalGameDataDetector.DiscLayout layout = ExternalGameDataDetector.detect(discDir);
        assertNotNull(layout);
        assertTrue(layout.hasTrackOne());
        assertTrue(layout.hasTrackTwo());
        assertNotNull(layout.getTrackOneFile());
        assertNotNull(layout.getTrackTwoFile());
        assertEquals(cueFile.getAbsolutePath(), layout.getCueFile().getAbsolutePath());
    }

    @Test(expected = IllegalStateException.class)
    public void rejectsCueWithoutTrackTwo() throws IOException {
        File tempRoot = Files.createTempDirectory("dinocrisis-invalid").toFile();
        File discDir = new File(tempRoot, "disc");
        assertTrue(discDir.mkdirs());

        File cueFile = new File(discDir, "partial.cue");
        File track1 = new File(discDir, "partial_track1.bin");
        writeFile(cueFile,
            "FILE \"partial_track1.bin\" BINARY\n"
                + "  TRACK 01 MODE1/2352\n");
        writeFile(track1, "ONLY-TRACK-1");

        ExternalGameDataDetector.detect(discDir);
    }

    private static void writeFile(File file, String content) throws IOException {
        FileWriter writer = new FileWriter(file, false);
        writer.write(content);
        writer.close();
    }
}
