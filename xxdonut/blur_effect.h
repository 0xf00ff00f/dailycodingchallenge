#pragma once

#include "noncopyable.h"

#include "shader_program.h"
#include "framebuffer.h"
#include "geometry.h"

#include <memory>

class blur_effect : private noncopyable
{
public:
    blur_effect(int framebuffer_width, int framebuffer_height);

    int width() const { return framebuffer_width_; }
    int height() const { return framebuffer_height_; }

    void bind() const;
    void render(int width, int height, int passes) const;

private:
    int framebuffer_width_;
    int framebuffer_height_;
    using vertex = std::tuple<glm::vec2, glm::vec2>;
    gl::geometry quad_;
    gl::shader_program program_;
    std::vector<std::unique_ptr<gl::framebuffer>> framebuffers_;
};
