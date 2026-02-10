#!/bin/bash

echo "Testing different GStreamer sources for img2ascii"
echo "================================================"

# Test different videotestsrc patterns
echo "1. Bouncing ball (default)"
echo "2. SMPTE color bars"
echo "3. Checkers pattern"
echo "4. Circular pattern"
echo "5. Webcam (if available)"
echo ""

read -p "Select test (1-5): " choice

case $choice in
    1)
        echo "Running bouncing ball..."
        ./build/img2ascii
        ;;
    2)
        echo "Running SMPTE color bars..."
        GST_DEBUG=2 gst-launch-1.0 videotestsrc pattern=smpte ! videoconvert ! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 ! appsink name=appsink emit-signals=true ! fakesink &
        sleep 1
        ./build/img2ascii
        ;;
    3)
        echo "Running checkers pattern..."
        # We'll modify the main.cpp to accept pipeline as argument
        echo "Note: Current version uses fixed pipeline. Modify main.cpp to accept different patterns."
        ./build/img2ascii
        ;;
    4)
        echo "Running circular pattern..."
        echo "Note: Current version uses fixed pipeline. Modify main.cpp to accept different patterns."
        ./build/img2ascii
        ;;
    5)
        echo "Testing webcam..."
        echo "Note: Webcam pipeline would be: avfvideosrc ! videoconvert ! video/x-raw,format=RGB,width=320,height=240,framerate=30/1 ! appsink name=appsink"
        echo "Modify main.cpp to use this pipeline for webcam input."
        ;;
    *)
        echo "Invalid selection"
        ;;
esac