#include "android_renderer.h"
#include <android/log.h>
#include <cstring>

#define LOG_TAG "AndroidRenderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void checkGLError(const char* op) {
    for (GLint error = glGetError(); error; error = glGetError()) {
        LOGE("after %s() glError (0x%x)", op, error);
    }
}

const char* vertex_shader_source = R"(
attribute vec2 a_position;
attribute vec2 a_texCoord;
varying vec2 v_texCoord;
uniform vec2 u_resolution;

void main() {
    // After rotation, width and height are swapped
    vec2 rotated_resolution = vec2(u_resolution.y, u_resolution.x);
    vec2 position = (a_position / rotated_resolution) * 2.0 - 1.0;
    position.y = -position.y;

    // 90 degree clockwise rotation: (x,y) -> (y,-x)
    float rotated_x = position.y;
    float rotated_y = -position.x;

    gl_Position = vec4(rotated_x, rotated_y, 0.0, 1.0);
    v_texCoord = a_texCoord;
}
)";

const char* fragment_shader_source = R"(
precision mediump float;
varying vec2 v_texCoord;
uniform sampler2D u_texture;
uniform vec4 u_color;

void main() {
    float alpha = texture2D(u_texture, v_texCoord).a;
    gl_FragColor = vec4(u_color.rgb, u_color.a * alpha);
}
)";

AndroidRenderer::AndroidRenderer()
    : display_(EGL_NO_DISPLAY)
    , config_(nullptr)
    , context_(EGL_NO_CONTEXT)
    , surface_(EGL_NO_SURFACE)
    , window_(nullptr)
    , width_(0)
    , height_(0)
    , initialized_(false)
    , shader_program_(0)
    , vbo_(0)
    , texture_atlas_(0)
    , color_r_(1.0f)
    , color_g_(1.0f)
    , color_b_(1.0f)
    , color_a_(1.0f) {
}

AndroidRenderer::~AndroidRenderer() {
    shutdown();
}

bool AndroidRenderer::initialize(ANativeWindow* window) {
    window_ = window;
    width_ = ANativeWindow_getWidth(window);
    height_ = ANativeWindow_getHeight(window);

    if (!initializeEGL()) {
        return false;
    }

    if (!createShaders()) {
        return false;
    }

    createCharacterAtlas();

    glEnable(GL_BLEND);
    checkGLError("glEnable");
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    checkGLError("glBlendFunc");
    glViewport(0, 0, width_, height_);
    checkGLError("glViewport");

    LOGI("Renderer initialized with dimensions: %dx%d", width_, height_);

    initialized_ = true;
    return true;
}

bool AndroidRenderer::initializeEGL() {
    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display_ == EGL_NO_DISPLAY) {
        LOGE("Failed to get EGL display");
        return false;
    }

    if (!eglInitialize(display_, nullptr, nullptr)) {
        LOGE("Failed to initialize EGL");
        return false;
    }

    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLint num_configs;
    if (!eglChooseConfig(display_, config_attribs, &config_, 1, &num_configs)) {
        LOGE("Failed to choose EGL config");
        return false;
    }

    surface_ = eglCreateWindowSurface(display_, config_, window_, nullptr);
    if (surface_ == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface");
        return false;
    }

    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, context_attribs);
    if (context_ == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context");
        return false;
    }

    if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
        LOGE("Failed to make EGL context current");
        return false;
    }

    return true;
}

bool AndroidRenderer::createShaders() {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
    glCompileShader(vertex_shader);

    GLint compiled;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint log_length;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            char* log = new char[log_length];
            glGetShaderInfoLog(vertex_shader, log_length, nullptr, log);
            LOGE("Vertex shader compilation failed: %s", log);
            delete[] log;
        }
        return false;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint log_length;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            char* log = new char[log_length];
            glGetShaderInfoLog(fragment_shader, log_length, nullptr, log);
            LOGE("Fragment shader compilation failed: %s", log);
            delete[] log;
        }
        return false;
    }

    shader_program_ = glCreateProgram();
    glAttachShader(shader_program_, vertex_shader);
    glAttachShader(shader_program_, fragment_shader);
    glLinkProgram(shader_program_);

    GLint linked;
    glGetProgramiv(shader_program_, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint log_length;
        glGetProgramiv(shader_program_, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            char* log = new char[log_length];
            glGetProgramInfoLog(shader_program_, log_length, nullptr, log);
            LOGE("Shader program linking failed: %s", log);
            delete[] log;
        }
        return false;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glGenBuffers(1, &vbo_);

    LOGI("Shaders compiled and linked successfully");
    return true;
}

