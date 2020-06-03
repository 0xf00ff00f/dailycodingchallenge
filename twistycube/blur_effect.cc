#include "blur_effect.h"

blur_effect::blur_effect(int framebuffer_width, int framebuffer_height)
    : framebuffer_width_(framebuffer_width)
    , framebuffer_height_(framebuffer_height)
{
    framebuffers_.emplace_back(new gl::framebuffer(framebuffer_width, framebuffer_height));
    framebuffers_.emplace_back(new gl::framebuffer(framebuffer_width, framebuffer_height));
    quad_.set_data(std::vector<vertex>{
            { { -1, -1 }, { 0, 0 } }, { { -1, 1 }, { 0, 1 } }, { { 1, -1 }, { 1, 0 } }, { { 1, 1 }, { 1, 1 } } });
    program_.add_shader(GL_VERTEX_SHADER, "shaders/blur.vert");
    program_.add_shader(GL_FRAGMENT_SHADER, "shaders/blur.frag");
    program_.link();
}

void blur_effect::bind() const
{
    framebuffers_[0]->bind();
    glViewport(0, 0, framebuffer_width_, framebuffer_height_);
}

void blur_effect::render(int width, int height, int passes) const
{
    glDisable(GL_DEPTH_TEST);
    quad_.bind();

    program_.bind();
    program_.set_uniform(program_.uniform_location("image"), 0);

    for (int i = 0; i < passes; ++i) {
        // 0 -> 1

        framebuffers_[1]->bind();
        glViewport(0, 0, framebuffer_width_, framebuffer_height_);

        framebuffers_[0]->bind_texture();
        program_.set_uniform(program_.uniform_location("horizontal"), 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // 1 -> 0 or screen

        if (i < passes - 1) {
            framebuffers_[0]->bind();
            glViewport(0, 0, framebuffer_width_, framebuffer_height_);
        } else {
            // last pass, render to screen
            gl::framebuffer::unbind();
            glViewport(0, 0, width, height);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
        }

        framebuffers_[1]->bind_texture();
        program_.set_uniform(program_.uniform_location("horizontal"), 1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glDisable(GL_BLEND);
}
