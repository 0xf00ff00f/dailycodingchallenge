#include "framebuffer.h"

namespace gl {

framebuffer::framebuffer(int width, int height, GLint internal_format, GLenum format, GLenum type)
    : width_{ width }
    , height_{ height }
{
    glGenTextures(1, &texture_id_);
    glGenFramebuffers(1, &fbo_id_);
    // glGenRenderbuffers(1, &rbo_id_);

    // initialize texture

    // note to self: non-power of 2 textures are allowed on es >2.0 if
    // GL_MIN_FILTER is set to a function that doesn't require mipmaps
    // and texture wrap is set to GL_CLAMP_TO_EDGE

    bind_texture();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width_, height_, 0, format, type, nullptr);
    unbind_texture();

    // initialize framebuffer/renderbuffer

    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_id_, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    unbind();
}

framebuffer::~framebuffer()
{
    glDeleteFramebuffers(1, &fbo_id_);
}

void framebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
}

void framebuffer::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void framebuffer::bind_texture() const
{
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}

void framebuffer::unbind_texture() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace gl
