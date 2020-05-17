#include "multi_shadow_buffer.h"

namespace gl {

multi_shadow_buffer::multi_shadow_buffer(int width, int height, int layers)
    : width_{ width }
    , height_{ height }
    , layers_{ layers }
{
    glGenTextures(1, &texture_id_);

    fbo_id_.resize(layers);
    glGenFramebuffers(layers, fbo_id_.data());

    bind_texture();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, width_, height_, layers_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    unbind_texture();

    for (int i = 0; i < layers; ++i)
    {
        bind(i);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_id_, 0, i);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        unbind();
    }
}

multi_shadow_buffer::~multi_shadow_buffer()
{
    glDeleteFramebuffers(layers_, fbo_id_.data());
    glDeleteTextures(1, &texture_id_);
}

void multi_shadow_buffer::bind(int layer) const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_[layer]);
}

void multi_shadow_buffer::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void multi_shadow_buffer::bind_texture() const
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id_);
}

void multi_shadow_buffer::unbind_texture() const
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

} // namespace gl
