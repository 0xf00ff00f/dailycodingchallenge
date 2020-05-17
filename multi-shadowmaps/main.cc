#include "panic.h"

#include "window.h"
#include "geometry.h"
#include "shader_program.h"
#include "util.h"
#include "buffer.h"
#include "multi_shadow_buffer.h"
#include "tween.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <random>
#include <fstream>

// #define DUMP_FRAMES

constexpr const auto CycleDuration = 3.f;
#ifdef DUMP_FRAMES
constexpr const auto FramesPerSecond = 40;
#else
constexpr const auto FramesPerSecond = 60;
#endif

class Plane
{
public:
    Plane(const glm::vec3 &center, const glm::vec3 &up, const glm::vec3 &side)
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

        verts_.push_back({center - up - side, normal});
        verts_.push_back({center + up - side, normal});
        verts_.push_back({center + up + side, normal});

        verts_.push_back({center + up + side, normal});
        verts_.push_back({center - up + side, normal});
        verts_.push_back({center - up - side, normal});
    }

    using vertex = std::tuple<glm::vec3, glm::vec3>; // position, normal, texuv
    std::vector<vertex> verts_;
    gl::geometry geometry_;
};

class Mesh
{
public:
    Mesh(const char *file)
    {
        initialize_geometry(file);
        geometry_.set_data(verts_);
    }

    void render() const
    {
        geometry_.bind();
        glDrawArrays(GL_TRIANGLES, 0, verts_.size());
    }

private:
    void initialize_geometry(const char *file)
    {
        std::ifstream ifs(file);
        if (ifs.fail())
            panic("failed to open %s\n", file);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        struct vertex {
            int position_index;
            int normal_index;
        };
        using face = std::vector<vertex>;
        std::vector<face> faces;

        std::string line;
        while (std::getline(ifs, line)) {
            std::vector<std::string> tokens;
            boost::split(tokens, line, boost::is_any_of(" \t"), boost::token_compress_on);
            if (tokens.empty())
                continue;
            if (tokens.front() == "v") {
                assert(tokens.size() == 4);
                positions.emplace_back(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            } else if (tokens.front() == "vn") {
                assert(tokens.size() == 4);
                normals.emplace_back(std::stof(tokens[1]), std::stof(tokens[2]), std::stof(tokens[3]));
            } else if (tokens.front() == "f") {
                face f;
                for (auto it = std::next(tokens.begin()); it != tokens.end(); ++it) {
                    std::vector<std::string> components;
                    boost::split(components, *it, boost::is_any_of("/"), boost::token_compress_off);
                    assert(components.size() == 3);
                    f.push_back({ std::stoi(components[0]) - 1, std::stoi(components[2]) - 1 });
                }
                faces.push_back(f);
            }
        }

        for (const auto &face : faces) {
            for (size_t i = 1; i < face.size() - 1; ++i) {
                const auto to_vertex = [&positions, &normals](const auto &vertex) {
                    return std::make_tuple(positions[vertex.position_index], normals[vertex.normal_index]);
                };
                const auto v0 = to_vertex(face[0]);
                const auto v1 = to_vertex(face[i]);
                const auto v2 = to_vertex(face[i + 1]);
                verts_.push_back(v0);
                verts_.push_back(v1);
                verts_.push_back(v2);
            }
        }
    }

    using vertex = std::tuple<glm::vec3, glm::vec3>; // position, normal, texuv
    std::vector<vertex> verts_;
    gl::geometry geometry_;
};

class Demo
{
public:
    Demo(int window_width, int window_height)
        : window_width_(window_width)
        , window_height_(window_height)
        , mesh_(new Mesh("assets/meshes/monkey.obj"))
        , plane_(new Plane(glm::vec3(0, 0, -2), glm::vec3(3, 0, 0), glm::vec3(0, 4, 0)))
    {
        initialize_lights();
        initialize_shader();
    }

