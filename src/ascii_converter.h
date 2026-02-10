#pragma once

#include <vector>
#include <string>

struct AsciiLayers {
    std::string red_layer;
    std::string green_layer;
    std::string blue_layer;
};

class AsciiConverter {
public:
    AsciiConverter(int output_width = 120, int output_height = 40);

    std::string convertRGBBuffer(const uint8_t* rgb_buffer, int width, int height);
    AsciiLayers convertRGBBufferToLayers(const uint8_t* rgb_buffer, int width, int height);

    void setOutputSize(int width, int height);
    void setAsciiChars(const std::string& chars);

private:
    int output_width_;
    int output_height_;
    std::string ascii_chars_;

    uint8_t rgbToGray(uint8_t r, uint8_t g, uint8_t b) const;
    char grayToAscii(uint8_t gray) const;
    std::string convertSingleChannel(const uint8_t* rgb_buffer, int width, int height, int channel);
};