#include "panic.h"

#include <window.h>
#include <demo.h>
#include <geometry.h>
#include <shader_program.h>
#include <shadow_buffer.h>
#include <util.h>

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

class PlaneGeometry
{
public:
    PlaneGeometry(const glm::vec3 &center, const glm::vec3 &up, const glm::vec3 &side)
    {
        initialize_geometry(center, up, side);
        geometry_.set_data(verts_);
    }

    void render() const
    {
        geometry_.bind();
        glDrawArrays(GL_TRIANGLES, 0, verts_.size());
    }

private:
    void initialize_geometry(const glm::vec3 &center, const glm::vec3 &up, const glm::vec3 &side)
    {
        const auto normal = glm::normalize(glm::cross(up, side));

        verts_.push_back({center - up - side, normal, glm::vec2(0, 0)});
        verts_.push_back({center + up - side, normal, glm::vec2(1, 0)});
        verts_.push_back({center + up + side, normal, glm::vec2(1, 1)});

        verts_.push_back({center + up + side, normal, glm::vec2(1, 1)});
        verts_.push_back({center - up + side, normal, glm::vec2(0, 1)});
        verts_.push_back({center - up - side, normal, glm::vec2(0, 0)});
    }

    using Vertex = std::tuple<glm::vec3, glm::vec3, glm::vec2>; // position, normal, texuv
    std::vector<Vertex> verts_;
    gl::geometry geometry_;
};

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

    void update_verts(float small_radius, float big_radius, float angle_offset, float u_offset) const
    {
        glBindBuffer(GL_ARRAY_BUFFER, geometry_.array_buffer_handle());
        auto *verts = static_cast<Vertex *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

        for (int i = 0; i < NumSegmentsOuter; ++i)
        {
            for (int j = 0; j < NumSegmentsInner; ++j)
            {
                const auto vert_at = [angle_offset, small_radius, big_radius](int i, int j) -> std::tuple<glm::vec3, glm::vec3> {
                    const float a = static_cast<float>(i) * 2.0 * M_PI / NumSegmentsOuter;

                    const float b = static_cast<float>(j) * 2.0 * M_PI / NumSegmentsInner + a + angle_offset;

                    const auto p = glm::vec3(cosf(a) * big_radius, sinf(a) * big_radius, 0.0);

                    const auto u = glm::vec3(cosf(a), sinf(a), 0.0);
                    const auto v = glm::vec3(0.0, 0.0, 1.0);

                    const float s = 0.5 * (1.0 + cos(a - M_PI));
                    const float r = small_radius * (0.25 + 1.5 * s);

                    const auto pos = p + (sinf(b) * u + cosf(b) * v) * r;
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
        , plane_(glm::vec3(0, 0, 0), glm::vec3(50, 0, 0), glm::vec3(0, 0, 50))
        , shadow_buffer_(ShadowWidth, ShadowHeight)
    {
        blur_.reset(new blur_effect(width_/4, height_/4));
        initialize_shader();
    }

private:
    void initialize_shader()
    {
        donut_program_.add_shader(GL_VERTEX_SHADER, "shaders/donut.vert");
        donut_program_.add_shader(GL_FRAGMENT_SHADER, "shaders/donut.frag");
        donut_program_.link();

        plane_program_.add_shader(GL_VERTEX_SHADER, "shaders/plane.vert");
        plane_program_.add_shader(GL_FRAGMENT_SHADER, "shaders/plane.frag");
        plane_program_.link();

        shadow_program_.add_shader(GL_VERTEX_SHADER, "shaders/shadow.vert");
        shadow_program_.add_shader(GL_FRAGMENT_SHADER, "shaders/shadow.frag");
        shadow_program_.link();
    }

    void update(float dt) override
    {
        cur_time_ += dt;
    }

    void render() override
    {
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_MULTISAMPLE);

        {
            const float angle = cur_time_ * 2.f * M_PI / (cycle_duration_ / 2.0);
            const float u_offset = cur_time_ / (cycle_duration_ / 2.0) / 4;
            geometry_.update_verts(DonutSmallRadius, DonutRadius, angle, u_offset);
        }

#if 1
        // const float angle = 0.5f * cosf(cur_time_ * 2.f * M_PI / cycle_duration_);
        const float angle = cur_time_ * 2.f * M_PI / cycle_duration_;
        const auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));
#else
        auto model = glm::mat4(1.0f); // glm::translate(glm::mat4(1.0f), glm::vec3(0, 1.5, 0));
#endif

        const auto light_position = glm::vec3(3, 4, 3);

        const auto projection =
                glm::perspective(glm::radians(45.0f), static_cast<float>(width_) / height_, 0.1f, 100.f);
        const auto eye = glm::vec3(0, 4, 5);
        const auto center = glm::vec3(0, 1, 0);
        const auto up = glm::vec3(0, 1, 0);
        const auto view = glm::lookAt(eye, center, up);
        const auto viewProjection = projection * view;

#if 0
        blur_->bind();

        glViewport(0, 0, blur_->width(), blur_->height());
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_scene(program_, glm::vec3(0), model);
        gl::framebuffer::unbind();
#endif

#if 0
        blur_->render(width_, height_, 4);
#endif

        // shadow

        const auto light_projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 1.0f, 50.0f);
        const auto light_view = glm::lookAt(light_position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

        glViewport(0, 0, ShadowWidth, ShadowHeight);
        shadow_buffer_.bind();

        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(4, 4);

        glDisable(GL_CULL_FACE);
        draw_scene(shadow_program_, glm::vec3(1), light_projection * light_view, model, light_position);

        glDisable(GL_POLYGON_OFFSET_FILL);
        shadow_buffer_.unbind();

        donut_program_.bind();
        donut_program_.set_uniform("lightViewProjection", light_projection * light_view);
        donut_program_.set_uniform("shadowMapTexture", 0);

        plane_program_.bind();
        plane_program_.set_uniform("lightViewProjection", light_projection * light_view);
        plane_program_.set_uniform("shadowMapTexture", 0);

        // reflection

        blur_->bind();
        glViewport(0, 0, blur_->width(), blur_->height());
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw_scene(donut_program_, glm::vec3(1), viewProjection, glm::scale(model, glm::vec3(1, -1, 1)), light_position);
        gl::framebuffer::unbind();

        // scene

        glViewport(0, 0, width_, height_);
        glClearColor(0.25, 0.25, 0.25, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        blur_->render(width_, height_, 8);

        glEnable(GL_DEPTH_TEST);
        draw_plane(plane_program_, viewProjection, model, light_position);

#if 1
        // bloom

        blur_->bind();
        glViewport(0, 0, blur_->width(), blur_->height());
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw_scene(donut_program_, glm::vec3(0), viewProjection, model, light_position);
        gl::framebuffer::unbind();

        glViewport(0, 0, width_, height_);
        draw_scene(donut_program_, glm::vec3(1), viewProjection, model, light_position);

        blur_->render(width_, height_, 8);
#else
        draw_scene(donut_program_, glm::vec3(1), viewProjection, model);
#endif
    }

    void draw_plane(gl::shader_program &program, const glm::mat4 &viewProjection, const glm::mat4 &model, const glm::vec3 &light_position)
    {
        shadow_buffer_.bind_texture();

        program.bind();
        program.set_uniform("mvp", viewProjection * model);
        program.set_uniform("modelMatrix", model);
        program.set_uniform("lightPosition", light_position);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        plane_.render();
        glDisable(GL_BLEND);
    }

    void draw_scene(gl::shader_program &program, const glm::vec3 &base_color, const glm::mat4 &viewProjection, const glm::mat4 &model, const glm::vec3 &light_position)
    {
        shadow_buffer_.bind_texture();

        program.bind();
        program.set_uniform("lightPosition", light_position);
        program.set_uniform("color", glm::vec3(1, 0, 0));
        program.set_uniform("baseColor", base_color);

        const auto translation = glm::translate(glm::mat4(1.0), glm::vec3(0, 1.5, 0));
        {
            const auto r = glm::rotate(glm::mat4(1.0), static_cast<float>(0.5f * M_PI), glm::vec3(1, 0, 0));
            const auto t = glm::translate(glm::mat4(1.0), glm::vec3(-0.75, 0, 0));
            const auto m = model * translation * r * t;
            program.set_uniform("mvp", viewProjection * m);
            program.set_uniform("modelMatrix", m);
            geometry_.render();
        }

        {
            const auto r = glm::rotate(glm::mat4(1.0), static_cast<float>(M_PI), glm::vec3(0, 1, 0));
            const auto t = glm::translate(glm::mat4(1.0), glm::vec3(0.75, 0, 0));
            const auto m = model * translation * t * r;
            program.set_uniform("mvp", viewProjection * m);
            program.set_uniform("modelMatrix", m);
            geometry_.render();
        }
    }

    static constexpr float DonutRadius = 1.0f;
    static constexpr float DonutSmallRadius = 0.25f;

    static constexpr auto ShadowWidth = 2048;
    static constexpr auto ShadowHeight = ShadowWidth;

    float cur_time_ = 0;
    gl::shader_program donut_program_, plane_program_, shadow_program_;
    DonutGeometry geometry_;
    PlaneGeometry plane_;
    std::unique_ptr<blur_effect> blur_;
    gl::shadow_buffer shadow_buffer_;
};

int main(int argc, char *argv[])
{
    Demo d(argc, argv);
    d.run();
}
