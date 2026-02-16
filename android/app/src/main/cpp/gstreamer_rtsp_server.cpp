#include "gstreamer_rtsp_server.h"
#include <android/log.h>
#include <cstring>
#include <sstream>

// Use generated plugin initialization
extern "C" void gst_android_init(void);

#define LOG_TAG "GStreamerRTSP"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

GStreamerRTSPServer::GStreamerRTSPServer()
    : rtsp_server_(nullptr)
    , mount_points_(nullptr)
    , factory_(nullptr)
    , appsrc_(nullptr)
    , width_(640)
    , height_(480)
    , fps_(20)
    , bitrate_(2000000)
    , streaming_(false)
    , initialized_(false)
    , main_loop_(nullptr)
    , server_id_(0)
    , frame_buffer_(nullptr)
    , frame_size_(0)
    , frame_ready_(false)
{
}

GStreamerRTSPServer::~GStreamerRTSPServer() {
    shutdown();
}

bool GStreamerRTSPServer::initialize(int width, int height, int fps, int bitrate) {
    LOGI("Initializing GStreamer RTSP Server: %dx%d @ %d fps, bitrate: %d",
         width, height, fps, bitrate);

    width_ = width;
    height_ = height;
    fps_ = fps;
    bitrate_ = bitrate;
    frame_size_ = width * height * 3; // RGB

    // Allocate frame buffer
    frame_buffer_ = new uint8_t[frame_size_];

    if (!initializeGStreamer()) {
        LOGE("Failed to initialize GStreamer");
        return false;
    }

    initialized_ = true;
    return true;
}

bool GStreamerRTSPServer::initializeGStreamer() {
    // Initialize GStreamer
    gst_init(NULL, NULL);

    // Register all static plugins using generated initialization
    gst_android_init();

    LOGI("GStreamer initialized and plugins registered");

    // Create RTSP server
    rtsp_server_ = gst_rtsp_server_new();
    if (!rtsp_server_) {
        LOGE("Failed to create RTSP server");
        return false;
    }

    // Get mount points
    mount_points_ = gst_rtsp_server_get_mount_points(rtsp_server_);

    // Create media factory
    factory_ = gst_rtsp_media_factory_new();

    // Build pipeline description
    setupPipeline();

    return true;
}

void GStreamerRTSPServer::setupPipeline() {
    std::ostringstream pipeline_stream;

    // Pipeline: appsrc → videoconvertscale → x264enc → rtph264pay
    pipeline_stream << "( "
                   << "appsrc name=source is-live=true do-timestamp=true format=time "
                   << "caps=\"video/x-raw,format=RGB,width=" << width_
                   << ",height=" << height_ << ",framerate=" << fps_ << "/1\" ! "
                   << "videoconvertscale ! "
                   << "video/x-raw,format=I420 ! "
                   << "x264enc bitrate=" << bitrate_ << " speed-preset=ultrafast tune=zerolatency ! "
                   << "video/x-h264,profile=baseline ! "
                   << "rtph264pay name=pay0 pt=96 "
                   << ")";

    pipeline_description_ = pipeline_stream.str();

    LOGI("Pipeline: %s", pipeline_description_.c_str());
    gst_rtsp_media_factory_set_launch(factory_, pipeline_description_.c_str());

    // Set up shared pipeline
    gst_rtsp_media_factory_set_shared(factory_, TRUE);

    // Connect to media configure signal
    g_signal_connect(factory_, "media-configure", G_CALLBACK(onMediaConfigure), this);
}

