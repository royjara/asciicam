#include "gstreamer_pipeline.h"
#include <gst/video/video.h>
#include <iostream>

GStreamerPipeline::GStreamerPipeline()
    : pipeline_(nullptr)
    , appsink_(nullptr) {
}

GStreamerPipeline::~GStreamerPipeline() {
    stop();
    if (pipeline_) {
        gst_object_unref(pipeline_);
    }
}

bool GStreamerPipeline::initialize(const std::string& pipeline_description) {
    gst_init(nullptr, nullptr);

    GError* error = nullptr;
    pipeline_ = gst_parse_launch(pipeline_description.c_str(), &error);

    if (error) {
        std::cerr << "Failed to parse pipeline: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    appsink_ = gst_bin_get_by_name(GST_BIN(pipeline_), "appsink");
    if (!appsink_) {
        std::cerr << "Failed to find appsink element" << std::endl;
        return false;
    }

    g_object_set(appsink_, "emit-signals", TRUE, nullptr);
    g_signal_connect(appsink_, "new-sample", G_CALLBACK(newSampleCallback), this);

    return true;
}

bool GStreamerPipeline::start() {
    if (!pipeline_) {
        return false;
    }

    GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    return ret != GST_STATE_CHANGE_FAILURE;
}

void GStreamerPipeline::stop() {
    if (pipeline_) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
    }
}

void GStreamerPipeline::setFrameCallback(FrameCallback callback) {
    frame_callback_ = callback;
}

GstFlowReturn GStreamerPipeline::newSampleCallback(GstElement* sink, gpointer user_data) {
    GStreamerPipeline* pipeline = static_cast<GStreamerPipeline*>(user_data);

    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (sample) {
        pipeline->processFrame(sample);
        gst_sample_unref(sample);
    }

    return GST_FLOW_OK;
}

void GStreamerPipeline::processFrame(GstSample* sample) {
    if (!frame_callback_) {
        return;
    }

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstCaps* caps = gst_sample_get_caps(sample);

    GstStructure* structure = gst_caps_get_structure(caps, 0);
    int width, height;
    gst_structure_get_int(structure, "width", &width);
    gst_structure_get_int(structure, "height", &height);

    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        frame_callback_(map.data, width, height);
        gst_buffer_unmap(buffer, &map);
    }
}