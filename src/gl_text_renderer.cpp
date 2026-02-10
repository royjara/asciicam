#include "gl_text_renderer.h"
#include <iostream>
#include <cstring>

GLTextRenderer::GLTextRenderer(int window_width, int window_height)
    : window_width_(window_width)
    , window_height_(window_height)
    , char_width_(8.0f)
    , char_height_(12.0f)
    , color_r_(1.0f)
    , color_g_(1.0f)
    , color_b_(1.0f)
    , color_a_(1.0f) {
    characters_.resize(256);
}

GLTextRenderer::~GLTextRenderer() {
    for (auto& character : characters_) {
        if (character.texture_id != 0) {
            glDeleteTextures(1, &character.texture_id);
        }
    }
}

bool GLTextRenderer::initialize() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, window_width_, window_height_, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    createCharacterTextures();
    return true;
}

void GLTextRenderer::createCharacterTextures() {
    const int font_width = 8;
    const int font_height = 12;

    for (int c = 32; c < 127; c++) {
        unsigned char bitmap[font_width * font_height];
        memset(bitmap, 0, sizeof(bitmap));

        if (c == ' ') {
            for (int i = 0; i < font_width * font_height; i++) {
                bitmap[i] = 0;
            }
        }
        else if (c == '.') {
            bitmap[(font_height-2) * font_width + 3] = 255;
            bitmap[(font_height-2) * font_width + 4] = 255;
        }
        else if (c == ':') {
            bitmap[4 * font_width + 3] = 255;
            bitmap[4 * font_width + 4] = 255;
            bitmap[8 * font_width + 3] = 255;
            bitmap[8 * font_width + 4] = 255;
        }
        else if (c == '-') {
            for (int x = 1; x < 7; x++) {
                bitmap[6 * font_width + x] = 255;
            }
        }
        else if (c == '=') {
            for (int x = 1; x < 7; x++) {
                bitmap[5 * font_width + x] = 255;
                bitmap[7 * font_width + x] = 255;
            }
        }
        else if (c == '+') {
            for (int x = 1; x < 7; x++) {
                bitmap[6 * font_width + x] = 255;
            }
            for (int y = 3; y < 10; y++) {
                bitmap[y * font_width + 4] = 255;
            }
        }
        else if (c == '*') {
            bitmap[4 * font_width + 4] = 255;
            bitmap[5 * font_width + 2] = 255;
            bitmap[5 * font_width + 4] = 255;
            bitmap[5 * font_width + 6] = 255;
            bitmap[6 * font_width + 1] = 255;
            bitmap[6 * font_width + 4] = 255;
            bitmap[6 * font_width + 7] = 255;
            bitmap[7 * font_width + 2] = 255;
            bitmap[7 * font_width + 4] = 255;
            bitmap[7 * font_width + 6] = 255;
            bitmap[8 * font_width + 4] = 255;
        }
        else if (c == '#') {
            for (int y = 2; y < 10; y++) {
                bitmap[y * font_width + 2] = 255;
                bitmap[y * font_width + 5] = 255;
            }
            for (int x = 1; x < 7; x++) {
                bitmap[4 * font_width + x] = 255;
                bitmap[7 * font_width + x] = 255;
            }
        }
        else if (c == '%') {
            bitmap[2 * font_width + 1] = 255;
            bitmap[2 * font_width + 2] = 255;
            bitmap[3 * font_width + 1] = 255;
            bitmap[3 * font_width + 2] = 255;
            bitmap[3 * font_width + 4] = 255;
            bitmap[4 * font_width + 3] = 255;
            bitmap[5 * font_width + 3] = 255;
            bitmap[6 * font_width + 2] = 255;
            bitmap[7 * font_width + 4] = 255;
            bitmap[8 * font_width + 5] = 255;
            bitmap[8 * font_width + 6] = 255;
            bitmap[9 * font_width + 5] = 255;
            bitmap[9 * font_width + 6] = 255;
        }
        else if (c == '@') {
            for (int y = 2; y < 10; y++) {
                for (int x = 1; x < 7; x++) {
                    if ((y == 2 || y == 9) && (x > 1 && x < 6)) bitmap[y * font_width + x] = 255;
                    else if ((x == 1 || x == 6) && (y > 2 && y < 9)) bitmap[y * font_width + x] = 255;
                    else if (y == 5 && x > 2 && x < 6) bitmap[y * font_width + x] = 255;
                    else if (x == 4 && y > 5 && y < 8) bitmap[y * font_width + x] = 255;
                }
            }
        }
        else {
            for (int y = 0; y < font_height; y++) {
                for (int x = 0; x < font_width; x++) {
                    bitmap[y * font_width + x] = ((x + y) % 2) ? 255 : 0;
                }
            }
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, font_width, font_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        characters_[c].texture_id = texture;
        characters_[c].advance_x = font_width;
        characters_[c].advance_y = font_height;
    }
}

void GLTextRenderer::renderText(const std::string& text, float x, float y, float scale) {
    float current_x = x;
    float current_y = y;

    for (char c : text) {
        if (c == '\n') {
            current_x = x;
            current_y += char_height_ * scale;
            continue;
        }

        if (c >= 0 && c < 256 && characters_[c].texture_id != 0) {
            renderCharacter(c, current_x, current_y, scale);
        }

        current_x += char_width_ * scale;
    }
}

void GLTextRenderer::renderCharacter(char c, float x, float y, float scale) {
    if (c < 0 || c >= 256 || characters_[c].texture_id == 0) {
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, characters_[c].texture_id);

    glColor4f(color_r_, color_g_, color_b_, color_a_);

    float width = char_width_ * scale;
    float height = char_height_ * scale;

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x + width, y);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x + width, y + height);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + height);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void GLTextRenderer::clear() {
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLTextRenderer::setColor(float r, float g, float b, float a) {
    color_r_ = r;
    color_g_ = g;
    color_b_ = b;
    color_a_ = a;
}

void GLTextRenderer::setCharSize(float width, float height) {
    char_width_ = width;
    char_height_ = height;
}