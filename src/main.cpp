#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include <string>
#include <mutex>
#include "ascii_converter.h"
#include "gstreamer_pipeline.h"
#include "gl_window.h"

static bool running = true;
static std::string current_ascii_frame;
static std::mutex frame_mutex;

void signalHandler(int signal) {
    running = false;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signalHandler);

    GLWindow window(1024, 768, "ASCII Video Stream");
    if (!window.initialize()) {
        std::cerr << "Failed to initialize OpenGL window" << std::endl;
        return 1;
    }

    AsciiConverter converter(128, 64);
    GStreamerPipeline pipeline;

    std::string pipeline_str;

    if (argc > 1) {
        std::string source = argv[1];
        if (source == "webcam") {
            pipeline_str = "avfvideosrc ! videoconvert ! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 ! appsink name=appsink";
        } else if (source == "smpte") {
            pipeline_str = "videotestsrc pattern=smpte ! videoconvert ! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 ! appsink name=appsink";
        } else if (source == "checkers") {
            pipeline_str = "videotestsrc pattern=checkers-1 ! videoconvert ! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 ! appsink name=appsink";
        } else if (source == "circular") {
            pipeline_str = "videotestsrc pattern=circular ! videoconvert ! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 ! appsink name=appsink";
        } else if (source.find(".") != std::string::npos) {
            pipeline_str = "filesrc location=" + source + " ! decodebin ! videoconvert ! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 ! appsink name=appsink";
        } else {
            std::cout << "Usage: " << argv[0] << " [webcam|smpte|checkers|circular|video_file.mp4]" << std::endl;
            return 1;
        }
    } else {
        pipeline_str = "videotestsrc pattern=ball ! videoconvert ! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 ! appsink name=appsink";
    }

    if (!pipeline.initialize(pipeline_str)) {
        std::cerr << "Failed to initialize pipeline" << std::endl;
        return 1;
    }

    pipeline.setFrameCallback([&converter](const uint8_t* data, int width, int height) {
        std::string ascii_frame = converter.convertRGBBuffer(data, width, height);

        std::lock_guard<std::mutex> lock(frame_mutex);
        current_ascii_frame = ascii_frame;
    });

    std::cout << "Starting ASCII video stream in OpenGL window (ESC to quit)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (!pipeline.start()) {
        std::cerr << "Failed to start pipeline" << std::endl;
        return 1;
    }

    auto last_time = std::chrono::high_resolution_clock::now();
    int frame_count = 0;

    while (!window.shouldClose() && running) {
        window.pollEvents();

        GLTextRenderer* renderer = window.getTextRenderer();
        renderer->clear();
        renderer->setColor(0.0f, 1.0f, 0.0f, 1.0f);

        {
            std::lock_guard<std::mutex> lock(frame_mutex);
            if (!current_ascii_frame.empty()) {
                renderer->renderText(current_ascii_frame, 10.0f, 20.0f, 1.0f);
            }
        }

        auto current_time = std::chrono::high_resolution_clock::now();
        frame_count++;

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_time);
        if (duration.count() >= 1000) {
            std::string fps_text = "FPS: " + std::to_string(frame_count);
            renderer->setColor(1.0f, 1.0f, 0.0f, 1.0f);
            renderer->renderText(fps_text, 10.0f, window.getHeight() - 30.0f, 1.0f);

            frame_count = 0;
            last_time = current_time;
        }

        window.swapBuffers();
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    pipeline.stop();
    std::cout << "\nStopped." << std::endl;

    return 0;
}