bool GStreamerRTSPServer::startStreaming(const std::string& mount_point, int port) {
    LOGI("startStreaming called - initialized: %s, streaming: %s",
         initialized_ ? "true" : "false", streaming_ ? "true" : "false");

    if (!initialized_) {
        LOGE("GStreamer not initialized");
        return false;
    }

    if (streaming_) {
        LOGI("Already streaming");
        return true;
    }

    LOGI("Starting RTSP server on port %d, mount point: %s", port, mount_point.c_str());

    // Check current state of GStreamer components
    LOGI("Component state - rtsp_server: %p, mount_points: %p, factory: %p",
         rtsp_server_, mount_points_, factory_);

    // Always recreate the factory to ensure clean state
    if (factory_) {
        LOGI("Checking factory validity before cleanup");
        if (GST_IS_RTSP_MEDIA_FACTORY(factory_)) {
            LOGI("Factory is valid, cleaning up");
            g_object_unref(factory_);
        } else {
            LOGE("Factory pointer is corrupted, skipping unref");
        }
    } else {
        LOGI("No existing factory to clean up");
    }
    factory_ = nullptr;

    // Reinitialize GStreamer components if needed
    if (!rtsp_server_ || !mount_points_) {
        LOGI("Reinitializing GStreamer components");
        if (!initializeGStreamer()) {
            LOGE("Failed to reinitialize GStreamer");
            return false;
        }
        LOGI("GStreamer components reinitialized successfully");
    } else {
        LOGI("RTSP server and mount points present");
    }

    // Always create a fresh factory
    LOGI("Creating new factory");
    factory_ = gst_rtsp_media_factory_new();
    if (!factory_) {
        LOGE("Failed to create media factory");
        return false;
    }
    setupPipeline();
    LOGI("New factory created and configured");

    // Start GStreamer main loop in separate thread first
    LOGI("Setting streaming to true and starting thread");
    streaming_ = true;
    gst_thread_ = std::thread(&GStreamerRTSPServer::gstThreadFunc, this);

    // Give the thread time to create the main loop
    LOGI("Waiting 100ms for main loop to start");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Set server port
    LOGI("Setting server port to %d", port);
    gchar* port_str = g_strdup_printf("%d", port);
    g_object_set(rtsp_server_, "service", port_str, nullptr);
    g_free(port_str);

    // Add factory to mount points
    LOGI("Adding factory to mount points for path: %s", mount_point.c_str());
    gst_rtsp_mount_points_add_factory(mount_points_, mount_point.c_str(), factory_);

    // Try to attach the server to the default main context
    // The main loop should be running in the separate thread
    LOGI("Attempting to attach RTSP server");
    server_id_ = gst_rtsp_server_attach(rtsp_server_, nullptr);
    if (server_id_ == 0) {
        LOGE("Failed to attach RTSP server - trying different approach");

        // Try with explicit main context
        GMainContext* context = g_main_context_get_thread_default();
        if (!context) {
            LOGI("Using default main context");
            context = g_main_context_default();
        } else {
            LOGI("Using thread default main context");
        }

        server_id_ = gst_rtsp_server_attach(rtsp_server_, context);
        if (server_id_ == 0) {
            LOGE("Failed to attach RTSP server to any context");
            streaming_ = false;
            return false;
        }
    }

    LOGI("RTSP server started successfully with ID: %u", server_id_);
    return true;
}

void GStreamerRTSPServer::stopStreaming() {
    if (!streaming_) {
        LOGI("stopStreaming called but already not streaming");
        return;
    }

    LOGI("Stopping RTSP server - setting streaming to false");
    streaming_ = false;

    // Detach server from context to free up the port
    if (server_id_ != 0 && rtsp_server_) {
        LOGI("Detaching RTSP server (ID: %u) from context", server_id_);
        g_source_remove(server_id_);
        server_id_ = 0;
    } else {
        LOGI("No server ID to detach (server_id: %u, rtsp_server: %p)", server_id_, rtsp_server_);
    }

    // Remove factory from mount points to clean up resources
    if (mount_points_ && factory_) {
        LOGI("Removing factory from mount points");
        gst_rtsp_mount_points_remove_factory(mount_points_, "/ascii_stream");
        LOGI("Factory removed from mount points");

        // The factory may have been freed by mount_points_remove, so clear our reference
        // We'll recreate it fresh on next start
        factory_ = nullptr;
        LOGI("Factory reference cleared - will recreate fresh on next start");
    }

    // Stop main loop
    if (main_loop_) {
        LOGI("Quitting main loop");
        g_main_loop_quit(main_loop_);
    } else {
        LOGI("No main loop to quit");
    }

    // Wait for thread to finish
    if (gst_thread_.joinable()) {
        LOGI("Waiting for GStreamer thread to join");
        gst_thread_.join();
        LOGI("GStreamer thread joined successfully");
    } else {
        LOGI("No GStreamer thread to join");
    }

    // Clear the appsrc reference
    if (appsrc_) {
        LOGI("Clearing appsrc reference");
    } else {
        LOGI("appsrc already null");
    }
    appsrc_ = nullptr;

    LOGI("RTSP server stopped successfully - port should be freed for reuse");
}

void GStreamerRTSPServer::shutdown() {
    LOGI("shutdown() called");
    stopStreaming();

    if (frame_buffer_) {
        LOGI("Deleting frame buffer");
        delete[] frame_buffer_;
        frame_buffer_ = nullptr;
    } else {
        LOGI("No frame buffer to delete");
    }

    // Clean up GStreamer objects only on final shutdown
    LOGI("Starting pipeline cleanup");
    cleanupPipeline();

    initialized_ = false;
    LOGI("shutdown() completed");
}

