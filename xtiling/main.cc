#include <demo.h>
#include <shader_program.h>
#include <tween.h>
#include <geometry.h>
#include <shadow_buffer.h>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <random>
#include <algorithm>

class Demo : public gl::demo
{
public:
    Demo(int argc, char *argv[])
        : gl::demo(argc, argv)
        , shadow_buffer_(ShadowWidth, ShadowHeight)
    {
        initialize_shader();
        initialize_geometry();
        initialize_flips();
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
        using Vertex = std::tuple<glm::vec2>;
        static const std::vector<Vertex> verts = {
            { glm::vec2(-1, 0.5) },
            { glm::vec2(-0.5, 0.5) },
            { glm::vec2(-0.5, 1) },
            { glm::vec2(0.5, 1) },
            { glm::vec2(0.5, 0.5) },
            { glm::vec2(1, 0.5) },
            { glm::vec2(1, -0.5) },
            { glm::vec2(0.5, -0.5) },
            { glm::vec2(0.5, -1) },
            { glm::vec2(-0.5, -1) },
            { glm::vec2(-0.5, -0.5) },
            { glm::vec2(-1, -0.5) }
        };
        tile_.set_data(verts);
    }

    void initialize_flips()
    {
        std::random_device device;
        std::mt19937 generator(device());
        std::normal_distribution<> d0(0.25 * cycle_duration_, 0.125);
        std::normal_distribution<> d1(0.75 * cycle_duration_, 0.125);

        for (int i = 0; i < GridRows; ++i)
        {
            for (int j = 0; j < GridColumns; ++j)
            {
                auto &animation = flip_start_[i][j];
                animation.s0 = std::clamp(static_cast<float>(d0(generator)), 0.0f, static_cast<float>(cycle_duration_));
                animation.s1 = std::clamp(static_cast<float>(d1(generator)), 0.0f, static_cast<float>(cycle_duration_));
                animation.flop = rand() % 4;
                animation.h0 = 1.0 + 7.0 * static_cast<float>(rand()) / RAND_MAX;
                animation.h1 = 1.0 + 7.0 * static_cast<float>(rand()) / RAND_MAX;
            }
        }
    }

    void update(float dt) override
    {
        cur_time_ += dt;
    }

    void render() override
    {
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        const auto light_position = glm::vec3(-6, 4, 6) * 2.5f;

#if 0
        const float angle = 2.0 * M_PI * cur_time_ / cycle_duration_;
        auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0));
#else
        auto model = glm::rotate(glm::mat4(1.0f), static_cast<float>(0.25 * M_PI), glm::vec3(0, 0, 1));
#endif

        // shadow

        const auto light_projection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 1.0f, 50.0f);
        const auto light_view = glm::lookAt(light_position, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

        glViewport(0, 0, ShadowWidth, ShadowHeight);
        shadow_buffer_.bind();

        glClear(GL_DEPTH_BUFFER_BIT);

        shadow_program_.bind();
        shadow_program_.set_uniform("viewProjectionMatrix", light_projection * light_view);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(4, 4);

        glDisable(GL_CULL_FACE);
        draw_grid(model, shadow_program_);

        glDisable(GL_POLYGON_OFFSET_FILL);
        shadow_buffer_.unbind();

        // render

        glViewport(0, 0, width_, height_);
        glClearColor(0, 0, 0, 0);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const auto projection =
                glm::perspective(glm::radians(45.0f), static_cast<float>(width_) / height_, 0.1f, 100.f);
        const auto camera_position = /* glm::vec3(0, 0, 7); */ glm::vec3(0, -6, 15) * 1.5f;
        const auto look_at = glm::vec3(0, 0, 0);
        const auto view = glm::lookAt(camera_position, look_at, glm::vec3(0, 1, 0));

        shadow_buffer_.bind_texture();

        program_.bind();
        program_.set_uniform("viewProjectionMatrix", projection * view);
        program_.set_uniform("modelMatrix", model);
        program_.set_uniform("lightPosition", light_position);
        program_.set_uniform("lightViewProjection", light_projection * light_view);
        program_.set_uniform("shadowMapTexture", 0);

        draw_grid(model, program_);
    }

    void draw_grid(const glm::mat4 &model, gl::shader_program &program)
    {
        tile_.bind();

        for (int i = 0; i < GridRows; ++i)
        {
            for (int j = 0; j < GridColumns; ++j)
            {
                auto x = 2.0 * (j - (0.5 * GridColumns -1));
                if (i % 2)
                    x += 1.0;
                const auto y = 1.5 * (i - 0.5 * (GridRows - 1));
                const auto t = glm::translate(glm::mat4(1.0), glm::vec3(x, y, 0));
                const auto &animation = flip_start_[i][j];
                float a, h;
                if (cur_time_ < animation.s0) {
                    a = h = 0;
                } else if (cur_time_ < animation.s0 + FlipDuration) {
                    const auto t = (cur_time_ - animation.s0) / FlipDuration;
                    a = in_quadratic(t) * M_PI;
                    h = (1 - (2*t - 1)*(2*t - 1)) * animation.h0;
                } else if (cur_time_ < animation.s1) {
                    a = M_PI;
                    h = 0;
                } else if (cur_time_ < animation.s1 + FlipDuration) {
                    const auto t = (cur_time_ - animation.s1) / FlipDuration;
                    a = M_PI + in_quadratic(t) * M_PI;
                    h = (1 - (2*t - 1)*(2*t - 1)) * animation.h1;
                } else {
                    a = 2.0 * M_PI;
                    h = 0;
                }
                glm::mat4 r0 = glm::rotate(glm::mat4(1.0), a, glm::vec3(1, 0, 0));
                glm::mat4 r1 = glm::rotate(glm::mat4(1.0), static_cast<float>(animation.flop * 0.5 * M_PI), glm::vec3(0, 0, 1));
                glm::mat4 ts = glm::translate(glm::mat4(1.0), glm::vec3(0, 0, h));
                program.set_uniform("modelMatrix", model * t * ts * r1 * r0);
                glDrawArrays(GL_LINE_LOOP, 0, 12);
            }
        }
    }

    static constexpr const auto GridColumns = 25;
    static constexpr const auto GridRows = 25;
    static constexpr const auto FlipDuration = 0.8f;

    static constexpr auto ShadowWidth = 2048;
    static constexpr auto ShadowHeight = ShadowWidth;

    float cur_time_ = 0.0f;
    gl::shader_program program_;
    gl::shader_program shadow_program_;
    gl::geometry tile_;
    gl::shadow_buffer shadow_buffer_;
    struct TileAnimation
    {
        float s0;
        float s1;
        float h0;
        float h1;
        int flop;
    };
    std::array<std::array<TileAnimation, GridColumns>, GridRows> flip_start_;
};

int main(int argc, char *argv[])
{
    Demo(argc, argv).run();
}
