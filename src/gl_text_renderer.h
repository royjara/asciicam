#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <vector>

class GLTextRenderer {
public:
    GLTextRenderer(int window_width, int window_height);
    ~GLTextRenderer();

    bool initialize();
    void renderText(const std::string& text, float x, float y, float scale = 1.0f);
    void clear();
    void setColor(float r, float g, float b, float a = 1.0f);
    void setCharSize(float width, float height);

private:
    int window_width_;
    int window_height_;
    float char_width_;
    float char_height_;
    float color_r_, color_g_, color_b_, color_a_;

    struct Character {
        unsigned int texture_id;
        float advance_x;
        float advance_y;
    };

    void createCharacterTextures();
    void renderCharacter(char c, float x, float y, float scale);
    std::vector<Character> characters_;
};