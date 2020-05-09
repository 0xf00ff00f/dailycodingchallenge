#pragma once

#include "noncopyable.h"

struct GLFWwindow;

namespace gl {

class window : private noncopyable
{
public:
    window(int width, int height, const char *title);
    ~window();

    int width() const { return width_; }
    int height() const { return height_; }
    operator GLFWwindow *() const { return window_; }

private:
    int width_;
    int height_;
    GLFWwindow *window_;
};

} // namespace gl
