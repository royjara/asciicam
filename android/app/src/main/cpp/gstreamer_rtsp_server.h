#pragma once

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

class GStreamerRTSPServer {
public:
    GStreamerRTSPServer();
    ~GStreamerRTSPServer();

    bool initialize(int width, int height, int fps = 20, int bitrate = 2000000);
    void shutdown();

    bool startStreaming(const std::string& mount_point = "/ascii_stream", int port = 8554);
    void stopStreaming();
    bool isStreaming() const { return streaming_.load(); }

    void pushFrame(const uint8_t* rgb_data, int width, int height);

    // Configuration
    void setBitrate(int bitrate);
    void setFrameRate(int fps);

private:
    // GStreamer elements
    GstRTSPServer* rtsp_server_;
    GstRTSPMountPoints* mount_points_;
    GstRTSPMediaFactory* factory_;

    // Pipeline elements (for the factory)
    std::string pipeline_description_;

    // App source for frame injection
    GstElement* appsrc_;

    // Stream parameters
    int width_, height_, fps_, bitrate_;

    // Threading
    std::thread gst_thread_;
    std::atomic<bool> streaming_;
    std::atomic<bool> initialized_;
    GMainLoop* main_loop_;

    // Server attachment tracking
    guint server_id_;

    // Frame buffering
    std::mutex frame_mutex_;
    uint8_t* frame_buffer_;
    size_t frame_size_;
    bool frame_ready_;

    // Private methods
    void gstThreadFunc();
    static void onMediaConfigure(GstRTSPMediaFactory* factory, GstRTSPMedia* media, gpointer user_data);
    static void onNeedData(GstElement* appsrc, guint unused_size, gpointer user_data);
    static void onEnoughData(GstElement* appsrc, gpointer user_data);

    void setupPipeline();
    void cleanupPipeline();
    bool initializeGStreamer();
};