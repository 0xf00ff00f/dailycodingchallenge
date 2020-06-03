#include "panic.h"

#include <window.h>
#include <demo.h>
#include <geometry.h>
#include <shader_program.h>
#include <shadow_buffer.h>
#include <util.h>
#include <tween.h>

#include "blur_effect.h"

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <iostream>
#include <memory>

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

glm::vec3 rand_point()
{
    const auto r = [] {
        return 2.0f * static_cast<float>(std::rand()) / RAND_MAX - 1.0f;
    };
    return glm::vec3(r(), r(), r()) * 2.5f;
}

class Demo : public gl::demo
{
public:
    static constexpr const auto SegmentPoints = 20;

    struct Edge
    {
        static constexpr const auto ControlPointCount = 5;
        static constexpr const auto NumStates = 4;
        std::array<std::array<glm::vec3, NumStates>, ControlPointCount> control_points;
    };

    using Vertex = std::tuple<glm::vec3>;

    Demo(int argc, char *argv[])
        : gl::demo(argc, argv)
    {
        geometry_.set_data(std::vector<Vertex>(Edge::ControlPointCount * SegmentPoints));

        const auto v0 = glm::vec3(-1, -1, 1);
        const auto v1 = glm::vec3(-1, 1, 1);
        const auto v2 = glm::vec3(1, 1, 1);
        const auto v3 = glm::vec3(1, -1, 1);

        const auto v4 = glm::vec3(-1, -1, -1);
        const auto v5 = glm::vec3(-1, 1, -1);
        const auto v6 = glm::vec3(1, 1, -1);
        const auto v7 = glm::vec3(1, -1, -1);

        edges_.push_back(make_edge(v0, v1));
        edges_.push_back(make_edge(v1, v2));
        edges_.push_back(make_edge(v2, v3));
        edges_.push_back(make_edge(v3, v0));

        edges_.push_back(make_edge(v4, v5));
        edges_.push_back(make_edge(v5, v6));
        edges_.push_back(make_edge(v6, v7));
        edges_.push_back(make_edge(v7, v4));

        edges_.push_back(make_edge(v4, v0));
        edges_.push_back(make_edge(v5, v1));
        edges_.push_back(make_edge(v6, v2));
        edges_.push_back(make_edge(v7, v3));

        initialize_shader();

        blur_.reset(new blur_effect(width_, height_));
    }

private:
    void initialize_shader()
    {
        program_.add_shader(GL_VERTEX_SHADER, "shaders/simple.vert");
        program_.add_shader(GL_FRAGMENT_SHADER, "shaders/simple.frag");
        program_.link();
    }

    void update(float dt) override
    {
        cur_time_ += dt;
    }

    void render() override
    {
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_MULTISAMPLE);

        glViewport(0, 0, width_, height_);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const auto projection =
                glm::perspective(glm::radians(45.0f), static_cast<float>(width_) / height_, 0.1f, 100.f);
        const auto eye = glm::vec3(0, 4, 4);
        const auto center = glm::vec3(0, 0, 0);
        const auto up = glm::vec3(0, 1, 0);
        const auto view = glm::lookAt(eye, center, up);

#if 1
        const float angle = 0.5f * cosf(cur_time_ * 2.f * M_PI / cycle_duration_);
        // const float angle = cur_time_ * 2.f * M_PI / cycle_duration_;
        const auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));
#else
        auto model = glm::mat4(1.0f); // glm::translate(glm::mat4(1.0f), glm::vec3(0, 1.5, 0));
#endif

        program_.bind();
        program_.set_uniform("mvp", projection * view * model);
        program_.set_uniform("color", glm::vec4(1, 0, 0, 1));

        blur_->bind();

        glEnable(GL_LINE_SMOOTH);
        glLineWidth(8.0);

        glViewport(0, 0, blur_->width(), blur_->height());
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        render_scene();
        gl::framebuffer::unbind();

        glLineWidth(2.0);
        program_.set_uniform("color", glm::vec4(1, 1, 1, 1));

        glViewport(0, 0, width_, height_);
        glClearColor(0.25, 0.25, 0.25, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        render_scene();

        blur_->render(width_, height_, 2);
    }

    void render_scene()
    {
        const float t = [this]() -> float {
            const auto TransitionDuration = 0.15 * cycle_duration_;
            const auto T0 = 0.25 * cycle_duration_;
            const auto T1 = 0.75 * cycle_duration_;
            float time = fmod(cur_time_, cycle_duration_);
            if (time < T0) {
                return 0;
            } else if (time < T0 + TransitionDuration) {
                return in_quadratic((time - T0) / TransitionDuration);
            } else if (time < T1) {
                return 1;
            } else if (time < T1 + TransitionDuration) {
                return out_quadratic(1 - (time - T1) / TransitionDuration);
            } else {
                return 0;
            }
        }();
        for (const auto &edge : edges_)
            render_edge(edge, 0, 1, t);
    }

    void render_edge(const Edge &edge, int prev_state, int next_state, float t)
    {
        glBindBuffer(GL_ARRAY_BUFFER, geometry_.array_buffer_handle());
        auto *verts = static_cast<Vertex *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

        for (int i = 0; i < Edge::ControlPointCount; ++i)
        {
            const auto &c0 = edge.control_points[std::max(i - 1, 0)];
            const auto &c1 = edge.control_points[i];
            const auto &c2 = edge.control_points[std::min(i + 1, Edge::ControlPointCount - 1)];

            const auto &v0 = c0[prev_state] + t * (c0[next_state] - c0[prev_state]);
            const auto &v1 = c1[prev_state] + t * (c1[next_state] - c1[prev_state]);
            const auto &v2 = c2[prev_state] + t * (c2[next_state] - c2[prev_state]);

            Bezier b{0.5f * (v0 + v1), v1, 0.5f * (v1 + v2)};
            for (int j = 0; j < SegmentPoints; ++j)
            {
                const auto t = static_cast<float>(j) / (SegmentPoints - 1);
                *verts++ = b.position(t);
            }
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        geometry_.bind();
        glDrawArrays(GL_LINE_STRIP, 0, Edge::ControlPointCount * SegmentPoints);
    }

    Edge make_edge(const glm::vec3 &v0, const glm::vec3 &v1) const
    {
        Edge e;
        for (int i = 0; i < Edge::ControlPointCount; ++i)
        {
            const auto t = static_cast<float>(i) / (Edge::ControlPointCount - 1);
            e.control_points[i][0] = v0 + t * (v1 - v0);
            for (int j = 1; j < Edge::NumStates; ++j)
                e.control_points[i][j] = rand_point();
        }
        return e;
    }

    float cur_time_;
    gl::geometry geometry_;
    std::vector<Edge> edges_;
    gl::shader_program program_;
    std::unique_ptr<blur_effect> blur_;
};

int main(int argc, char *argv[])
{
    Demo d(argc, argv);
    d.run();
}
