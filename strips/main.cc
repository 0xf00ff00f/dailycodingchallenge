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

struct Bezier
{
    glm::vec3 p0, p1, p2;

    glm::vec3 position(float t) const
    {
        return (1.0f - t) * (1.0f - t) * p0 + 2.0f * (1.0f - t) * t * p1 + t * t * p2;
    }

    glm::vec3 direction(float t) const
    {
        return (t - 1.0f) * p0 + (1.0f - 2.0f * t) * p1 + t * p2;
    }
};

using Path = std::vector<Bezier>;

class StripGeometry
{
public:
    StripGeometry(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2)
        : path_{ p0, p1, p2 }
    {
        initialize();
        geometry_.set_data(verts_);
    }

    void render() const
    {
        geometry_.bind();
        glDrawArrays(GL_LINE_STRIP, 0, verts_.size());
    }

private:
    void initialize()
    {
        constexpr const auto NumPoints = 20;
        for (int i = 0; i < NumPoints; ++i)
        {
            const auto t = static_cast<float>(i) / (NumPoints - 1);
            verts_.push_back({ path_.position(t), glm::normalize(path_.direction(t)) });
        }
    }

    using vertex = std::tuple<glm::vec3, glm::vec3>; // position, direction
    std::vector<vertex> verts_;
    gl::geometry geometry_;
    Bezier path_;
};

class demo
{
public:
    demo(int window_width, int window_height)
        : window_width_(window_width)
        , window_height_(window_height)
        , strip_(new StripGeometry(glm::vec3(-1, -1, -1), glm::vec3(0, 0, 2), glm::vec3(1, 1, -1)))
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

        glDisable(GL_CULL_FACE);

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

        strip_->render();
    }

    int window_width_;
    int window_height_;
    float cur_time_ = 0;
    gl::shader_program program_;
    std::unique_ptr<StripGeometry> strip_;
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
