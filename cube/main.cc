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

constexpr const auto CycleDuration = 4.f;
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
        geometry_.set_data(verts_);
    }

    void render() const
    {
        geometry_.bind();
        glDrawArrays(GL_TRIANGLES, 0, verts_.size());
    }

private:
    void initialize_geometry()
    {
        const auto add_face = [this](const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3) {
            const auto u = v1 - v0;
            const auto v = v3 - v0;
            const auto n = glm::normalize(glm::cross(u, v));

            verts_.emplace_back(v0, n);
            verts_.emplace_back(v1, n);
            verts_.emplace_back(v2, n);

            verts_.emplace_back(v2, n);
            verts_.emplace_back(v3, n);
            verts_.emplace_back(v0, n);
        };

        const glm::vec3 v0(-1, -1, -1);
        const glm::vec3 v1(-1,  1, -1);
        const glm::vec3 v2( 1,  1, -1);
        const glm::vec3 v3( 1, -1, -1);

        const glm::vec3 v4(-1, -1,  1);
        const glm::vec3 v5(-1,  1,  1);
        const glm::vec3 v6( 1,  1,  1);
        const glm::vec3 v7( 1, -1,  1);

        add_face(v0, v1, v2, v3);
        add_face(v1, v5, v6, v2);
        add_face(v0, v4, v5, v1);
        add_face(v7, v4, v0, v3);
        add_face(v7, v3, v2, v6);
        add_face(v4, v7, v6, v5);
    }

    using vertex = std::tuple<glm::vec3, glm::vec3>; // position, normal, texuv
    std::vector<vertex> verts_;
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
#if 1
        glClearColor(0.75, 0.75, 0.75, 0);
#else
        glClearColor(1, 1, 1, 0);
#endif
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_MULTISAMPLE);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_CULL_FACE);

        const auto projection =
                glm::perspective(glm::radians(45.0f), static_cast<float>(window_width_) / window_height_, 0.1f, 100.f);
        const auto view_pos = glm::vec3(2.5, -2.5, 2.5);
        const auto view_up = glm::vec3(0, 1, 0);
        const auto view = glm::lookAt(view_pos, glm::vec3(0, 0, 0), view_up);

#if 1
        const float angle = 0.3f * cosf(cur_time_ * 2.f * M_PI / CycleDuration);
        const auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(-1, 1, 1));
#else
        const auto model = glm::identity<glm::mat4x4>();
#endif
        const auto mvp = projection * view * model;

        glm::mat3 model_normal(model);
        model_normal = glm::inverse(model_normal);
        model_normal = glm::transpose(model_normal);

        program.bind();
        program.set_uniform(program.uniform_location("mvp"), mvp);
        program.set_uniform(program.uniform_location("normalMatrix"), model_normal);
        program.set_uniform(program.uniform_location("modelMatrix"), model * view);
        program.set_uniform(program.uniform_location("global_light"), glm::vec3(5, -5, 5));

        {
#if 1
        const float angle = 0.3f * sinf(cur_time_ * 2.f * M_PI / CycleDuration);
#else
        const float angle = 0.3f * cur_time_ * 2.f * M_PI / CycleDuration;
#endif
        const auto texture_transform = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1, 1, 1));
        program.set_uniform(program.uniform_location("texture_transform"), texture_transform);
        }

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
