#pragma once

#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraMetadata.h>
#include <media/NdkImageReader.h>
#include <functional>

class AndroidCamera {
public:
    using FrameCallback = std::function<void(const uint8_t*, int, int)>;

    AndroidCamera();
    ~AndroidCamera();

    bool initialize();
    bool start();
    void stop();
    void setFrameCallback(FrameCallback callback);

private:
    ACameraManager* camera_manager_;
    ACameraDevice* camera_device_;
    ACaptureSessionOutputContainer* output_container_;
    ACameraCaptureSession* capture_session_;
    AImageReader* image_reader_;
    ACaptureRequest* capture_request_;

    FrameCallback frame_callback_;
    const char* camera_id_;

    static void onImageAvailable(void* context, AImageReader* reader);
    static void onCameraStateChanged(void* context, ACameraDevice* device);
    static void onCameraError(void* context, ACameraDevice* device, int error);

    bool openCamera();
    bool createImageReader();
    bool createCaptureSession();
    void processImage(AImage* image);
};