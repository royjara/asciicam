#include "android_camera.h"
#include <android/log.h>
#include <media/NdkImage.h>

#define LOG_TAG "AndroidCamera"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AndroidCamera::AndroidCamera()
    : camera_manager_(nullptr)
    , camera_device_(nullptr)
    , output_container_(nullptr)
    , capture_session_(nullptr)
    , image_reader_(nullptr)
    , capture_request_(nullptr)
    , camera_id_(nullptr) {
}

AndroidCamera::~AndroidCamera() {
    stop();
}

bool AndroidCamera::initialize() {
    camera_manager_ = ACameraManager_create();
    if (!camera_manager_) {
        LOGE("Failed to create camera manager");
        return false;
    }

    ACameraIdList* camera_id_list = nullptr;
    camera_status_t result = ACameraManager_getCameraIdList(camera_manager_, &camera_id_list);

    if (result != ACAMERA_OK || !camera_id_list || camera_id_list->numCameras == 0) {
        LOGE("No cameras available");
        return false;
    }

    camera_id_ = camera_id_list->cameraIds[0];

    if (!createImageReader()) {
        return false;
    }

    if (!openCamera()) {
        return false;
    }

    return true;
}

bool AndroidCamera::createImageReader() {
    media_status_t result = AImageReader_new(320, 240, AIMAGE_FORMAT_YUV_420_888, 2, &image_reader_);

    if (result != AMEDIA_OK) {
        LOGE("Failed to create image reader");
        return false;
    }

    AImageReader_ImageListener listener;
    listener.context = this;
    listener.onImageAvailable = onImageAvailable;

    AImageReader_setImageListener(image_reader_, &listener);

    return true;
}

bool AndroidCamera::openCamera() {
    ACameraDevice_StateCallbacks device_callbacks = {};
    device_callbacks.context = this;
    device_callbacks.onDisconnected = onCameraStateChanged;
    device_callbacks.onError = onCameraError;

    camera_status_t result = ACameraManager_openCamera(camera_manager_, camera_id_, &device_callbacks, &camera_device_);

    if (result != ACAMERA_OK) {
        LOGE("Failed to open camera");
        return false;
    }

    return true;
}

bool AndroidCamera::createCaptureSession() {
    ANativeWindow* window = nullptr;
    media_status_t media_result = AImageReader_getWindow(image_reader_, &window);
    if (media_result != AMEDIA_OK) {
        LOGE("Failed to get window from image reader");
        return false;
    }

    ACameraOutputTarget* camera_output_target = nullptr;
    ACameraOutputTarget_create(window, &camera_output_target);

    ACaptureSessionOutput* session_output = nullptr;
    ACaptureSessionOutput_create(window, &session_output);

    ACaptureSessionOutputContainer_create(&output_container_);
    ACaptureSessionOutputContainer_add(output_container_, session_output);

    ACameraCaptureSession_stateCallbacks session_callbacks = {};

    camera_status_t result = ACameraDevice_createCaptureSession(camera_device_, output_container_, &session_callbacks, &capture_session_);

    if (result != ACAMERA_OK) {
        LOGE("Failed to create capture session");
        return false;
    }

    ACameraDevice_createCaptureRequest(camera_device_, TEMPLATE_PREVIEW, &capture_request_);
    ACaptureRequest_addTarget(capture_request_, camera_output_target);

    return true;
}

bool AndroidCamera::start() {
    if (!createCaptureSession()) {
        return false;
    }

    ACameraCaptureSession_setRepeatingRequest(capture_session_, nullptr, 1, &capture_request_, nullptr);

    return true;
}

void AndroidCamera::stop() {
    if (capture_session_) {
        ACameraCaptureSession_close(capture_session_);
        capture_session_ = nullptr;
    }

    if (camera_device_) {
        ACameraDevice_close(camera_device_);
        camera_device_ = nullptr;
    }

    if (output_container_) {
        ACaptureSessionOutputContainer_free(output_container_);
        output_container_ = nullptr;
    }

    if (image_reader_) {
        AImageReader_delete(image_reader_);
        image_reader_ = nullptr;
    }

    if (camera_manager_) {
        ACameraManager_delete(camera_manager_);
        camera_manager_ = nullptr;
    }
}

void AndroidCamera::setFrameCallback(FrameCallback callback) {
    frame_callback_ = callback;
}

void AndroidCamera::onImageAvailable(void* context, AImageReader* reader) {
    AndroidCamera* camera = static_cast<AndroidCamera*>(context);

    AImage* image = nullptr;
    media_status_t result = AImageReader_acquireLatestImage(reader, &image);

    if (result == AMEDIA_OK && image) {
        camera->processImage(image);
        AImage_delete(image);
    }
}

void AndroidCamera::processImage(AImage* image) {
    if (!frame_callback_) {
        return;
    }

    int32_t format;
    AImage_getFormat(image, &format);

    if (format != AIMAGE_FORMAT_YUV_420_888) {
        return;
    }

    int32_t width, height;
    AImage_getWidth(image, &width);
    AImage_getHeight(image, &height);

    int32_t num_planes;
    media_status_t result = AImage_getNumberOfPlanes(image, &num_planes);

    if (result != AMEDIA_OK || num_planes < 1) {
        return;
    }

    uint8_t* y_data = nullptr;
    int y_len = 0;
    result = AImage_getPlaneData(image, 0, &y_data, &y_len);

    if (result != AMEDIA_OK || !y_data) {
        return;
    }

    int32_t y_stride;
    AImage_getPlaneRowStride(image, 0, &y_stride);

    uint8_t* rgb_buffer = new uint8_t[width * height * 3];

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t luma = y_data[y * y_stride + x];

            int rgb_index = (y * width + x) * 3;
            rgb_buffer[rgb_index] = luma;
            rgb_buffer[rgb_index + 1] = luma;
            rgb_buffer[rgb_index + 2] = luma;
        }
    }

    frame_callback_(rgb_buffer, width, height);

    delete[] rgb_buffer;
}

void AndroidCamera::onCameraStateChanged(void* context, ACameraDevice* device) {
    LOGI("Camera state changed");
}

void AndroidCamera::onCameraError(void* context, ACameraDevice* device, int error) {
    LOGE("Camera error: %d", error);
}