void AndroidRenderer::createCharacterAtlas() {
    const int font_width = 8;
    const int font_height = 12;
    const int atlas_size = 512;

    unsigned char* atlas_data = new unsigned char[atlas_size * atlas_size];
    memset(atlas_data, 0, atlas_size * atlas_size);

    for (int c = 32; c < 127; c++) {
        int char_x = ((c - 32) % 16) * font_width * 2;
        int char_y = ((c - 32) / 16) * font_height * 2;

        unsigned char bitmap[font_width * font_height];
        memset(bitmap, 0, sizeof(bitmap));

        if (c == '.') {
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
        else if (c == '+') {
            for (int x = 1; x < 7; x++) {
                bitmap[6 * font_width + x] = 255;
            }
            for (int y = 3; y < 10; y++) {
                bitmap[y * font_width + 4] = 255;
            }
        }
        else if (c == '*') {
            bitmap[5 * font_width + 4] = 255;
            bitmap[6 * font_width + 2] = 255;
            bitmap[6 * font_width + 4] = 255;
            bitmap[6 * font_width + 6] = 255;
            bitmap[7 * font_width + 4] = 255;
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
        else if (c == '@') {
            for (int y = 2; y < 10; y++) {
                for (int x = 1; x < 7; x++) {
                    if ((y == 2 || y == 9) && (x > 1 && x < 6)) bitmap[y * font_width + x] = 255;
                    else if ((x == 1 || x == 6) && (y > 2 && y < 9)) bitmap[y * font_width + x] = 255;
                }
            }
        } else if (c != ' ') {
            for (int y = 0; y < font_height; y++) {
                for (int x = 0; x < font_width; x++) {
                    bitmap[y * font_width + x] = ((x + y) % 2) ? 200 : 0;
                }
            }
        }

        for (int y = 0; y < font_height; y++) {
            for (int x = 0; x < font_width; x++) {
                if (char_x + x < atlas_size && char_y + y < atlas_size) {
                    atlas_data[(char_y + y) * atlas_size + (char_x + x)] = bitmap[y * font_width + x];
                }
            }
        }
    }

    glGenTextures(1, &texture_atlas_);
    checkGLError("glGenTextures");
    glBindTexture(GL_TEXTURE_2D, texture_atlas_);
    checkGLError("glBindTexture");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, atlas_size, atlas_size, 0, GL_ALPHA, GL_UNSIGNED_BYTE, atlas_data);
    checkGLError("glTexImage2D");
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGLError("glTexParameteri");

    LOGI("Character atlas created successfully, texture ID: %u", texture_atlas_);

    delete[] atlas_data;
}

void AndroidRenderer::clear() {
    // Check if EGL context is current
    EGLContext current_context = eglGetCurrentContext();
    EGLDisplay current_display = eglGetCurrentDisplay();
    EGLSurface current_surface = eglGetCurrentSurface(EGL_DRAW);

    static bool logged_context = false;
    if (!logged_context) {
        LOGI("EGL Context check - Context: %p (ours: %p), Display: %p (ours: %p), Surface: %p (ours: %p)",
             current_context, context_, current_display, display_, current_surface, surface_);
        logged_context = true;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Black background
    glClear(GL_COLOR_BUFFER_BIT);
    checkGLError("clear");
}

void AndroidRenderer::renderText(const std::string& text, float x, float y, float scale) {
    float current_x = x;
    float current_y = y;
    const float char_width = 8.0f * scale;
    const float char_height = 12.0f * scale;

    static bool logged_text_info = false;
    if (!logged_text_info) {
        LOGI("Rendering text: length=%zu, pos=(%.1f,%.1f), scale=%.1f", text.length(), x, y, scale);
        logged_text_info = true;
    }

    for (char c : text) {
        if (c == '\n') {
            current_x = x;
            current_y += char_height;
            continue;
        }

        if (c >= 32 && c < 127) {
            renderCharacter(c, current_x, current_y, scale);
        }

        current_x += char_width;
    }
}

void AndroidRenderer::renderCharacter(char c, float x, float y, float scale) {
    if (c < 32 || c >= 127) return;

    static int render_count = 0;
    if (render_count < 5) {
        LOGI("renderCharacter: '%c' at (%.1f,%.1f) scale=%.1f", c, x, y, scale);
        render_count++;
    }

    const float font_width = 8.0f;
    const float font_height = 12.0f;
    const float atlas_size = 512.0f;

    float char_x = ((c - 32) % 16) * font_width * 2.0f / atlas_size;
    float char_y = ((c - 32) / 16) * font_height * 2.0f / atlas_size;
    float char_w = font_width / atlas_size;
    float char_h = font_height / atlas_size;

    float width = font_width * scale;
    float height = font_height * scale;

    float vertices[] = {
        x, y, char_x, char_y,
        x + width, y, char_x + char_w, char_y,
        x, y + height, char_x, char_y + char_h,
        x + width, y, char_x + char_w, char_y,
        x, y + height, char_x, char_y + char_h,
        x + width, y + height, char_x + char_w, char_y + char_h
    };

    glUseProgram(shader_program_);

    GLint position_attrib = glGetAttribLocation(shader_program_, "a_position");
    GLint texcoord_attrib = glGetAttribLocation(shader_program_, "a_texCoord");
    GLint resolution_uniform = glGetUniformLocation(shader_program_, "u_resolution");
    GLint color_uniform = glGetUniformLocation(shader_program_, "u_color");

    glUniform2f(resolution_uniform, width_, height_);
    glUniform4f(color_uniform, color_r_, color_g_, color_b_, color_a_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(position_attrib);
    glVertexAttribPointer(position_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glEnableVertexAttribArray(texcoord_attrib);
    glVertexAttribPointer(texcoord_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindTexture(GL_TEXTURE_2D, texture_atlas_);
    checkGLError("glBindTexture");
    glDrawArrays(GL_TRIANGLES, 0, 6);
    checkGLError("glDrawArrays");
}

void AndroidRenderer::swapBuffers() {
    if (display_ != EGL_NO_DISPLAY && surface_ != EGL_NO_SURFACE) {
        eglSwapBuffers(display_, surface_);
    }
}

void AndroidRenderer::setColor(float r, float g, float b, float a) {
    color_r_ = r;
    color_g_ = g;
    color_b_ = b;
    color_a_ = a;
}

bool AndroidRenderer::makeCurrent() {
    if (display_ == EGL_NO_DISPLAY || surface_ == EGL_NO_SURFACE || context_ == EGL_NO_CONTEXT) {
        LOGE("makeCurrent: Invalid EGL objects");
        return false;
    }

    if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
        LOGE("makeCurrent: eglMakeCurrent failed");
        return false;
    }

    return true;
}

void AndroidRenderer::releaseContext() {
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        LOGI("Released EGL context from current thread");
    }
}

void AndroidRenderer::shutdown() {
    if (texture_atlas_) {
        glDeleteTextures(1, &texture_atlas_);
        texture_atlas_ = 0;
    }
    if (vbo_) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }
    if (shader_program_) {
        glDeleteProgram(shader_program_);
        shader_program_ = 0;
    }

    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
        }

        eglTerminate(display_);
    }

    display_ = EGL_NO_DISPLAY;
    context_ = EGL_NO_CONTEXT;
    surface_ = EGL_NO_SURFACE;
    initialized_ = false;
}