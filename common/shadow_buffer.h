#pragma once

#include "noncopyable.h"

#include <GL/glew.h>

namespace gl {

class shadow_buffer : private noncopyable
{
public:
    shadow_buffer(int width, int height);
    ~shadow_buffer();

    void bind() const;
    void unbind() const;

    void bind_texture() const;
    void unbind_texture() const;

    int width() const { return width_; }
    int height() const { return height_; }

private:
    int width_;
    int height_;
    GLuint texture_id_;
    GLuint fbo_id_;
};

} // namespace gl
