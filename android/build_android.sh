#!/bin/bash

# Build script for Android version

set -e

ANDROID_SDK="/Users/azucar/Library/Android/sdk"
ADB="$ANDROID_SDK/platform-tools/adb"

if [ ! -d "$ANDROID_SDK" ]; then
    echo "Error: Android SDK not found at $ANDROID_SDK"
    echo "Please set ANDROID_SDK path correctly"
    exit 1
fi

echo "Building Android APK..."
cd android

# Clean and build
./gradlew clean
./gradlew assembleDebug

echo "Build complete!"
echo "APK location: app/build/outputs/apk/debug/app-debug.apk"

# Check if device is connected
if $ADB devices | grep -q "device$"; then
    echo "Android device detected. Install APK? (y/n)"
    read -r response
    if [ "$response" = "y" ]; then
        echo "Installing APK..."
        $ADB install -r app/build/outputs/apk/debug/app-debug.apk
        echo "APK installed successfully!"
        echo "Launching app..."
        $ADB shell am start -n com.example.img2ascii/.MainActivity
    fi
else
    echo "No Android device connected. Connect device and run:"
    echo "$ADB install -r app/build/outputs/apk/debug/app-debug.apk"
fi