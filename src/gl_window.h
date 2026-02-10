#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include <memory>
#include "gl_text_renderer.h"

class GLWindow {
public:
    using FrameCallback = std::function<void()>;

    GLWindow(int width, int height, const std::string& title);
    ~GLWindow();

    bool initialize();
    bool shouldClose() const;
    void pollEvents();
    void swapBuffers();
    void setFrameCallback(FrameCallback callback);

    GLTextRenderer* getTextRenderer() { return text_renderer_.get(); }

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    GLFWwindow* window_;
    int width_;
    int height_;
    std::string title_;
    std::unique_ptr<GLTextRenderer> text_renderer_;
    FrameCallback frame_callback_;

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
};