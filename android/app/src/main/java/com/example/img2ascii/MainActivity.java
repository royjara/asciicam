package com.example.img2ascii;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import java.net.InetAddress;
import java.net.UnknownHostException;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

public class MainActivity extends Activity implements SurfaceHolder.Callback {
    private static final String TAG = "MainActivity";

    static {
        System.loadLibrary("img2ascii");
    }

    private static final int CAMERA_PERMISSION_REQUEST = 100;
    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    private boolean hasPermission = false;
    private boolean surfaceReady = false;

    // Streaming UI components
    private Button streamButton;
    private TextView streamStatusText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        surfaceView = findViewById(R.id.surface_view);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);

        // Initialize streaming UI
        streamButton = findViewById(R.id.stream_button);
        streamStatusText = findViewById(R.id.stream_status);

        streamButton.setOnClickListener(v -> {
            if (nativeIsStreaming()) {
                stopStreaming();
            } else {
                startStreaming();
            }
        });

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.CAMERA},
                    CAMERA_PERMISSION_REQUEST);
        } else {
            hasPermission = true;
            tryInitialize();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == CAMERA_PERMISSION_REQUEST) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                hasPermission = true;
                tryInitialize();
            } else {
                Toast.makeText(this, "Camera permission required", Toast.LENGTH_SHORT).show();
                finish();
            }
        }
    }

    private void tryInitialize() {
        Log.i(TAG, "tryInitialize: hasPermission=" + hasPermission + ", surfaceReady=" + surfaceReady);
        if (hasPermission && surfaceReady) {
            Log.i(TAG, "Initializing native code");
            nativeInit();
        }
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.i(TAG, "surfaceCreated");
        surfaceReady = true;
        nativeSurfaceCreated(holder.getSurface());
        tryInitialize();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        // Handle surface changes if needed
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.i(TAG, "surfaceDestroyed");
        surfaceReady = false;
        nativeSurfaceDestroyed();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        nativeCleanup();
    }

    private void startStreaming() {
        Log.i(TAG, "Starting streaming...");
        if (nativeStartStreaming()) {
            streamButton.setText("Stop Streaming");
            String deviceIP = getDeviceIPAddress();
            String streamUrl = "rtsp://" + deviceIP + ":8554/ascii_stream";
            streamStatusText.setText("Stream: Active (" + streamUrl + ")");
            streamStatusText.setTextColor(0xFF00FF00); // Green
            Toast.makeText(this, "Streaming at " + streamUrl, Toast.LENGTH_LONG).show();
        } else {
            Toast.makeText(this, "Failed to start streaming", Toast.LENGTH_SHORT).show();
        }
    }

    private String getDeviceIPAddress() {
        try {
            WifiManager wifiManager = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
            if (wifiManager != null) {
                WifiInfo wifiInfo = wifiManager.getConnectionInfo();
                int ipAddress = wifiInfo.getIpAddress();

                if (ipAddress != 0) {
                    return String.format("%d.%d.%d.%d",
                        (ipAddress & 0xff),
                        (ipAddress >> 8 & 0xff),
                        (ipAddress >> 16 & 0xff),
                        (ipAddress >> 24 & 0xff));
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Error getting IP address: " + e.getMessage());
        }
        return "192.168.1.xxx"; // Fallback
    }

    private void stopStreaming() {
        Log.i(TAG, "Stopping streaming...");
        nativeStopStreaming();
        streamButton.setText("Start Streaming");
        streamStatusText.setText("Stream: Stopped");
        streamStatusText.setTextColor(0xFFFFFFFF); // White
        Toast.makeText(this, "Streaming stopped", Toast.LENGTH_SHORT).show();
    }

    // Native method declarations
    public native void nativeInit();
    public native void nativeSurfaceCreated(Surface surface);
    public native void nativeSurfaceDestroyed();
    public native void nativeCleanup();

    // Streaming native methods
    public native boolean nativeStartStreaming();
    public native void nativeStopStreaming();
    public native boolean nativeIsStreaming();
}