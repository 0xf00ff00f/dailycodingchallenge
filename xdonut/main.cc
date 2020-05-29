#include "panic.h"

#include "window.h"
#include "demo.h"
#include "geometry.h"
#include "shader_program.h"
#include "util.h"

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

class DonutGeometry
{
public:
    DonutGeometry()
    {
        geometry_.set_data(std::vector<Vertex>(VertexCount));
    }

    void render() const
    {
        geometry_.bind();
        glDrawArrays(GL_TRIANGLES, 0, VertexCount);
    }

    void update_verts(float angle_offset, float u_offset) const
    {
        glBindBuffer(GL_ARRAY_BUFFER, geometry_.array_buffer_handle());
        auto *verts = static_cast<Vertex *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

        for (int i = 0; i < NumSegmentsOuter; ++i)
        {
            for (int j = 0; j < NumSegmentsInner; ++j)
            {
                const auto vert_at = [angle_offset](int i, int j) -> std::tuple<glm::vec3, glm::vec3> {
                    const float a = static_cast<float>(i) * 2.0 * M_PI / NumSegmentsOuter;

                    float b = static_cast<float>(j) * 2.0 * M_PI / NumSegmentsInner + a + angle_offset;

                    const auto p = glm::vec3(cosf(a) * DonutRadius, sinf(a) * DonutRadius, 0.0);

                    const auto u = glm::vec3(cosf(a), sinf(a), 0.0);
                    const auto v = glm::vec3(0.0, 0.0, 1.0);

                    const auto pos = p + (sinf(b) * u + cosf(b) * v) * DonutSmallRadius;
                    const auto normal = glm::normalize(pos - p);

                    return {pos, normal};
                };

                const auto i1 = (i + 1) % NumSegmentsOuter;
                const auto j1 = (j + 1) % NumSegmentsInner;

                const auto [v0, n0] = vert_at(i, j);
                const auto [v1, n1] = vert_at(i1, j);
                const auto [v2, n2] = vert_at(i1, j1);
                const auto [v3, n3] = vert_at(i, j1);

                const auto na = glm::normalize(n0 + n3);
                const auto nb = glm::normalize(n1 + n2);

                const auto s0 = 4.0 * (static_cast<float>(i) / NumSegmentsOuter + u_offset);
                const auto s1 = 4.0 * (static_cast<float>(i + 1) / NumSegmentsOuter + u_offset);

                const auto t0 = static_cast<float>(j) / NumSegmentsInner;
                const auto t1 = static_cast<float>(j + 1) / NumSegmentsInner;

                *verts++ = {v0, na, {s0, t0}};
                *verts++ = {v1, nb, {s1, t0}};
                *verts++ = {v2, nb, {s1, t1}};

                *verts++ = {v2, nb, {s1, t1}};
                *verts++ = {v3, na, {s0, t1}};
                *verts++ = {v0, na, {s0, t0}};
            }
        }

        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

private:
    static constexpr float DonutRadius = 1.0f;
    static constexpr float DonutSmallRadius = 0.3f;

    static constexpr int NumSegmentsOuter = 128;
    static constexpr int NumSegmentsInner = 4;

    static constexpr int VertexCount = NumSegmentsOuter * NumSegmentsInner * 6;

    using Vertex = std::tuple<glm::vec3, glm::vec3, glm::vec2>; // position, normal, uv
    gl::geometry geometry_;
};

class Demo : public gl::demo
{
public:
    Demo(int argc, char *argv[])
        : gl::demo(argc, argv)
    {
        initialize_shader();
    }

private:
    void initialize_shader()
    {
        program_.add_shader(GL_VERTEX_SHADER, "shaders/donut.vert");
        program_.add_shader(GL_FRAGMENT_SHADER, "shaders/donut.frag");
        program_.link();
    }

    void update(float dt) override
    {
        cur_time_ += dt;
    }

    void render() override
    {
        const auto light_position = glm::vec3(-1, -1, 3);

        const float angle = cur_time_ * 2.f * M_PI / cycle_duration_;
        const float u_offset = cur_time_ / cycle_duration_ / 4;
        geometry_.update_verts(angle, u_offset);

#if 0
        const float angle = -cur_time_ * 2.f * M_PI / cycle_duration_ / CircleVerts;
        const auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));
#else
        const auto model = glm::mat4(1.0);
#endif

        glDisable(GL_CULL_FACE);

        // scene

        glViewport(0, 0, width_, height_);
        // glClearColor(0.75, 0.75, 0.75, 0);
        glClearColor(0, 0, 0, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        const auto projection =
                glm::perspective(glm::radians(45.0f), static_cast<float>(width_) / height_, 0.1f, 100.f);
        const auto view = glm::lookAt(glm::vec3(0, 0, 4), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        const auto mvp = projection * view * model;

        program_.bind();
        program_.set_uniform("mvp", mvp);
        program_.set_uniform("modelMatrix", model);
        program_.set_uniform("lightPosition", light_position);
        program_.set_uniform("color", glm::vec3(1, 0, 0));

        geometry_.render();
    }

    static constexpr auto ShadowWidth = 2048;
    static constexpr auto ShadowHeight = ShadowWidth;

    float cur_time_ = 0;
    gl::shader_program program_;
    DonutGeometry geometry_;
};

int main(int argc, char *argv[])
{
    Demo d(argc, argv);
    d.run();
}
