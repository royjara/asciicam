# Image to ASCII Converter

Real-time image to ASCII conversion using GStreamer and C++.

## Features

- Real-time video to ASCII conversion
- RGB buffer input support
- Configurable output dimensions
- GStreamer pipeline integration
- Processing prototype for algorithm verification
- Android app with camera capture and RTSP streaming

## Prerequisites (macOS with Homebrew)

```bash
brew install cmake gstreamer gst-plugins-base gst-plugins-good gst-plugins-bad gst-plugins-ugly
```

## Building

### Desktop (macOS/Linux)
```bash
./build.sh
```

### Android
1. Download GStreamer Android universal binaries from https://gstreamer.freedesktop.org/download/ and extract to the project root directory (e.g., `gstreamer-1.0-android-universal-1.28.0/`)
2. Build the Android project:
```bash
cd android
./gradlew assembleDebug
```

## Running

```bash
./build/img2ascii
```

## Processing Prototype

Test the ASCII conversion algorithm in Processing:

1. Open `prototype/img2ascii.pde` in Processing IDE
2. Press 'r' to convert with test image
3. Press 't' to test RGB buffer conversion

## Pipeline Examples

Default (test video source):
```
videotestsrc pattern=ball ! videoconvert ! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 ! appsink name=appsink
```

Webcam input:
```
avfvideosrc ! videoconvert ! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 ! appsink name=appsink
```