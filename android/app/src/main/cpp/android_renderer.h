#pragma once

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window.h>
#include <string>

class AndroidRenderer {
public:
    AndroidRenderer();
    ~AndroidRenderer();

    bool initialize(ANativeWindow* window);
    void shutdown();
    bool isReady() const { return initialized_; }
    bool makeCurrent();
    void releaseContext();

    void clear();
    void renderText(const std::string& text, float x, float y, float scale = 1.0f);
    void swapBuffers();
    void setColor(float r, float g, float b, float a = 1.0f);

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    EGLDisplay display_;
    EGLConfig config_;
    EGLContext context_;
    EGLSurface surface_;
    ANativeWindow* window_;

    int width_;
    int height_;
    bool initialized_;

    GLuint shader_program_;
    GLuint vbo_;
    GLuint texture_atlas_;

    float color_r_, color_g_, color_b_, color_a_;

    bool initializeEGL();
    bool createShaders();
    void createCharacterAtlas();
    void renderCharacter(char c, float x, float y, float scale);
};