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

#include <algorithm>
#include <iostream>
#include <memory>
#include <random>

// #define DUMP_FRAMES

constexpr const auto CycleDuration = 3.f;
#ifdef DUMP_FRAMES
constexpr const auto FramesPerSecond = 40;
#else
constexpr const auto FramesPerSecond = 60;
#endif

class cube_geometry
{
public:
    cube_geometry()
    {
        initialize_geometry();
        geometry_.set_data(verts_);
    }

    void render(int instance_count) const
    {
        geometry_.bind();
        glDrawArraysInstanced(GL_TRIANGLES, 0, verts_.size(), instance_count);
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
        const glm::vec3 v1(-1, 1, -1);
        const glm::vec3 v2(1, 1, -1);
        const glm::vec3 v3(1, -1, -1);

        const glm::vec3 v4(-1, -1, 1);
        const glm::vec3 v5(-1, 1, 1);
        const glm::vec3 v6(1, 1, 1);
        const glm::vec3 v7(1, -1, 1);

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
        , states_(GL_SHADER_STORAGE_BUFFER, GridSize * GridSize * GridSize)
        , cube_(new cube_geometry)
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

        const float angle = 0.3f * cosf(cur_time_ * 2.f * M_PI / CycleDuration);
        const auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(-1, 1, 1));

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
        constexpr const auto CenterEntity = (GridSize / 2) * GridSize * GridSize + (GridSize / 2) * GridSize + (GridSize / 2);

        auto *state = states_.map();

        for (int i = 0; i < GridSize; ++i) {
            for (int j = 0; j < GridSize; ++j) {
                for (int k = 0; k < GridSize; ++k) {
                    const auto index = i * GridSize * GridSize + j * GridSize + k;
                    float scale, alpha;
                    if (index != CenterEntity) {
                        const auto start = collapse_start_[index];
                        if (cur_time_ < start) {
                            scale = 1;
                            alpha = 1;
                        } else if (cur_time_ > start + CollapseDuration) {
                            scale = 0;
                            alpha = 0;
                        } else {
                            float t = (cur_time_ - start) / CollapseDuration;
                            scale = in_quadratic(1 - t);
                            alpha = 1 - t;
                        }
                    } else {
                        constexpr const auto ExpansionStart = 1.2f;
                        constexpr const auto ExpansionDuration = 1.5f;
                        if (cur_time_ < ExpansionStart) {
                            scale = 1;
                        } else if (cur_time_ > ExpansionStart + ExpansionDuration) {
                            scale = static_cast<float>(GridSize);
                        } else {
                            const auto t = (cur_time_ - ExpansionStart) / ExpansionDuration;
                            scale = 1 + out_bounce(t) * (static_cast<float>(GridSize) - 1);
                        }
                        alpha = 1;
                    }

                    const float x = (static_cast<float>(i) - 0.5f * static_cast<float>(GridSize - 1)) * CellSize;
                    const float y = (static_cast<float>(j) - 0.5f * static_cast<float>(GridSize - 1)) * CellSize;
                    const float z = (static_cast<float>(k) - 0.5f * static_cast<float>(GridSize - 1)) * CellSize;
                    const auto transform_matrix = glm::translate(glm::mat4(1.0), glm::vec3(x, y, z));
                    const auto scale_matrix = glm::scale(glm::mat4(1.0), glm::vec3(scale * 0.5 * CellSize));

                    const auto diffuse_color = glm::vec3(1.0, 0.0, 0.0);

                    state->transform = transform_matrix * scale_matrix;
                    state->color = glm::vec4(diffuse_color, alpha);
                    ++state;
                }
            }
        }

        states_.unmap();
    }

    static constexpr auto GridSize = 5;
    static constexpr auto CellSize = 1.f / GridSize;
    static constexpr auto CollapseDuration = 0.5f;

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
    std::unique_ptr<cube_geometry> cube_;
    std::vector<float> collapse_start_;
};

int main()
{
    constexpr auto window_width = 800;
    constexpr auto window_height = 800;

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
