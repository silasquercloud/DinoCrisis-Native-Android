package com.dinocrisis.nativeandroid;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public final class ExternalGameDataDetector {
    private ExternalGameDataDetector() {
    }

    public static DiscLayout detect(File rootDirectory) throws IOException {
        if (rootDirectory == null || !rootDirectory.exists() || !rootDirectory.isDirectory()) {
            throw new IllegalArgumentException("Game data root must be an existing directory");
        }

        List<File> cueFiles = new ArrayList<>();
        collectCueFiles(rootDirectory, cueFiles);
        if (cueFiles.isEmpty()) {
            throw new IllegalStateException("No CUE file found in " + rootDirectory.getAbsolutePath());
        }

        File cueFile = cueFiles.get(0);
        DiscLayout layout = parseCue(cueFile);
        if (!layout.hasTrackOne() || !layout.hasTrackTwo()) {
            throw new IllegalStateException("CUE file requires both Track 1 and Track 2 BIN files: " + cueFile.getAbsolutePath());
        }

        return layout;
    }

    private static void collectCueFiles(File directory, List<File> result) {
        File[] entries = directory.listFiles();
        if (entries == null) {
            return;
        }
        for (File entry : entries) {
            if (entry.isDirectory()) {
                collectCueFiles(entry, result);
            } else if (entry.getName().toLowerCase().endsWith(".cue")) {
                result.add(entry);
            }
        }
    }

    private static DiscLayout parseCue(File cueFile) throws IOException {
        BufferedReader reader = new BufferedReader(new FileReader(cueFile));
        Map<Integer, String> trackMap = new LinkedHashMap<>();
        String fileReference = null;
        try {
            String line;
            while ((line = reader.readLine()) != null) {
                String trimmed = line.trim();
                if (trimmed.isEmpty()) {
                    continue;
                }
                if (trimmed.regionMatches(true, 0, "FILE ", 0, 5)) {
                    int start = trimmed.indexOf('"');
                    int end = trimmed.lastIndexOf('"');
                    if (start >= 0 && end > start) {
                        fileReference = trimmed.substring(start + 1, end);
                    }
                    continue;
                }
                if (trimmed.regionMatches(true, 0, "TRACK ", 0, 6)) {
                    String[] parts = trimmed.split("\\s+");
                    if (parts.length >= 2) {
                        try {
                            int trackNumber = Integer.parseInt(parts[1].substring(0, 2));
                            if (fileReference != null) {
                                trackMap.put(trackNumber, new File(cueFile.getParentFile(), fileReference).getAbsolutePath());
                            }
                        } catch (NumberFormatException ignored) {
                            // Intentionally ignored for invalid or non-standard CUE metadata.
                        }
                    }
                }
            }
        } finally {
            reader.close();
        }

        if (trackMap.isEmpty()) {
            throw new IllegalStateException("No readable tracks were found in cue file: " + cueFile.getAbsolutePath());
        }

        DiscLayout layout = new DiscLayout();
        layout.cueFile = cueFile;
        layout.trackFiles = new LinkedHashMap<>();
        for (Map.Entry<Integer, String> entry : trackMap.entrySet()) {
            File candidate = new File(entry.getValue());
            if (candidate.exists()) {
                layout.trackFiles.put(entry.getKey(), candidate);
            }
        }
        return layout;
    }

    public static final class DiscLayout {
        private File cueFile;
        private Map<Integer, File> trackFiles;

        public File getCueFile() {
            return cueFile;
        }

        public boolean hasTrackOne() {
            return trackFiles != null && trackFiles.containsKey(1);
        }

        public boolean hasTrackTwo() {
            return trackFiles != null && trackFiles.containsKey(2);
        }

        public File getTrackOneFile() {
            return trackFiles != null ? trackFiles.get(1) : null;
        }

        public File getTrackTwoFile() {
            return trackFiles != null ? trackFiles.get(2) : null;
        }

        public List<File> getTrackFiles() {
            if (trackFiles == null) {
                return Collections.emptyList();
            }
            return new ArrayList<>(trackFiles.values());
        }
    }
}
