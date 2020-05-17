#include "panic.h"

#include "window.h"
#include "geometry.h"
#include "shader_program.h"
#include "util.h"
#include "tween.h"
#include "shadow_buffer.h"

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

constexpr const auto ExplodeDuration = 0.25f;
constexpr const auto ImplodeDuration = 0.125f;

#ifdef DUMP_FRAMES
constexpr const auto FramesPerSecond = 40;
#else
constexpr const auto FramesPerSecond = 60;
#endif

struct Polygon {
    glm::vec3 normal;
    glm::vec3 color;
    std::vector<glm::vec3> verts;
};

using Mesh = std::vector<Polygon>;

struct Plane {
    glm::vec3 point;
    glm::vec3 normal;
};

auto split(const std::vector<glm::vec3> &verts, const Plane &plane)
{
    std::vector<glm::vec3> front_verts, back_verts;

    for (int i = 0; i < verts.size(); ++i) {
        const auto is_behind = [&plane](const glm::vec3 &v) {
            return glm::dot(v - plane.point, plane.normal) < 0;
        };

        const auto &v0 = verts[i];
        const auto &v1 = verts[(i + 1) % verts.size()];

        const auto b0 = is_behind(v0);
        const auto b1 = is_behind(v1);

        if (b0)
            front_verts.push_back(v0);
        else
            back_verts.push_back(v0);

        if (b0 != b1) {
            const auto t = glm::dot(plane.point - v0, plane.normal) / glm::dot(v1 - v0, plane.normal);
            const auto m = v0 + t * (v1 - v0);
            front_verts.push_back(m);
            back_verts.push_back(m);
        }
    }

    return std::make_tuple(front_verts, back_verts);
}

auto split(const Mesh &mesh, const Plane &plane)
{
    Mesh front_mesh, back_mesh;

    for (const auto &poly : mesh) {
        const auto [front_verts, back_verts] = split(poly.verts, plane);
        if (!front_verts.empty()) {
            front_mesh.push_back({ poly.normal, poly.color, front_verts });
        }
        if (!back_verts.empty()) {
            back_mesh.push_back({ poly.normal, poly.color, back_verts });
        }
    }

    if (!front_mesh.empty() && !back_mesh.empty()) {
        glm::vec3 up = glm::vec3{0, 1, 0};
        const glm::vec3 right = glm::cross(plane.normal, up);
        up = glm::cross(right, plane.normal);

        constexpr const auto PlaneSize = 10.0f;
        std::vector<glm::vec3> plane_verts;
        plane_verts.push_back(plane.point + PlaneSize * (-up -right));
        plane_verts.push_back(plane.point + PlaneSize * (-up + right));
        plane_verts.push_back(plane.point + PlaneSize * (up + right));
        plane_verts.push_back(plane.point + PlaneSize * (up - right));

        for (const auto &poly : mesh) {
            const auto [front_verts, back_verts] = split(plane_verts, Plane{poly.verts[0], poly.normal});
            plane_verts = front_verts;
            assert(!plane_verts.empty());
        }

        const auto InnerColor = glm::vec3(1, 1, 0);
        front_mesh.push_back({ plane.normal, InnerColor, plane_verts });
        back_mesh.push_back({ -plane.normal, InnerColor, plane_verts });
    }

    return std::make_tuple(front_mesh, back_mesh);
}

using Vertex = std::tuple<glm::vec3, glm::vec3, glm::vec3>; // position / normal / color

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
        const auto color = glm::vec3(0.75);

        verts_.push_back({center - up - side, normal, color});
        verts_.push_back({center + up - side, normal, color});
        verts_.push_back({center + up + side, normal, color});

        verts_.push_back({center + up + side, normal, color});
        verts_.push_back({center - up + side, normal, color});
        verts_.push_back({center - up - side, normal, color});
    }

    std::vector<Vertex> verts_;
    gl::geometry geometry_;
};

class MeshGeometry
{
public:
    MeshGeometry(const Mesh &m)
    {
        initialize_geometry(m);
        geometry_.set_data(verts_);
    }

    void render() const
    {
        geometry_.bind();
        glDrawArrays(GL_TRIANGLES, 0, verts_.size());
    }

private:
    void initialize_geometry(const Mesh &mesh)
    {
        for (const auto &poly : mesh) {
            const auto &verts = poly.verts;
            for (int i = 1; i < verts.size() - 1; ++i) {
                const auto &v0 = verts[0];
                const auto &v1 = verts[i];
                const auto &v2 = verts[i + 1];
                verts_.push_back({ v0, poly.normal, poly.color });
                verts_.push_back({ v1, poly.normal, poly.color });
                verts_.push_back({ v2, poly.normal, poly.color });
            }
        }
    }

