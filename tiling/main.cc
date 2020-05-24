#include "panic.h"

#include "window.h"
#include "geometry.h"
#include "shader_program.h"
#include "shadow_buffer.h"
#include "util.h"
#include "tween.h"
#include "buffer.h"

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

constexpr const auto CycleDuration = 3.f;
#ifdef DUMP_FRAMES
constexpr const auto FramesPerSecond = 40;
#else
constexpr const auto FramesPerSecond = 60;
#endif

class Demo
{
public:
    Demo(int window_width, int window_height)
        : window_width_(window_width)
        , window_height_(window_height)
        , shadow_buffer_(ShadowWidth, ShadowHeight)
        , hexagon_states_(GL_SHADER_STORAGE_BUFFER, GridRows * GridColumns)
        , diamond_states_(GL_SHADER_STORAGE_BUFFER, (GridRows - 1) * (GridColumns - 1))
    {
        initialize_shader();
        initialize_geometry();
        initialize_heights();
    }

    void render_and_step(float dt)
    {
        render();
        cur_time_ += dt;
    }

private:
    void initialize_shader()
    {
        shadow_program_.add_shader(GL_VERTEX_SHADER, "shaders/shadow.vert");
        shadow_program_.add_shader(GL_GEOMETRY_SHADER, "shaders/shadow.geom");
        shadow_program_.add_shader(GL_FRAGMENT_SHADER, "shaders/shadow.frag");
        shadow_program_.link();

        program_.add_shader(GL_VERTEX_SHADER, "shaders/tile.vert");
        program_.add_shader(GL_GEOMETRY_SHADER, "shaders/tile.geom");
        program_.add_shader(GL_FRAGMENT_SHADER, "shaders/tile.frag");
        program_.link();
    }

    void initialize_geometry()
    {
        const auto cos_30 = std::cos(M_PI / 6.0);

        static const std::vector<Vertex> hexagon_verts = {
            { glm::vec2(cos_30, 0.5) },
            { glm::vec2(0, 1) },
            { glm::vec2(-cos_30, 0.5) },
            { glm::vec2(-cos_30, -0.5) },
            { glm::vec2(0, -1) },
            { glm::vec2(cos_30, -0.5) },
        };
        hexagon_.set_data(hexagon_verts);

        static const std::vector<Vertex> diamond_verts = {
            { glm::vec2(cos_30, 0.0) },
            { glm::vec2(0.0, 0.5) },
            { glm::vec2(-cos_30, 0.0) },
            { glm::vec2(0.0, -0.5) }
        };
        diamond_.set_data(diamond_verts);
    }

    void initialize_heights()
    {
        std::generate(hexagon_heights_.begin(), hexagon_heights_.end(), [] {
            return static_cast<float>(std::rand()) / RAND_MAX;
        });
        std::generate(diamond_heights_.begin(), diamond_heights_.end(), [] {
            return static_cast<float>(std::rand()) / RAND_MAX;
        });
    }

