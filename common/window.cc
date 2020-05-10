#include "window.h"

#include "panic.h"

#include <GLFW/glfw3.h>

namespace gl {

window::window(int width, int height, const char *title)
    : width_(width)
    , height_(height)
{
    glfwInit();
    glfwSetErrorCallback([](int error, const char *description) {
        panic("GLFW error %08x: %s\n", error, description);
    });

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 16);
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);
}

window::~window()
{
    glfwDestroyWindow(window_);
    glfwTerminate();
}

} // namespace gl
