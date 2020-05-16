#pragma once

#include "noncopyable.h"

#include <GL/glew.h>

namespace gl {

class framebuffer : private noncopyable
{
public:
    framebuffer(int width, int height, GLint internal_format = GL_RGBA, GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE);
    ~framebuffer();

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
