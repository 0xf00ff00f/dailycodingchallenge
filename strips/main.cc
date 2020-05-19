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

float frand()
{
    return static_cast<float>(rand()) / RAND_MAX;
}

void subdivide(std::vector<glm::vec3> &points, const glm::vec3 &from, const glm::vec3 &to, int level)
{
    if (level > 0)
    {
        auto m = 0.5f * (from + to);

        // perturb
        const auto d = glm::normalize(to - from);
        const auto n = glm::vec3(0, 0, 1);
        const auto u = glm::cross(d, n);

        const auto perturbed = [&to, &from](float factor, const glm::vec3 &v) {
            const auto l = glm::distance(to, from);
            return factor * (2.0f * frand() - 1.0f) * l * v;
        };

        m += perturbed(0.25f, u) + perturbed(0.25f, n);

        subdivide(points, from, m, level - 1);
        subdivide(points, m, to, level - 1);
    }
    else
    {
        points.push_back(from);
    }
}

class StripGeometry
{
public:
    StripGeometry(float angle_offset, float coil_radius)
    {
        initialize(angle_offset, coil_radius);
        geometry_.set_data(verts_);
    }

    void render() const
    {
        geometry_.bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, verts_.size());
    }

private:
    void initialize(float angle_offset, float coil_radius)
    {
        std::vector<glm::vec3> control_points;

        constexpr auto CircleVerts = 5;
        constexpr auto CircleRadius = 1.0f;

        const auto vertex_at = [](int index) {
            const float angle = static_cast<float>(index) * 2.0 * M_PI / CircleVerts;
            const float s = std::sin(angle);
            const float c = std::cos(angle);
            return glm::vec3(c * CircleRadius, s * CircleRadius, 0);
        };

        for (int i = 0; i < CircleVerts; ++i)
        {
            const auto v0 = vertex_at(i);
            const auto v1 = vertex_at(i + 1);
            subdivide(control_points, v0, v1, 1);
        }

        std::vector<glm::vec3> path_points;

        const auto num_control_points = control_points.size();
        for (int i = 0; i < num_control_points; ++i)
        {
            const auto &va = control_points[(i + num_control_points - 1) % num_control_points];
            const auto &vb = control_points[i];
            const auto &vc = control_points[(i + 1) % num_control_points];

            const auto segment = Bezier{0.5f * (va + vb), vb, 0.5f * (vb + vc)};

            constexpr const auto PointsPerSegment = 60;
            constexpr const auto TurnsPerSegment = 1;

            for (int i = 0; i < PointsPerSegment; ++i)
            {
                const auto t = static_cast<float>(i) / PointsPerSegment;
                const auto p = segment.position(t);

                const auto d = glm::normalize(segment.direction(t));
                const auto s = glm::cross(d, glm::vec3(0, 0, 1));
                const auto n = glm::cross(s, d);

                const float a = angle_offset +
                    static_cast<float>(i) * TurnsPerSegment * 2.0 * M_PI / PointsPerSegment;

                // there's probably a cleaner way to do this with matrices but screw it
                const auto r = p + (std::cos(a) * s + std::sin(a) * n) * coil_radius;
                path_points.push_back(r);
            }
        }

        const auto num_path_points = path_points.size();
        for (int i = 0; i <= num_path_points; ++i)
        {
            const auto &v0 = path_points[i % num_path_points];
            const auto &v1 = path_points[(i + 1) % num_path_points];

            const auto d = glm::normalize(v1 - v0);
            const auto s = glm::cross(d, glm::vec3(0, 0, 1));
            const auto n = glm::cross(s, d);

            const auto t = static_cast<float>(i) / num_path_points;

            constexpr const auto TapeWidth = 0.025f;

            verts_.emplace_back(v0 - TapeWidth * s, n, glm::vec2(0, t));
            verts_.emplace_back(v0 + TapeWidth * s, n, glm::vec2(1, t));
        }
    }

    using vertex = std::tuple<glm::vec3, glm::vec3, glm::vec2>; // position, direction, uv
    std::vector<vertex> verts_;
    gl::geometry geometry_;
};

class demo
{
public:
    demo(int window_width, int window_height)
        : window_width_(window_width)
        , window_height_(window_height)
    {
        initialize_shader();

        for (int i = 0; i < NumStrips; ++i)
        {
            const float a = static_cast<float>(i) * 2.0f * M_PI / NumStrips;
            const auto coil_radius = 0.1f + frand() * 0.1f;
            strips_.emplace_back(new StripGeometry(a, coil_radius));

            auto &params = params_[i];
            params.speed = 0.1f + frand() * 0.3f;
            params.length = 02.f + frand() * 0.4f;
            params.color = glm::vec3(frand(), frand(), frand()) * 0.5f + glm::vec3(0.5f);
            // this sucks
        }
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
        glClearColor(0.75, 0.75, 0.75, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_MULTISAMPLE);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_CULL_FACE);

        const auto projection =
                glm::perspective(glm::radians(45.0f), static_cast<float>(window_width_) / window_height_, 0.1f, 100.f);
        const auto view_pos = glm::vec3(0, 0, 3);
        const auto view_up = glm::vec3(0, 1, 0);
        const auto view = glm::lookAt(view_pos, glm::vec3(0, 0, 0), view_up);

#if 1
        const float angle = 0.3f * cosf(cur_time_ * 2.f * M_PI / CycleDuration);
        const auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));
#else
        const auto model = glm::identity<glm::mat4x4>();
#endif
        const auto mvp = projection * view * model;

        glm::mat3 model_normal(model);
        model_normal = glm::inverse(model_normal);
        model_normal = glm::transpose(model_normal);

        program.bind();
        program.set_uniform("mvp", mvp);
        program.set_uniform("modelMatrix", model);
        program.set_uniform("lightPosition", glm::vec3(5, -5, 5));

        for (int i = 0; i < strips_.size(); ++i)
        {
            const auto &params = params_[i];

            program.set_uniform("color", params.color);

            auto v_start = fmod(cur_time_ * params.speed, 1.0f);
            auto v_end = fmod(v_start + params.length, 1.0f);
            program.set_uniform("vRange", glm::vec2(v_start, v_end));

            strips_[i]->render();
        }
    }

    static constexpr auto NumStrips = 1;

    int window_width_;
    int window_height_;
    float cur_time_ = 0;
    gl::shader_program program_;
    std::vector<std::unique_ptr<StripGeometry>> strips_;
    struct StripParams
    {
        glm::vec3 color;
        float speed;
        float length;
    };
    std::array<StripParams, NumStrips> params_;
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
