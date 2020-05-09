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

class sphere_geometry
{
public:
    sphere_geometry()
    {
        initialize_geometry();
        geometry_.set_data(verts_, indices_);
    }

    void render() const
    {
        geometry_.bind();
        glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, nullptr);
    }

private:
    void initialize_geometry()
    {
        constexpr auto Rings = 450;
        constexpr auto Slices = 20;
        constexpr auto Turns = 3.0;

        for (int i = 0; i < Rings; ++i) {
            const auto big_radius = powf(1.0005, 10.5 * i) * i * 0.0001; // /* static_cast<float>(i) * 0.0048 */;
            for (int j = 0; j < Slices; ++j) {
                const auto small_radius = big_radius * .45; // /* sqrtf(static_cast<float>(i)) * */ i * 0.00025;

                const auto phi = (static_cast<double>(i) / Rings) * 2.0 * M_PI * Turns;
                const auto theta = (static_cast<double>(j) / Slices) * 2.0 * M_PI;

                const auto r = glm::mat3(std::cos(phi), std::sin(phi), 0, -std::sin(phi), std::cos(phi), 0, 0, 0, 1);
                const auto p = r * glm::vec3(0, big_radius + small_radius * std::cos(theta), small_radius * std::sin(theta));
                const auto o = r * glm::vec3(0, big_radius, 0);

                const auto uv = glm::vec2(static_cast<float>(i) / Rings, static_cast<float>(j) / Slices);

                verts_.emplace_back(p, glm::normalize(p - o), uv);
            }
        }

        for (int i = 0; i < Rings - 1; ++i) {
            for (int j = 0; j < Slices; ++j) {
                const auto i0 = i * Slices + j;
                const auto i1 = (i + 1) * Slices + j;
                const auto i2 = (i + 1) * Slices + (j + 1) % Slices;
                const auto i3 = i * Slices + (j + 1) % Slices;

                indices_.push_back(i0);
                indices_.push_back(i1);
                indices_.push_back(i2);

                indices_.push_back(i2);
                indices_.push_back(i3);
                indices_.push_back(i0);
            }
        }
    }

    using vertex = std::tuple<glm::vec3, glm::vec3, glm::vec2>; // position, normal, texuv
    std::vector<vertex> verts_;
    std::vector<GLuint> indices_;
    gl::geometry geometry_;
};

class demo
{
public:
    demo(int window_width, int window_height)
        : window_width_(window_width)
        , window_height_(window_height)
        , sphere_(new sphere_geometry)
    {
        initialize_shader();
    }

    void render_and_step(float dt)
    {
        render(program_);
        cur_time_ += dt;
    }

private:
    void initialize_shader()
    {
        program_.add_shader(GL_VERTEX_SHADER, "shaders/sphere.vert");
        program_.add_shader(GL_FRAGMENT_SHADER, "shaders/sphere.frag");
        program_.link();
    }

    void render(const gl::shader_program &program) const
    {
        glViewport(0, 0, window_width_, window_height_);
        glClearColor(0.5, 0.5, 0.5, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_MULTISAMPLE);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_CULL_FACE);

        const auto projection =
                glm::perspective(glm::radians(45.0f), static_cast<float>(window_width_) / window_height_, 0.1f, 100.f);
        const auto view_pos = glm::vec3(0, -0.04, 0.3);
        const auto view_up = glm::vec3(0, 1, 0);
        const auto view = glm::lookAt(view_pos, glm::vec3(0, 0, 0), view_up);

        const float angle = cur_time_ * 2.f * M_PI / CycleDuration;
        const auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1));
        const auto mvp = projection * view * model;

        glm::mat3 model_normal(model);
        model_normal = glm::inverse(model_normal);
        model_normal = glm::transpose(model_normal);

        program.bind();

        constexpr const auto LocationMvp = 0;
        constexpr const auto LocationNormalMatrix = 1;
        constexpr const auto LocationModelMatrix = 2;
        constexpr const auto LocationGlobalLight = 3;
        constexpr const auto LocationUvOffset = 4;

        program.set_uniform(LocationMvp, mvp);
        program.set_uniform(LocationNormalMatrix, model_normal);
        program.set_uniform(LocationModelMatrix, model * view);
        program.set_uniform(LocationGlobalLight, glm::vec3(5, 7, 5));
        const auto a = static_cast<float>(cur_time_) / CycleDuration; // sinf(cur_time_ * 2.f * M_PI / cycle_duration);
        program.set_uniform(LocationUvOffset, glm::vec2(-a, a));

        glCullFace(GL_FRONT);
        sphere_->render();

        glCullFace(GL_BACK);
        sphere_->render();
    }

    int window_width_;
    int window_height_;
    float cur_time_ = 0;
    gl::shader_program program_;
    std::unique_ptr<sphere_geometry> sphere_;
};

int main()
{
    constexpr auto window_width = 512;
    constexpr auto window_height = 512;

    gl::window w(window_width, window_height, "demo");

    glewInit();

    glfwSetKeyCallback(w, [](GLFWwindow *window, int key, int scancode, int action, int mode) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
    });

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(
        [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar *message,
           const void * /*user*/) { std::cout << source << ':' << type << ':' << severity << ':' << message << '\n'; },
        nullptr);

#ifdef DUMP_FRAMES
    constexpr auto total_frames = CycleDuration * FramesPerSecond;
    auto frame_num = 0;
#endif

    {
        demo d(window_width, window_height);

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