    void render() const
    {
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        const auto light_position = glm::vec3(-6, 4, 6);

        const auto cos_30 = std::cos(M_PI / 6.0);
        const auto x_offset = std::fmod(static_cast<float>(cur_time_) / CycleDuration, 1.0) * 4.0 * cos_30;
        auto model =
            glm::rotate(glm::mat4(1.0f), static_cast<float>(0.25 * M_PI), glm::vec3(0, 0, 1)) *
            glm::translate(glm::mat4(1.0f), glm::vec3(-x_offset, 0, 0));

        update_buffers(model, x_offset);

        // shadow

        const auto light_projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 50.0f);
        const auto light_view = glm::lookAt(light_position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

        glViewport(0, 0, ShadowWidth, ShadowHeight);
        shadow_buffer_.bind();

        glClear(GL_DEPTH_BUFFER_BIT);

        shadow_program_.bind();
        shadow_program_.set_uniform("viewProjectionMatrix", light_projection * light_view);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(4, 4);

        glDisable(GL_CULL_FACE);
        draw_grid(shadow_program_, model, x_offset);

        glDisable(GL_POLYGON_OFFSET_FILL);
        shadow_buffer_.unbind();

        // render

        glViewport(0, 0, window_width_, window_height_);
        glClearColor(0, 0, 0, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const auto projection =
                glm::perspective(glm::radians(45.0f), static_cast<float>(window_width_) / window_height_, 0.1f, 100.f);
        const auto camera_position = /* glm::vec3(0, 0, 7); */ glm::vec3(0, -6, 15);
        const auto look_at = glm::vec3(0, 0, 0);
        const auto view = glm::lookAt(camera_position, look_at, glm::vec3(0, 1, 0));

        shadow_buffer_.bind_texture();

        program_.bind();
        program_.set_uniform("viewProjectionMatrix", projection * view);
        program_.set_uniform("lightPosition", light_position);
        program_.set_uniform("lightViewProjection", light_projection * light_view);
        program_.set_uniform("shadowMapTexture", 0);

        glEnable(GL_CULL_FACE);
        draw_grid(program_, model, x_offset);
    }

    void update_buffers(const glm::mat4 &model, float x_offset) const
    {
        const auto cos_30 = std::cos(M_PI / 6.0);
        constexpr const auto StepHeight = 3.0;

        const auto tile_height = [cos_30, &model, x_offset](float x, float drop_start) -> float {
            constexpr const auto DropDuration = 0.3;
            const auto max_offset = 2 * cos_30;

            x -= x_offset;
            drop_start *= (1.0 - DropDuration);

            if (x < -max_offset)
            {
                return 0;
            }
            else if (x > max_offset)
            {
                return StepHeight;
            }
            else
            {
                float t = (x + max_offset) / (2 * max_offset);
                if (t < drop_start)
                {
                    return 0;
                }
                else if (t > drop_start + DropDuration)
                {
                    return StepHeight;
                }
                else
                {
                    float s = ((t - drop_start) / DropDuration);
                    return out_quadratic(s) * StepHeight;
                }
            }
        };

        // hexagons

        {
            auto *state = hexagon_states_.map();
            for (int i = 0; i < GridRows; ++i) {
                for (int j = 0; j < GridColumns; ++j) {
                    const auto x = 2.0 * cos_30 * (j - (0.5 * (GridColumns - 1)));
                    const auto y = 2.0 * (i - 0.5 * (GridRows - 1));
                    const auto t = glm::translate(glm::mat4(1.0), glm::vec3(x, y, 0));
                    state->transform = model * t;
                    state->height = tile_height(x, hexagon_heights_[i]);
                    ++state;
                }
            }
            hexagon_states_.unmap();
        }

        // diamonds

        {
            auto *state = diamond_states_.map();
            for (int i = 0; i < GridRows - 1; ++i) {
                for (int j = 0; j < GridColumns - 1; ++j) {
                    const auto x = 2.0 * cos_30 * (j - (0.5 * (GridColumns - 2)));
                    const auto y = 2.0 * (i - 0.5 * (GridRows - 2));
                    const auto t = glm::translate(glm::mat4(1.0), glm::vec3(x, y, 0));
                    state->transform = model * t;
                    state->height = tile_height(x, diamond_heights_[i]);
                    ++state;
                }
            }
            diamond_states_.unmap();
        }
    }

    void draw_grid(const gl::shader_program &program, const glm::mat4 &model, float x_offset) const
    {
        // hexagons
        hexagon_.bind();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, hexagon_states_.handle());
        glDrawArraysInstanced(GL_LINE_LOOP, 0, 6, GridRows * GridColumns);

        // diamonds
        diamond_.bind();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, diamond_states_.handle());
        glDrawArraysInstanced(GL_LINE_LOOP, 0, 4, (GridRows - 1) * (GridColumns - 1));
    }

    static constexpr auto NumStrips = 3;

    static constexpr auto ShadowWidth = 2048;
    static constexpr auto ShadowHeight = ShadowWidth;

    static constexpr auto GridRows = 12;
    static constexpr auto GridColumns = 15;

    std::array<float, GridRows> hexagon_heights_;
    std::array<float, GridRows - 1> diamond_heights_;

    int window_width_;
    int window_height_;
    float cur_time_ = 0;
    gl::shader_program program_;
    gl::shader_program shadow_program_;
    using Vertex = std::tuple<glm::vec2>;
    gl::geometry hexagon_;
    gl::geometry diamond_;
    gl::shadow_buffer shadow_buffer_;
    struct TileState {
        glm::mat4 transform;
        float height;
        float padding[3];
    };
    gl::buffer<TileState> hexagon_states_;
    gl::buffer<TileState> diamond_states_;
};

int main()
{
    constexpr auto window_width = 1024;
    constexpr auto window_height = 1024;

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
