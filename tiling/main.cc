#include "panic.h"

#include "window.h"
#include "geometry.h"
#include "shader_program.h"
#include "util.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <iostream>
#include <memory>

// #define DUMP_FRAMES

constexpr const auto CycleDuration = 3.f;
#ifdef DUMP_FRAMES
constexpr const auto FramesPerSecond = 40;
#else
constexpr const auto FramesPerSecond = 60;
#endif

class TileGeometry
{
public:
    TileGeometry()
    {
        initialize();
        geometry_.set_data(verts_);
    }

    void render() const
    {
        geometry_.bind();
        glDrawArrays(GL_LINE_LOOP, 0, verts_.size());
    }

private:
    void initialize()
    {
        constexpr auto NumSides = 6;
        for (int i = 0; i < NumSides; ++i)
        {
            const float a = static_cast<float>(i) * 2.0 * M_PI / NumSides;
            verts_.emplace_back(glm::vec2(std::cos(a), std::sin(a)));
        }
    }

    using vertex = std::tuple<glm::vec2>;
    std::vector<vertex> verts_;
    gl::geometry geometry_;
};

class Demo
{
public:
    Demo(int window_width, int window_height)
        : window_width_(window_width)
        , window_height_(window_height)
        , tile_(new TileGeometry)
    {
        initialize_shader();
    }

    void render_and_step(float dt)
    {
        render();
        cur_time_ += dt;
    }

private:
    void initialize_shader()
    {
        program_.add_shader(GL_VERTEX_SHADER, "shaders/tile.vert");
        program_.add_shader(GL_GEOMETRY_SHADER, "shaders/tile.geom");
        program_.add_shader(GL_FRAGMENT_SHADER, "shaders/tile.frag");
        program_.link();
    }

    void render() const
    {
        const auto light_position = glm::vec3(-3, -3, 7);

#if 0
        const float angle = -cur_time_ * 2.f * M_PI / CycleDuration;
        const auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1, 0, 0));
#else
        const auto model = glm::mat4(1.0);
#endif

        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);

        glViewport(0, 0, window_width_, window_height_);
        glClearColor(0, 0, 0, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        const auto projection =
                glm::perspective(glm::radians(45.0f), static_cast<float>(window_width_) / window_height_, 0.1f, 100.f);
        const auto view = glm::lookAt(glm::vec3(-4, -4, 7), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        const auto mvp = projection * view * model;

        program_.bind();
        program_.set_uniform("mvp", mvp);
        program_.set_uniform("modelMatrix", model);
        program_.set_uniform("lightPosition", light_position);
        program_.set_uniform("color", glm::vec3(1.0));
        program_.set_uniform("height", static_cast<float>(2.0 + 0.5 * glm::sin(2.0 * cur_time_)));

        tile_->render();
    }

    static constexpr auto NumStrips = 3;

    static constexpr auto ShadowWidth = 2048;
    static constexpr auto ShadowHeight = ShadowWidth;

    int window_width_;
    int window_height_;
    float cur_time_ = 0;
    gl::shader_program program_;
    std::unique_ptr<TileGeometry> tile_;
};

int main()
{
    constexpr auto window_width = 800;
    constexpr auto window_height = 800;

    gl::window w(window_width, window_height, "demo");

    glfwSetKeyCallback(w, [](GLFWwindow *window, int key, int scancode, int action, int mode) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
    });

#ifdef DUMP_FRAMES
    constexpr auto total_frames = CycleDuration * FramesPerSecond;
    auto frame_num = 0;
#endif

    {
        Demo d(window_width, window_height);

#ifndef DUMP_FRAMES
        double curTime = glfwGetTime();
#endif
        while (!glfwWindowShouldClose(w)) {
#ifndef DUMP_FRAMES
            auto now = glfwGetTime();
            const auto dt = now - curTime;
            curTime = now;
#else
            constexpr auto dt = 1.0f / FramesPerSecond;
#endif
            d.render_and_step(dt);

#ifdef DUMP_FRAMES
            char path[80];
            std::sprintf(path, "%05d.ppm", frame_num);
            dump_frame_to_file(path, window_width, window_height);

            if (++frame_num == total_frames)
                break;
#endif

            glfwSwapBuffers(w);
            glfwPollEvents();
        }
    }
}
