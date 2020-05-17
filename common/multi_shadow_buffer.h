#pragma once

#include "noncopyable.h"

#include <GL/glew.h>

#include <vector>

namespace gl {

class multi_shadow_buffer : private noncopyable
{
public:
    multi_shadow_buffer(int width, int height, int layers);
    ~multi_shadow_buffer();

    void bind(int layer) const;
    static void unbind();

    void bind_texture() const;
    void unbind_texture() const;

    int width() const { return width_; }
    int height() const { return height_; }

private:
    int width_;
    int height_;
    int layers_;
    GLuint texture_id_;
    std::vector<GLuint> fbo_id_;
};

} // namespace gl
