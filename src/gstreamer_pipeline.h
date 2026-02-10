#pragma once

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <functional>

class GStreamerPipeline {
public:
    using FrameCallback = std::function<void(const uint8_t*, int, int)>;

    GStreamerPipeline();
    ~GStreamerPipeline();

    bool initialize(const std::string& pipeline_description);
    bool start();
    void stop();

    void setFrameCallback(FrameCallback callback);

private:
    GstElement* pipeline_;
    GstElement* appsink_;
    FrameCallback frame_callback_;

    static GstFlowReturn newSampleCallback(GstElement* sink, gpointer user_data);
    void processFrame(GstSample* sample);
};