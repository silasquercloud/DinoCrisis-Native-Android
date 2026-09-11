package com.dinocrisis.nativeandroid;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.Gravity;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.database.Cursor;
import android.provider.OpenableColumns;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public final class MainActivity extends Activity {
    private static final int REQUEST_CUE = 100;
    private static final int REQUEST_TRACK_ONE = 101;
    private static final int REQUEST_TRACK_TWO = 102;

    static { System.loadLibrary("dinocrisis_native"); }

    private static final String ALPHA_RUNTIME_STATUS =
        "Alpha runtime ready: waiting for legal Dino Crisis disc data";
    private NativeSurfaceView surfaceView;
    private TextView dataStatus;
    private File gameDataDirectory;
    private File cueFile;
    private File trackOneFile;

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

        gameDataDirectory = ExternalGameDataConfig.resolveDirectory(this, null);
        nativeInitialize(gameDataDirectory.getAbsolutePath());
        surfaceView = new NativeSurfaceView();

        FrameLayout root = new FrameLayout(this);
        root.addView(surfaceView, new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.MATCH_PARENT));

        Button selectDisc = new Button(this);
        selectDisc.setText("Select Dino Crisis Disc");
        selectDisc.setOnClickListener(view -> beginCueSelection());
        FrameLayout.LayoutParams buttonParams = new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        buttonParams.gravity = Gravity.TOP | Gravity.CENTER_HORIZONTAL;
        root.addView(selectDisc, buttonParams);

        dataStatus = new TextView(this);
        dataStatus.setText(ALPHA_RUNTIME_STATUS);
        dataStatus.setTextColor(0xFFFFFFFF);
        dataStatus.setBackgroundColor(0xAA000000);
        dataStatus.setPadding(20, 12, 20, 12);
        FrameLayout.LayoutParams statusParams = new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        statusParams.gravity = Gravity.BOTTOM;
        root.addView(dataStatus, statusParams);
        setContentView(root);
    }

    @Override protected void onPause() { super.onPause(); surfaceView.onPause(); }
    @Override protected void onResume() { super.onResume(); if (surfaceView != null) surfaceView.onResume(); }
    @Override protected void onDestroy() { nativeShutdown(); super.onDestroy(); }

    private final class NativeSurfaceView extends GLSurfaceView {
        NativeSurfaceView() {
            super(MainActivity.this);
            setEGLContextClientVersion(3);
            setRenderer(new Renderer());
            setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
        }
        private final class Renderer implements GLSurfaceView.Renderer {
            @Override public void onSurfaceCreated(javax.microedition.khronos.opengles.GL10 gl, javax.microedition.khronos.egl.EGLConfig config) { nativeOnSurfaceCreated(); }
            @Override public void onSurfaceChanged(javax.microedition.khronos.opengles.GL10 gl, int width, int height) { nativeOnSurfaceChanged(width, height); }
            @Override public void onDrawFrame(javax.microedition.khronos.opengles.GL10 gl) { nativeOnDrawFrame(); }
        }
    }

    private static native void nativeInitialize(String gameDataPath);
    private static native void nativeShutdown();
    private static native void nativeOnSurfaceCreated();
    private static native void nativeOnSurfaceChanged(int width, int height);
    private static native void nativeOnDrawFrame();
    private static native String nativeValidateDiscFiles(String cuePath, String trackOnePath, String trackTwoPath);

    private static String formatRuntimeStatus(String phase, String value) {
        return phase + ": " + value;
    }

    private void beginCueSelection() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, REQUEST_CUE);
    }

    @Override protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            showDataError("Disc selection cancelled");
            return;
        }

        try {
            if (requestCode == REQUEST_CUE) {
                cueFile = importCue(data.getData());
                showDataStatus("CUE selected. Select Track 1 BIN.");
                beginBinarySelection(REQUEST_TRACK_ONE);
            } else if (requestCode == REQUEST_TRACK_ONE) {
                trackOneFile = importBinary(data.getData(), "track1.bin");
                showDataStatus("Track 1 selected. Select Track 2 BIN.");
                beginBinarySelection(REQUEST_TRACK_TWO);
            } else if (requestCode == REQUEST_TRACK_TWO) {
                File trackTwoFile = importBinary(data.getData(), "track2.bin");
                String runtimeStatus = nativeValidateDiscFiles(
                    cueFile.getAbsolutePath(), trackOneFile.getAbsolutePath(), trackTwoFile.getAbsolutePath());
                if (runtimeStatus != null && !runtimeStatus.isEmpty() && runtimeStatus.startsWith("ERROR:")) {
                    showDataError(runtimeStatus.substring("ERROR:".length()));
                    return;
                }
                if (runtimeStatus == null || runtimeStatus.isEmpty()) {
                    runtimeStatus = "Disc validated: CUE + Track 1 BIN + Track 2 BIN; runtime bootstrapped";
                }
                showDataStatus(runtimeStatus);
            }
        } catch (IOException | IllegalArgumentException error) {
            showDataError(error.getMessage() == null ? "Unable to import selected file" : error.getMessage());
        }
    }

    private void beginBinarySelection(int requestCode) {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, requestCode);
    }

    private File importCue(Uri source) throws IOException {
        String displayName = displayName(source);
        if (displayName != null && !displayName.toLowerCase().endsWith(".cue")) {
            throw new IllegalArgumentException("Select a CUE file for the disc layout");
        }
        StringBuilder canonicalCue = new StringBuilder();
        int trackReference = 0;
        try (InputStream input = getContentResolver().openInputStream(source)) {
            if (input == null) {
                throw new IOException("Unable to open selected CUE file");
            }
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(input, StandardCharsets.UTF_8))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    String trimmed = line.trim();
                    if (trimmed.regionMatches(true, 0, "FILE ", 0, 5)) {
                        trackReference++;
                        if (trackReference == 1 || trackReference == 2) {
                            int firstQuote = line.indexOf('"');
                            int secondQuote = line.indexOf('"', firstQuote + 1);
                            if (firstQuote >= 0 && secondQuote > firstQuote) {
                                String replacement = trackReference == 1 ? "track1.bin" : "track2.bin";
                                line = line.substring(0, firstQuote + 1) + replacement + line.substring(secondQuote);
                            }
                        }
                    }
                    canonicalCue.append(line).append('\n');
                }
            }
        }
        if (trackReference < 2) {
            throw new IllegalArgumentException("Selected CUE must reference Track 1 and Track 2");
        }
        File target = new File(gameDataDirectory, "disc.cue");
        writeBytes(target, canonicalCue.toString().getBytes(StandardCharsets.UTF_8));
        return target;
    }

    private File importBinary(Uri source, String targetName) throws IOException {
        String displayName = displayName(source);
        if (displayName != null && !displayName.toLowerCase().endsWith(".bin")) {
            throw new IllegalArgumentException("Select a BIN file for " + targetName);
        }
        File target = new File(gameDataDirectory, targetName);
        try (InputStream input = getContentResolver().openInputStream(source)) {
            if (input == null) {
                throw new IOException("Unable to open selected BIN file");
            }
            try (FileOutputStream output = new FileOutputStream(target, false)) {
                byte[] buffer = new byte[64 * 1024];
                int count;
                while ((count = input.read(buffer)) != -1) {
                    output.write(buffer, 0, count);
                }
            }
        }
        if (target.length() == 0) {
            throw new IllegalArgumentException("Selected BIN file is empty: " + targetName);
        }
        return target;
    }

    private static void writeBytes(File target, byte[] bytes) throws IOException {
        try (FileOutputStream output = new FileOutputStream(target, false)) {
            output.write(bytes);
        }
    }

    private String displayName(Uri source) {
        try (Cursor cursor = getContentResolver().query(source,
                new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (index >= 0) {
                    return cursor.getString(index);
                }
            }
        }
        return null;
    }

    private void showDataStatus(String message) {
        if (dataStatus != null) {
            dataStatus.setText(message);
            dataStatus.setTextColor(0xFFFFFFFF);
        }
    }

    private void showDataError(String message) {
        if (dataStatus != null) {
            dataStatus.setText("Game data error: " + message);
            dataStatus.setTextColor(0xFFFF8080);
        }
    }
}