    std::vector<Vertex> verts_;
    gl::geometry geometry_;
};

static Mesh make_cube()
{
    const auto make_face = [](const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec3 &v3) {
        const auto u = v1 - v0;
        const auto v = v3 - v0;
        const auto n = glm::normalize(glm::cross(u, v));
        return Polygon{ n, glm::vec3(1.0), { v0, v1, v2, v3 } };
    };

    const glm::vec3 v0(-1, -1, -1);
    const glm::vec3 v1(-1, 1, -1);
    const glm::vec3 v2(1, 1, -1);
    const glm::vec3 v3(1, -1, -1);

    const glm::vec3 v4(-1, -1, 1);
    const glm::vec3 v5(-1, 1, 1);
    const glm::vec3 v6(1, 1, 1);
    const glm::vec3 v7(1, -1, 1);

    Mesh m;

    m.push_back(make_face(v0, v1, v2, v3));
    m.push_back(make_face(v1, v5, v6, v2));
    m.push_back(make_face(v0, v4, v5, v1));
    m.push_back(make_face(v7, v4, v0, v3));
    m.push_back(make_face(v7, v3, v2, v6));
    m.push_back(make_face(v4, v7, v6, v5));

    return m;
}

struct Node
{
    virtual ~Node() = default;
    virtual void render(const gl::shader_program &program, const glm::mat4 &m, float time) const = 0;
};

struct Split : Node
{
    void render(const gl::shader_program &program, const glm::mat4 &m, float time) const override
    {
        constexpr const auto MaxOffset = 0.5f;

        const auto offset = [this, time]() -> float {
            if (time < start_explode) {
                return 0;
            } else if (time < start_explode + ExplodeDuration) {
                float t = (time - start_explode) / ExplodeDuration;
                return in_quadratic(t) * MaxOffset;
            } else if (time < start_implode) {
                return MaxOffset;
            } else if (time < start_implode + ImplodeDuration) {
                float t = (time - start_implode) / ImplodeDuration;
                return out_quadratic(1 - t) * MaxOffset;
            } else {
                return 0;
            }
        }();

        front->render(program, m * glm::translate(glm::mat4(1), -offset * normal), time);
        back->render(program, m * glm::translate(glm::mat4(1), offset * normal), time);
    }

    glm::vec3 normal;
    float start_explode;
    float start_implode;
    std::unique_ptr<Node> front, back;
};

struct Leaf : Node
{
    void render(const gl::shader_program &program, const glm::mat4 &model, float) const override
    {
        program.set_uniform(program.uniform_location("modelMatrix"), model);
        mesh->render();
    }

    std::unique_ptr<MeshGeometry> mesh;
};

std::unique_ptr<Node> build_tree(const Mesh &mesh, int depth)
{
    constexpr const auto MaxDepth = 7;
    if (depth == MaxDepth) {
        auto leaf = new Leaf;
        leaf->mesh = std::make_unique<MeshGeometry>(mesh);
        return std::unique_ptr<Node>(leaf);
    }

    const auto rand_vector = [] {
        auto v = glm::vec3(static_cast<float>(rand()) / RAND_MAX, static_cast<float>(rand()) / RAND_MAX, static_cast<float>(rand()) / RAND_MAX);
        return 2.0f * v - glm::vec3(1.0f);
    };

    Plane plane;
    plane.point = rand_vector();
    plane.normal = glm::normalize(rand_vector());

    const auto [front_mesh, back_mesh] = split(mesh, plane);

    if (front_mesh.empty() || back_mesh.empty()) {
        auto leaf = new Leaf;
        leaf->mesh = std::make_unique<MeshGeometry>(mesh);
        return std::unique_ptr<Node>(leaf);
    }

    constexpr const auto StartExplode = 0.25;
    constexpr const auto StartImplode = CycleDuration - StartExplode - ImplodeDuration;

    auto split = new Split;
    split->normal = plane.normal;
    split->front = build_tree(front_mesh, depth + 1);
    split->back = build_tree(back_mesh, depth + 1);
    split->start_explode = StartExplode + 0.25 * depth;
    split->start_implode = StartImplode - 0.5 * 0.125 * depth;
    return std::unique_ptr<Node>(split);
}

