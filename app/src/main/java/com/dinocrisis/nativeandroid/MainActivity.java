package com.dinocrisis.nativeandroid;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.graphics.Color;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
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
    private File trackTwoFile;
    private Uri cueUri;
    private Uri trackOneUri;
    private Uri trackTwoUri;
    private Button loadGameButton;
    private TextView cueSelectionStatus;
    private TextView trackOneSelectionStatus;
    private TextView trackTwoSelectionStatus;

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

        LinearLayout menu = new LinearLayout(this);
        menu.setOrientation(LinearLayout.VERTICAL);
        menu.setPadding(32, 28, 32, 28);
        menu.setBackgroundColor(0xEE101820);

        TextView title = new TextView(this);
        title.setText("DINO CRISIS NATIVE");
        title.setTextColor(Color.WHITE);
        title.setTextSize(24);
        title.setGravity(Gravity.CENTER);
        menu.addView(title, matchWrapParams(0, 16));

        cueSelectionStatus = addSelection(menu, "SELECT CUE", view -> beginSelection(REQUEST_CUE, "text/plain"), "Not selected");
        trackOneSelectionStatus = addSelection(menu, "SELECT TRACK 1 BIN", view -> beginSelection(REQUEST_TRACK_ONE, "application/octet-stream"), "Not selected");
        trackTwoSelectionStatus = addSelection(menu, "SELECT TRACK 2 BIN", view -> beginSelection(REQUEST_TRACK_TWO, "application/octet-stream"), "Not selected");

        loadGameButton = new Button(this);
        loadGameButton.setText("LOAD GAME");
        loadGameButton.setEnabled(false);
        loadGameButton.setOnClickListener(view -> loadGame());
        menu.addView(loadGameButton, matchWrapParams(0, 12));

        Button settingsButton = new Button(this);
        settingsButton.setText("SETTINGS");
        settingsButton.setOnClickListener(view -> showSettings());
        menu.addView(settingsButton, matchWrapParams(0, 8));

        dataStatus = new TextView(this);
        dataStatus.setText(ALPHA_RUNTIME_STATUS);
        dataStatus.setTextColor(Color.WHITE);
        dataStatus.setPadding(12, 18, 12, 18);
        menu.addView(dataStatus, matchWrapParams(0, 8));

        ScrollView scrollView = new ScrollView(this);
        scrollView.addView(menu);
        FrameLayout.LayoutParams menuParams = new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        menuParams.gravity = Gravity.TOP;
        root.addView(scrollView, menuParams);
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
    private static native String nativeLoadGame(String cuePath, String trackOnePath, String trackTwoPath);

    private LinearLayout.LayoutParams matchWrapParams(int topMargin, int bottomMargin) {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.topMargin = topMargin;
        params.bottomMargin = bottomMargin;
        return params;
    }

    private TextView addSelection(LinearLayout menu, String label, View.OnClickListener listener, String initialText) {
        Button button = new Button(this);
        button.setText(label);
        button.setOnClickListener(listener);
        menu.addView(button, matchWrapParams(8, 0));
        TextView status = new TextView(this);
        status.setText(initialText);
        status.setTextColor(0xFFB8C7D1);
        status.setPadding(12, 2, 12, 8);
        menu.addView(status, matchWrapParams(0, 4));
        return status;
    }

    private void beginSelection(int requestCode, String mimeType) {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.putExtra(Intent.EXTRA_MIME_TYPES, new String[]{mimeType, "application/octet-stream"});
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        startActivityForResult(intent, requestCode);
    }

    @Override protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            showDataError("Disc selection cancelled");
            return;
        }

        try {
            persistPermission(data.getData());
            if (requestCode == REQUEST_CUE) {
                cueUri = data.getData();
                cueFile = importCue(cueUri);
                setSelectionStatus(cueSelectionStatus, "Valid: " + displayName(cueUri));
            } else if (requestCode == REQUEST_TRACK_ONE) {
                trackOneUri = data.getData();
                trackOneFile = importBinary(trackOneUri, "track1.bin");
                setSelectionStatus(trackOneSelectionStatus, "Valid: " + displayName(trackOneUri));
            } else if (requestCode == REQUEST_TRACK_TWO) {
                trackTwoUri = data.getData();
                trackTwoFile = importBinary(trackTwoUri, "track2.bin");
                setSelectionStatus(trackTwoSelectionStatus, "Valid: " + displayName(trackTwoUri));
            }
            updateLoadButton();
            showDataStatus("Selection accepted. Choose LOAD GAME when all three files are ready.");
        } catch (IOException | IllegalArgumentException error) {
            TextView status = requestCode == REQUEST_CUE ? cueSelectionStatus :
                requestCode == REQUEST_TRACK_ONE ? trackOneSelectionStatus : trackTwoSelectionStatus;
            setSelectionError(status, error.getMessage() == null ? "Unable to import selected file" : error.getMessage());
            showDataError(error.getMessage() == null ? "Unable to import selected file" : error.getMessage());
        }
    }

    private void persistPermission(Uri source) {
        try {
            getContentResolver().takePersistableUriPermission(source,
                Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } catch (SecurityException ignored) {
            showDataStatus("File accepted for this session; persistent access was unavailable.");
        }
    }

    private void updateLoadButton() {
        if (cueFile == null || trackOneFile == null || trackTwoFile == null) {
            loadGameButton.setEnabled(false);
            return;
        }
        String validation = nativeValidateDiscFiles(
            cueFile.getAbsolutePath(), trackOneFile.getAbsolutePath(), trackTwoFile.getAbsolutePath());
        boolean valid = validation != null && !validation.startsWith("ERROR:");
        loadGameButton.setEnabled(valid);
        if (valid) {
            showDataStatus("All three files pass native validation. LOAD GAME is ready.");
        }
    }

    private void loadGame() {
        if (cueFile == null || trackOneFile == null || trackTwoFile == null) {
            showDataError("Select CUE, Track 1 BIN, and Track 2 BIN first");
            return;
        }
        loadGameButton.setEnabled(false);
        showDataStatus("Loading: validating CUE, tracks, sector access, and native runtime...");
        String runtimeStatus = nativeLoadGame(cueFile.getAbsolutePath(), trackOneFile.getAbsolutePath(), trackTwoFile.getAbsolutePath());
        if (runtimeStatus != null && runtimeStatus.startsWith("ERROR:")) {
            showDataError(runtimeStatus.substring("ERROR:".length()));
            updateLoadButton();
            return;
        }
        showDataStatus(runtimeStatus + "\nGame runtime not yet ready");
    }

    private void showSettings() {
        new AlertDialog.Builder(this)
            .setTitle("Settings")
            .setMessage("OpenGL ES 3.x\nPrimary ABI: arm64-v8a\nGame files stay external and read-only.\nThe native game runtime is not yet ready.")
            .setPositiveButton("OK", null)
            .show();
    }

    private void setSelectionStatus(TextView view, String message) {
        view.setText(message);
        view.setTextColor(0xFF80E080);
    }

    private void setSelectionError(TextView view, String message) {
        view.setText("Error: " + message);
        view.setTextColor(0xFFFF8080);
    }

    private File importCue(Uri source) throws IOException {
        String displayName = displayName(source);
        if (displayName != null) {
            GameDataFileSelection.requireCueName(displayName);
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
        if (displayName != null) {
            GameDataFileSelection.requireBinName(displayName, targetName);
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
        GameDataFileSelection.requireNonEmpty(target, targetName);
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