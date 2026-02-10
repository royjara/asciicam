#include "gl_window.h"
#include <iostream>

GLWindow::GLWindow(int width, int height, const std::string& title)
    : window_(nullptr)
    , width_(width)
    , height_(height)
    , title_(title) {
}

GLWindow::~GLWindow() {
    if (window_) {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

bool GLWindow::initialize() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
    if (!window_) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSetWindowUserPointer(window_, this);

    glfwSetKeyCallback(window_, keyCallback);
    glfwSetFramebufferSizeCallback(window_, framebufferSizeCallback);

    glfwSwapInterval(1);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    text_renderer_ = std::make_unique<GLTextRenderer>(width_, height_);
    if (!text_renderer_->initialize()) {
        std::cerr << "Failed to initialize text renderer" << std::endl;
        return false;
    }

    return true;
}

bool GLWindow::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

void GLWindow::pollEvents() {
    glfwPollEvents();
}

void GLWindow::swapBuffers() {
    glfwSwapBuffers(window_);
}

void GLWindow::setFrameCallback(FrameCallback callback) {
    frame_callback_ = callback;
}

void GLWindow::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    GLWindow* gl_window = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && gl_window->frame_callback_) {
        gl_window->frame_callback_();
    }
}

void GLWindow::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);

    GLWindow* gl_window = static_cast<GLWindow*>(glfwGetWindowUserPointer(window));
    gl_window->width_ = width;
    gl_window->height_ = height;

    if (gl_window->text_renderer_) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, width, height, 0.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }
}