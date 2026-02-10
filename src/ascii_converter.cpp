#include "ascii_converter.h"
#include <algorithm>
#include <cmath>

AsciiConverter::AsciiConverter(int output_width, int output_height)
    : output_width_(output_width)
    , output_height_(output_height)
    , ascii_chars_(" .:-=+*#%@") {
}

void AsciiConverter::setOutputSize(int width, int height) {
    output_width_ = width;
    output_height_ = height;
}

void AsciiConverter::setAsciiChars(const std::string& chars) {
    ascii_chars_ = chars;
}

uint8_t AsciiConverter::rgbToGray(uint8_t r, uint8_t g, uint8_t b) const {
    return static_cast<uint8_t>(0.299 * r + 0.587 * g + 0.114 * b);
}

char AsciiConverter::grayToAscii(uint8_t gray) const {
    int char_index = (gray * (ascii_chars_.length() - 1)) / 255;
    char_index = std::clamp(char_index, 0, static_cast<int>(ascii_chars_.length() - 1));
    return ascii_chars_[char_index];
}

std::string AsciiConverter::convertRGBBuffer(const uint8_t* rgb_buffer, int width, int height) {
    std::string result;
    result.reserve(output_height_ * (output_width_ + 1));

    float scale_x = static_cast<float>(width) / output_width_;
    float scale_y = static_cast<float>(height) / output_height_;

    for (int y = 0; y < output_height_; y++) {
        for (int x = 0; x < output_width_; x++) {
            int src_x = static_cast<int>(x * scale_x);
            int src_y = static_cast<int>(y * scale_y);

            if (src_x < width && src_y < height) {
                int pixel_index = (src_y * width + src_x) * 3;

                uint8_t r = rgb_buffer[pixel_index];
                uint8_t g = rgb_buffer[pixel_index + 1];
                uint8_t b = rgb_buffer[pixel_index + 2];

                uint8_t gray = rgbToGray(r, g, b);
                char ascii_char = grayToAscii(gray);

                result += ascii_char;
            } else {
                result += ' ';
            }
        }
        result += '\n';
    }

    return result;
}

AsciiLayers AsciiConverter::convertRGBBufferToLayers(const uint8_t* rgb_buffer, int width, int height) {
    AsciiLayers layers;
    layers.red_layer = convertSingleChannel(rgb_buffer, width, height, 0);    // Red channel
    layers.green_layer = convertSingleChannel(rgb_buffer, width, height, 1);  // Green channel
    layers.blue_layer = convertSingleChannel(rgb_buffer, width, height, 2);   // Blue channel
    return layers;
}

std::string AsciiConverter::convertSingleChannel(const uint8_t* rgb_buffer, int width, int height, int channel) {
    std::string result;
    result.reserve(output_height_ * (output_width_ + 1));

    float scale_x = static_cast<float>(width) / output_width_;
    float scale_y = static_cast<float>(height) / output_height_;

    for (int y = 0; y < output_height_; y++) {
        for (int x = 0; x < output_width_; x++) {
            int src_x = static_cast<int>(x * scale_x);
            int src_y = static_cast<int>(y * scale_y);

            if (src_x < width && src_y < height) {
                int pixel_index = (src_y * width + src_x) * 3;
                uint8_t channel_value = rgb_buffer[pixel_index + channel];
                char ascii_char = grayToAscii(channel_value);
                result += ascii_char;
            } else {
                result += ' ';
            }
        }
        result += '\n';
    }

    return result;
}