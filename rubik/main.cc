#include "panic.h"

#include "window.h"
#include "geometry.h"
#include "shader_program.h"
#include "util.h"
#include "buffer.h"

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

class mesh
{
public:
    mesh(const char *file)
    {
        initialize_geometry(file);
        geometry_.set_data(verts_);
    }

    void render(int instance_count) const
    {
        geometry_.bind();
        glDrawArraysInstanced(GL_TRIANGLES, 0, verts_.size(), instance_count);
    }

private:
    void initialize_geometry(const char *file)
    {
        std::ifstream ifs(file);
        if (ifs.fail())
            panic("failed to open %s", file);

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

class demo
{
public:
    demo(int window_width, int window_height)
        : window_width_(window_width)
        , window_height_(window_height)
        , states_(GL_SHADER_STORAGE_BUFFER, GridSize * GridSize * GridSize)
        , cube_(new mesh("assets/meshes/beveled-cube.obj"))
    {
        initialize_shader();

        std::random_device device;
        std::mt19937 generator(device());
        std::uniform_real_distribution<float> distribution(0.5, 1.5);

        collapse_start_.resize(GridSize * GridSize * GridSize);
        std::generate(collapse_start_.begin(), collapse_start_.end(), [&distribution, &generator] {
            return distribution(generator);
        });
    }

    void render_and_step(float dt)
    {
        render(program_);
        cur_time_ += dt;
        if (cur_time_ >= MotionDuration) {
            flip_ = !flip_;
            moving_ = rand() % ((1 << GridSize) - 1);
            moving_direction_ = rand() % ((1 << GridSize) - 1);
            cur_time_ -= MotionDuration;
        }
    }

private:
    void initialize_shader()
    {
        program_.add_shader(GL_VERTEX_SHADER, "assets/shaders/sphere.vert");
        program_.add_shader(GL_FRAGMENT_SHADER, "assets/shaders/sphere.frag");
        program_.link();
    }

    void render(const gl::shader_program &program) const
    {
        update_grid_state();

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
        const auto view_pos = glm::vec3(1.5, -1.5, 1.5);
        const auto view_up = glm::vec3(0, 1, 0);
        const auto view = glm::lookAt(view_pos, glm::vec3(0, 0, 0), view_up);

        const auto model = flip_ ? glm::rotate(glm::mat4(1.0f), static_cast<float>(0.5 * M_PI), glm::vec3(0, 1, 0)) : glm::mat4(1.0);

        program.bind();
        program.set_uniform(program.uniform_location("modelMatrix"), model);
        program.set_uniform(program.uniform_location("viewMatrix"), view);
        program.set_uniform(program.uniform_location("projectionMatrix"), projection);
        program.set_uniform(program.uniform_location("eyePosition"), view_pos);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, states_.handle());

        glCullFace(GL_BACK);
        cube_->render(GridSize * GridSize * GridSize);
    }

    void update_grid_state() const
    {
        const auto motion_time = fmod(cur_time_, MotionDuration) / MotionDuration;

        constexpr const auto CenterEntity = (GridSize / 2) * GridSize * GridSize + (GridSize / 2) * GridSize + (GridSize / 2);

        auto *state = states_.map();

        for (int i = 0; i < GridSize; ++i) {
            float slice_rotation;
            if ((moving_ & (1 << i)) == 0) {
                slice_rotation = 0;
            } else {
                slice_rotation = in_quadratic(motion_time) * 0.5f * M_PI;
                if (moving_direction_ & (1 << i))
                    slice_rotation = -slice_rotation;
            }
            const auto v = (glm::vec3(i, 0, 0) - 0.5f * glm::vec3(GridSize - 1, 0, 0)) * CellSize;
            const auto slice_translate_matrix = glm::translate(glm::mat4(1.0), v);
            const auto slice_rotation_matrix = glm::rotate(glm::mat4(1.0), slice_rotation, glm::vec3(1, 0, 0));
            const auto slice_transform = slice_translate_matrix * slice_rotation_matrix;

            for (int j = 0; j < GridSize; ++j) {
                for (int k = 0; k < GridSize; ++k) {
                    const auto index = i * GridSize * GridSize + j * GridSize + k;

                    const auto v = (glm::vec3(0, j, k) - 0.5f * glm::vec3(0, GridSize - 1, GridSize - 1)) * CellSize;
                    const auto translate_matrix = glm::translate(glm::mat4(1.0), v);
                    const auto scale_matrix = glm::scale(glm::mat4(1.0), glm::vec3(0.95 * 0.5 * CellSize));

                    const auto diffuse_color = glm::vec3(1.0, 1.0, 1.0);

                    state->transform = slice_transform * translate_matrix * scale_matrix;
                    state->color = glm::vec4(diffuse_color, 1.0);
                    ++state;
                }
            }
        }

        states_.unmap();
    }

    static constexpr auto GridSize = 3;
    static constexpr auto CellSize = 1.f / GridSize;
    static constexpr auto MotionDuration = 0.25f;

    struct entity_state {
        glm::mat4 transform;
        glm::vec4 color;
    };
    int window_width_;
    int window_height_;
    float cur_time_ = 0;
    gl::shader_program program_;
    static_assert(sizeof(glm::mat4) == 16 * sizeof(float));
    gl::buffer<entity_state> states_;
    std::unique_ptr<mesh> cube_;
    std::vector<float> collapse_start_;
    unsigned moving_ = 1;
    unsigned moving_direction_ = 1;
    bool flip_ = false;
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
