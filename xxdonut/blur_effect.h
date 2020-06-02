#pragma once

#include "noncopyable.h"

#include "framebuffer.h"
#include "geometry.h"

class blur_effect : private noncopyable
{
public:
    blur_effect(int width, int height)
        : horiz_blur_fb_(width, height)
        , vert_blur_fb_(width, height)
    {
        quad_.set_data(std::vector<vertex>{
            {{-1, -1}, {0, 0}}, {{-1, 1}, {0, 1}}, {{1, -1}, {1, 0}}, {{1, 1}, {1, 1}}});
        program_.add_shader(GL_VERTEX_SHADER, "shaders/blur.vert");
        program_.add_shader(GL_FRAGMENT_SHADER, "shaders/blur.frag");
        program_.link();
    }

    int width() const { return horiz_blur_fb_.width(); }
    int height() const { return horiz_blur_fb_.height(); }

    void bind() const
    {
        horiz_blur_fb_.bind();
        glViewport(0, 0, vert_blur_fb_.width(), vert_blur_fb_.height());
    }

    void render(int width, int height) const
    {
        glDisable(GL_DEPTH_TEST);
        quad_.bind();

        program_.bind();
        program_.set_uniform(program_.uniform_location("image"), 0);

        vert_blur_fb_.bind();
        glViewport(0, 0, vert_blur_fb_.width(), vert_blur_fb_.height());
        program_.set_uniform(program_.uniform_location("horizontal"), 0);
        horiz_blur_fb_.bind_texture();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        framebuffer::unbind();
        glViewport(0, 0, width, height);
        program_.set_uniform(program_.uniform_location("horizontal"), 1);
        vert_blur_fb_.bind_texture();

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisable(GL_BLEND);
    }

private:
    using vertex = std::tuple<glm::vec2, glm::vec2>;
    gl::geometry quad_;
    gl::shader_program program_;
    framebuffer horiz_blur_fb_;
    framebuffer vert_blur_fb_;
};
