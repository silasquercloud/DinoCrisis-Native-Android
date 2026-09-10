package com.dinocrisis.nativeandroid;

import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.Window;
import android.view.WindowManager;
import java.io.File;

public final class MainActivity extends Activity {
    static { System.loadLibrary("dinocrisis_native"); }
    private NativeSurfaceView surfaceView;

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);

        File gameData = ExternalGameDataConfig.resolveDirectory(this, null);
        nativeInitialize(gameData.getAbsolutePath());
        surfaceView = new NativeSurfaceView();
        setContentView(surfaceView);
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
            @Override public void onSurfaceCreated(javax.microedition.khronos.opengles.GL10 gl, javax.microedition.khronos.egl.EGLConfig config) {
                nativeOnSurfaceCreated();
            }
            @Override public void onSurfaceChanged(javax.microedition.khronos.opengles.GL10 gl, int width, int height) {
                nativeOnSurfaceChanged(width, height);
            }
            @Override public void onDrawFrame(javax.microedition.khronos.opengles.GL10 gl) {
                nativeOnDrawFrame();
            }
        }
    }

    private static native void nativeInitialize(String gameDataPath);
    private static native void nativeShutdown();
    private static native void nativeOnSurfaceCreated();
    private static native void nativeOnSurfaceChanged(int width, int height);
    private static native void nativeOnDrawFrame();
}