void GStreamerRTSPServer::cleanupPipeline() {
    LOGI("Cleaning up pipeline resources");

    // Clear appsrc reference without unreffing (it's owned by the media pipeline)
    appsrc_ = nullptr;

    // More defensive checks for object validity
    if (mount_points_) {
        LOGI("Cleaning up mount_points");
        if (GST_IS_RTSP_MOUNT_POINTS(mount_points_)) {
            g_object_unref(mount_points_);
        } else {
            LOGE("mount_points_ is not a valid RTSP mount points object");
        }
        mount_points_ = nullptr;
    }

    if (factory_) {
        LOGI("Cleaning up factory");
        if (GST_IS_RTSP_MEDIA_FACTORY(factory_)) {
            g_object_unref(factory_);
        } else {
            LOGE("factory_ is not a valid RTSP media factory object");
        }
        factory_ = nullptr;
    }

    if (rtsp_server_) {
        LOGI("Cleaning up rtsp_server");
        if (GST_IS_RTSP_SERVER(rtsp_server_)) {
            g_object_unref(rtsp_server_);
        } else {
            LOGE("rtsp_server_ is not a valid RTSP server object");
        }
        rtsp_server_ = nullptr;
    }

    LOGI("Pipeline cleanup completed");
}

void GStreamerRTSPServer::gstThreadFunc() {
    LOGI("GStreamer main loop thread started");

    // Create and run main loop
    LOGI("Creating main loop");
    main_loop_ = g_main_loop_new(nullptr, FALSE);

    if (!main_loop_) {
        LOGE("Failed to create main loop");
        return;
    }

    LOGI("Running main loop (this will block until quit is called)");
    g_main_loop_run(main_loop_);

    LOGI("Main loop finished, cleaning up");

    // Cleanup
    g_main_loop_unref(main_loop_);
    main_loop_ = nullptr;

    LOGI("GStreamer main loop thread ended");
}

void GStreamerRTSPServer::pushFrame(const uint8_t* rgb_data, int width, int height) {
    if (!streaming_ || !appsrc_ || !rgb_data) {
        return;
    }

    // Verify dimensions match
    if (width != width_ || height != height_) {
        LOGE("Frame dimensions mismatch: got %dx%d, expected %dx%d",
             width, height, width_, height_);
        return;
    }

    // Copy frame data
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        memcpy(frame_buffer_, rgb_data, frame_size_);
        frame_ready_ = true;
    }

    // Create GStreamer buffer
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, frame_size_, nullptr);
    if (!buffer) {
        LOGE("Failed to allocate GstBuffer");
        return;
    }

    // Map and copy data
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            memcpy(map.data, frame_buffer_, frame_size_);
        }
        gst_buffer_unmap(buffer, &map);

        // Push buffer to appsrc
        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc_), buffer);
        if (ret != GST_FLOW_OK) {
            LOGE("Failed to push buffer to appsrc: %d", ret);
        }
    } else {
        LOGE("Failed to map GstBuffer");
        gst_buffer_unref(buffer);
    }
}

void GStreamerRTSPServer::setBitrate(int bitrate) {
    bitrate_ = bitrate;
    if (initialized_) {
        // Rebuild pipeline with new bitrate
        setupPipeline();
    }
}

void GStreamerRTSPServer::setFrameRate(int fps) {
    fps_ = fps;
    if (initialized_) {
        // Rebuild pipeline with new framerate
        setupPipeline();
    }
}

// Static callback functions
void GStreamerRTSPServer::onMediaConfigure(GstRTSPMediaFactory* factory, GstRTSPMedia* media, gpointer user_data) {
    GStreamerRTSPServer* server = static_cast<GStreamerRTSPServer*>(user_data);

    LOGI("Media configure callback");

    // Get the pipeline element
    GstElement* element = gst_rtsp_media_get_element(media);

    // Find the appsrc element
    server->appsrc_ = gst_bin_get_by_name_recurse_up(GST_BIN(element), "source");
    if (server->appsrc_) {
        LOGI("Found appsrc element");

        // Configure appsrc
        g_object_set(server->appsrc_,
            "stream-type", 0, // GST_APP_STREAM_TYPE_STREAM
            "is-live", TRUE,
            "do-timestamp", TRUE,
            "min-latency", G_GINT64_CONSTANT(0),
            "max-latency", G_GINT64_CONSTANT(0),
            "format", GST_FORMAT_TIME,
            nullptr);

        // Connect callbacks
        g_signal_connect(server->appsrc_, "need-data", G_CALLBACK(onNeedData), server);
        g_signal_connect(server->appsrc_, "enough-data", G_CALLBACK(onEnoughData), server);
    } else {
        LOGE("Failed to find appsrc element");
    }

    gst_object_unref(element);
}

void GStreamerRTSPServer::onNeedData(GstElement* appsrc, guint unused_size, gpointer user_data) {
    // This callback is called when the appsrc needs data
    // We'll push frames from the main thread, so nothing to do here
}

void GStreamerRTSPServer::onEnoughData(GstElement* appsrc, gpointer user_data) {
    // This callback is called when the appsrc has enough data
    // We can use this to implement flow control if needed
}