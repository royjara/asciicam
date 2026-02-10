#!/bin/bash

# Manual APK build using Android SDK build tools

set -e

ANDROID_SDK="/Users/azucar/Library/Android/sdk"
BUILD_TOOLS="$ANDROID_SDK/build-tools/34.0.0"
PLATFORM_TOOLS="$ANDROID_SDK/platform-tools"
ANDROID_JAR="$ANDROID_SDK/platforms/android-34/android.jar"
NDK_BUILD="$ANDROID_SDK/ndk/25.1.8937393/ndk-build"

if [ ! -d "$ANDROID_SDK" ]; then
    echo "Error: Android SDK not found at $ANDROID_SDK"
    exit 1
fi

echo "Building native library with NDK..."
cd app/src/main/cpp
$NDK_BUILD NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk

echo "Compiling Java sources..."
cd ../../../..
mkdir -p build/classes
javac -d build/classes -classpath "$ANDROID_JAR" app/src/main/java/com/example/img2ascii/*.java

echo "Creating DEX file..."
"$BUILD_TOOLS/dx" --dex --output=build/classes.dex build/classes/

echo "Packaging APK..."
"$BUILD_TOOLS/aapt" package -f -m -J build/gen -S app/src/main/res -M app/src/main/AndroidManifest.xml -I "$ANDROID_JAR"

"$BUILD_TOOLS/aapt" package -f -M app/src/main/AndroidManifest.xml -S app/src/main/res -I "$ANDROID_JAR" -F build/img2ascii-unsigned.apk build/

echo "Adding DEX to APK..."
cd build
"$BUILD_TOOLS/aapt" add img2ascii-unsigned.apk classes.dex

echo "Signing APK..."
"$BUILD_TOOLS/apksigner" sign --ks ~/.android/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android --key-pass pass:android --out img2ascii-debug.apk img2ascii-unsigned.apk

echo "Installing APK..."
"$PLATFORM_TOOLS/adb" install -r img2ascii-debug.apk

echo "Launching app..."
"$PLATFORM_TOOLS/adb" shell am start -n com.example.img2ascii/.MainActivity

echo "Build and install complete!"