class Demo
{
public:
    Demo(int window_width, int window_height)
        : window_width_(window_width)
        , window_height_(window_height)
        , plane_(glm::vec3(0, 0, -2.5), glm::vec3(10, 0, 0), glm::vec3(0, 10, 0))
        , shadow_buffer_(ShadowWidth, ShadowHeight)
    {
        initialize_shader();
        split_tree_ = build_tree(make_cube(), 0);
    }

    void render_and_step(float dt)
    {
        render();
        cur_time_ += dt;
        if (cur_time_ >= CycleDuration) {
            cur_time_ -= CycleDuration;
            split_tree_ = build_tree(make_cube(), 0);
        }
    }

private:
    void initialize_shader()
    {
        shadow_program_.add_shader(GL_VERTEX_SHADER, "shaders/shadow.vert");
        shadow_program_.add_shader(GL_FRAGMENT_SHADER, "shaders/shadow.frag");
        shadow_program_.link();

        program_.add_shader(GL_VERTEX_SHADER, "shaders/phong.vert");
        program_.add_shader(GL_FRAGMENT_SHADER, "shaders/phong.frag");
        program_.link();
    }

    void render() const
    {
        const auto light_position = glm::vec3(3, -3, 5);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glDisable(GL_CULL_FACE);

        // render shadow

        glViewport(0, 0, ShadowWidth, ShadowHeight);
        shadow_buffer_.bind();

        glClear(GL_DEPTH_BUFFER_BIT);

        const auto light_projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 1.0f, 12.5f);
        const auto light_view = glm::lookAt(light_position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

        const float angle = 0.3f * cosf(cur_time_ * 2.f * M_PI / CycleDuration);
        const auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(-1, 1, 1)) *
            glm::rotate(glm::mat4(1.0f), static_cast<float>(0.25f * M_PI), glm::vec3(1, 0, 0)) *
            glm::rotate(glm::mat4(1.0f), static_cast<float>(0.25f * M_PI), glm::vec3(0, 1, 0));

        shadow_program_.bind();
        shadow_program_.set_uniform(program_.uniform_location("viewMatrix"), light_view);
        shadow_program_.set_uniform(program_.uniform_location("projectionMatrix"), light_projection);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(4, 4);

        shadow_program_.set_uniform(program_.uniform_location("modelMatrix"), glm::mat4(1.0));
        plane_.render();
        split_tree_->render(shadow_program_, model, fmod(cur_time_, CycleDuration));

        glDisable(GL_POLYGON_OFFSET_FILL);

        shadow_buffer_.unbind();

        // render scene

        glViewport(0, 0, window_width_, window_height_);

        glClearColor(0.75, 0.75, 0.75, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const auto projection =
                glm::perspective(glm::radians(45.0f), static_cast<float>(window_width_) / window_height_, 0.1f, 100.f);
        const auto view_pos = glm::vec3(0, 0, 7);
        const auto view_up = glm::vec3(0, 1, 0);
        const auto view = glm::lookAt(view_pos, glm::vec3(0, 0, 0), view_up);

        shadow_buffer_.bind_texture();

        program_.bind();
        program_.set_uniform(program_.uniform_location("lightPosition"), light_position);
        program_.set_uniform(program_.uniform_location("eyePosition"), view_pos);
        program_.set_uniform(program_.uniform_location("projectionMatrix"), projection);
        program_.set_uniform(program_.uniform_location("viewMatrix"), view);
        program_.set_uniform(program_.uniform_location("lightViewProjection"), light_projection * light_view);
        program_.set_uniform(program_.uniform_location("shadowMapTexture"), 0);

        program_.set_uniform(program_.uniform_location("modelMatrix"), glm::mat4(1.0));
        plane_.render();
        split_tree_->render(program_, model, fmod(cur_time_, CycleDuration));
    }

    static constexpr auto ShadowWidth = 2048;
    static constexpr auto ShadowHeight = ShadowWidth;

    int window_width_;
    int window_height_;
    float cur_time_ = 0;
    std::unique_ptr<Node> split_tree_;
    PlaneGeometry plane_;
    gl::shadow_buffer shadow_buffer_;
    gl::shader_program program_;
    gl::shader_program shadow_program_;
};

int main()
{
    constexpr auto window_width = 800;
    constexpr auto window_height = 800;

    srand(time(nullptr));

    gl::window w(window_width, window_height, "Demo");

    glewInit();

    glfwSetKeyCallback(w, [](GLFWwindow *window, int key, int scancode, int action, int mode) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
    });

#ifdef DUMP_FRAMES
    constexpr auto total_frames = 5 * CycleDuration * FramesPerSecond;
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
