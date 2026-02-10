#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <thread>
#include <mutex>
#include <string>

#include "android_renderer.h"
#include "android_camera.h"
#include "ascii_converter.h"

#define LOG_TAG "AndroidMain"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static AndroidRenderer* g_renderer = nullptr;
static AndroidCamera* g_camera = nullptr;
static AsciiConverter* g_converter = nullptr;
static AsciiLayers g_current_ascii_layers;
static std::mutex g_frame_mutex;
static bool g_running = false;
static std::thread g_render_thread;

void renderLoop() {
    LOGI("Render loop started");

    while (g_running && g_renderer && g_renderer->isReady()) {
        static int loop_count = 0;
        if (loop_count < 3) {
            LOGI("Render loop iteration %d", loop_count);
            loop_count++;
        }

        // Make sure EGL context is current on this thread
        if (!g_renderer->makeCurrent()) {
            LOGE("Failed to make EGL context current in render thread");
            break;
        }

        g_renderer->clear();

        g_renderer->setColor(1.0f, 0.0f, 0.0f, 1.0f);
        g_renderer->renderText("TEST", 100.0f, 100.0f, 8.0f);

        {
            std::lock_guard<std::mutex> lock(g_frame_mutex);
            if (!g_current_ascii_layers.red_layer.empty()) {
                // Render red layer
                g_renderer->setColor(1.0f, 0.0f, 0.0f, 0.7f);  // Red with transparency
                g_renderer->renderText(g_current_ascii_layers.red_layer, 50.0f, 150.0f, 1.5f);

                // Render green layer with slight offset for visual effect
                g_renderer->setColor(0.0f, 1.0f, 0.0f, 0.7f);  // Green with transparency
                g_renderer->renderText(g_current_ascii_layers.green_layer, 52.0f, 152.0f, 1.5f);

                // Render blue layer with slight offset
                g_renderer->setColor(0.0f, 0.0f, 1.0f, 0.7f);  // Blue with transparency
                g_renderer->renderText(g_current_ascii_layers.blue_layer, 54.0f, 154.0f, 1.5f);
            } else {
                static bool logged_empty = false;
                if (!logged_empty) {
                    LOGI("No ASCII layers available for rendering");
                    logged_empty = true;
                }
            }
        }

        g_renderer->swapBuffers();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    LOGI("Render loop ended");
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_example_img2ascii_MainActivity_nativeInit(JNIEnv* env, jobject thiz) {
    LOGI("Native init");

    g_converter = new AsciiConverter(80, 40);

    g_camera = new AndroidCamera();
    if (g_camera->initialize()) {
        g_camera->setFrameCallback([](const uint8_t* data, int width, int height) {
            if (g_converter) {
                static int frame_count = 0;
                if (frame_count < 3) {
                    LOGI("Camera callback: %dx%d frame received", width, height);
                    frame_count++;
                }

                AsciiLayers layers = g_converter->convertRGBBufferToLayers(data, width, height);

                std::lock_guard<std::mutex> lock(g_frame_mutex);
                g_current_ascii_layers = layers;
            }
        });
        g_camera->start();
    } else {
        LOGE("Failed to initialize camera");
    }
}

JNIEXPORT void JNICALL
Java_com_example_img2ascii_MainActivity_nativeSurfaceCreated(JNIEnv* env, jobject thiz, jobject surface) {
    LOGI("Native surface created");

    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("Failed to get native window");
        return;
    }

    g_renderer = new AndroidRenderer();
    if (g_renderer->initialize(window)) {
        // Release context from main thread so render thread can use it
        g_renderer->releaseContext();

        g_running = true;
        g_render_thread = std::thread(renderLoop);
        LOGI("Renderer initialized successfully");
    } else {
        LOGE("Failed to initialize renderer");
        delete g_renderer;
        g_renderer = nullptr;
    }
}

JNIEXPORT void JNICALL
Java_com_example_img2ascii_MainActivity_nativeSurfaceDestroyed(JNIEnv* env, jobject thiz) {
    LOGI("Native surface destroyed");

    g_running = false;

    if (g_render_thread.joinable()) {
        g_render_thread.join();
    }

    if (g_renderer) {
        g_renderer->shutdown();
        delete g_renderer;
        g_renderer = nullptr;
    }
}

JNIEXPORT void JNICALL
Java_com_example_img2ascii_MainActivity_nativeCleanup(JNIEnv* env, jobject thiz) {
    LOGI("Native cleanup");

    if (g_camera) {
        g_camera->stop();
        delete g_camera;
        g_camera = nullptr;
    }

    if (g_converter) {
        delete g_converter;
        g_converter = nullptr;
    }
}

}