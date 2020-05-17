#include "shadow_buffer.h"

namespace gl {

shadow_buffer::shadow_buffer(int width, int height)
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width_, height_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    unbind_texture();

    // initialize shadow_buffer/renderbuffer

    bind();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_id_, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    unbind();
}

shadow_buffer::~shadow_buffer()
{
    glDeleteFramebuffers(1, &fbo_id_);
    glDeleteTextures(1, &texture_id_);
}

void shadow_buffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
}

void shadow_buffer::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void shadow_buffer::bind_texture() const
{
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}

void shadow_buffer::unbind_texture() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace gl
