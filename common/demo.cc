#include "demo.h"

#include "util.h"
#include "window.h"

#include <unistd.h>
#include <cstdlib>
#include <GLFW/glfw3.h>

namespace gl {

demo::demo(int argc, char *argv[])
{
    parse_arguments(argc, argv);
    window_.reset(new window(width_, height_, "demo"));
    glfwSetKeyCallback(*window_, [](GLFWwindow *window, int key, int scancode, int action, int mode) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
    });
}

demo::~demo() = default;

void demo::run()
{
    int frame_num = 0;

    double cur_time = glfwGetTime();
    while (!glfwWindowShouldClose(*window_)) {
        float elapsed;
        if (!dump_frames_) {
            auto now = glfwGetTime();
            elapsed = now - cur_time;
            cur_time = now;
        } else {
            elapsed = 1.0f / frames_per_second_;
        }

        render();
        update(elapsed);

        if (dump_frames_) {
            char path[80];
            std::sprintf(path, "%05d.ppm", frame_num);
            dump_frame_to_file(path, window_->width(), window_->height());
            if (++frame_num == cycle_duration_ * frames_per_second_)
                break;
        }

        glfwSwapBuffers(*window_);
        glfwPollEvents();
    }
}

void demo::parse_arguments(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "w:h:c:f:d")) != -1) {
        switch (opt)
        {
        case 'w':
            width_ = std::atoi(optarg);
            break;
        case 'h':
            height_ = std::atoi(optarg);
            break;
        case 'c':
            cycle_duration_ = std::atoi(optarg);
            break;
        case 'f':
            frames_per_second_ = std::atoi(optarg);
            break;
        case 'd':
            dump_frames_ = true;
            break;
        }
    }
}

}