    void render_and_step(float dt)
    {
        render();
        cur_time_ += dt;
    }

private:
    void initialize_lights()
    {
        lights_.emplace_back(glm::vec3(-4, 4, 7));
        lights_.emplace_back(glm::vec3(5, -5, 9));
        lights_.emplace_back(glm::vec3(3, 3, 6));
        lights_.emplace_back(glm::vec3(-3, -2, 8));

        shadow_buffer_.reset(new gl::multi_shadow_buffer(ShadowWidth, ShadowHeight, lights_.size()));
        std::vector<BufferLight> buffer;
        buffer.reserve(lights_.size());
        std::transform(lights_.begin(), lights_.end(), std::back_inserter(buffer), [](const Light &light) {
            return BufferLight{ glm::vec4(light.position, 1.0), light.projection * light.view };
        });
        light_buffer_.reset(new gl::buffer<BufferLight>(GL_SHADER_STORAGE_BUFFER, buffer.data(), buffer.size()));
    }

    void initialize_shader()
    {
        shadow_program_.add_shader(GL_VERTEX_SHADER, "assets/shaders/shadow.vert");
        shadow_program_.add_shader(GL_FRAGMENT_SHADER, "assets/shaders/shadow.frag");
        shadow_program_.link();

        program_.add_shader(GL_VERTEX_SHADER, "assets/shaders/simple.vert");
        program_.add_shader(GL_FRAGMENT_SHADER, "assets/shaders/simple.frag");
        program_.link();
    }

    void render() const
    {
        const auto light_position = glm::vec3(-4, 4, 5); // glm::vec3(-2 * cosf(cur_time_), -2 * sinf(cur_time_), 5);

        const auto model = glm::mat4(1.0);
        const auto monkey_model = glm::rotate(glm::mat4(1.0), cur_time_, glm::vec3(0, 1, 0));

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glDisable(GL_CULL_FACE);

        // render shadow maps

        glViewport(0, 0, ShadowWidth, ShadowHeight);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(4, 4);

        for (int i = 0; i < lights_.size(); ++i)
        {
            const auto &light = lights_[i];

            shadow_buffer_->bind(i);

            glClear(GL_DEPTH_BUFFER_BIT);

            shadow_program_.bind();
            shadow_program_.set_uniform("viewMatrix", light.view);
            shadow_program_.set_uniform("projectionMatrix", light.projection);

            shadow_program_.set_uniform("modelMatrix", model);
            plane_->render();

            shadow_program_.set_uniform("modelMatrix", model * monkey_model);
            mesh_->render();
        }

        glDisable(GL_POLYGON_OFFSET_FILL);

        shadow_buffer_->unbind();

        // render cube

        glViewport(0, 0, window_width_, window_height_);
        glClearColor(0.75, 0.75, 0.75, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const auto projection =
                glm::perspective(glm::radians(45.0f), static_cast<float>(window_width_) / window_height_, 0.1f, 100.f);
        // const auto view_pos = glm::vec3(1.5, -1.5, 1.5);
        const auto view_pos = glm::vec3(2, 2, 7);
        const auto view = glm::lookAt(view_pos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

        shadow_buffer_->bind_texture();

        program_.bind();
        program_.set_uniform("modelMatrix", model);
        program_.set_uniform("viewMatrix", view);
        program_.set_uniform("projectionMatrix", projection);
        program_.set_uniform("eyePosition", view_pos);
        program_.set_uniform("lightPosition", light_position);
        program_.set_uniform("lightCount", static_cast<int>(lights_.size()));
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, light_buffer_->handle());

        program_.set_uniform("modelMatrix", model);
        plane_->render();

        program_.set_uniform("modelMatrix", model * monkey_model);
        mesh_->render();
    }

    static constexpr auto ShadowWidth = 2048;
    static constexpr auto ShadowHeight = ShadowWidth;

    int window_width_;
    int window_height_;
    float cur_time_ = 0;
    gl::shader_program program_;
    gl::shader_program shadow_program_;
    std::unique_ptr<Mesh> mesh_;
    std::unique_ptr<Plane> plane_;
    std::unique_ptr<gl::multi_shadow_buffer> shadow_buffer_;
    struct Light
    {
        glm::vec3 position;
        glm::mat4 projection;
        glm::mat4 view;
        Light(const glm::vec3 &position)
            : position(position)
        {
            projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 1.0f, 12.5f);
            view = glm::lookAt(position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        }
    };
    std::vector<Light> lights_;
    struct BufferLight
    {
        glm::vec4 position;
        glm::mat4 viewProjection;
    };
    std::unique_ptr<gl::buffer<BufferLight>> light_buffer_;
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
