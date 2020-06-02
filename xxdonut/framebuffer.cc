#include "framebuffer.h"

framebuffer::framebuffer(int width, int height)
    : width_{width}
    , height_{height}
{
	glGenTextures(1, &texture_id_);
	glGenFramebuffers(1, &fbo_id_);
    glGenRenderbuffers(1, &rbo_id_);

	// initialize texture

	// note to self: non-power of 2 textures are allowed on es >2.0 if
	// GL_MIN_FILTER is set to a function that doesn't require mipmaps
	// and texture wrap is set to GL_CLAMP_TO_EDGE

	bind_texture();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    unbind_texture();

	// initialize framebuffer/renderbuffer

	bind();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id_, 0);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_id_);
    unbind();
}

framebuffer::~framebuffer()
{
	glDeleteFramebuffers(1, &fbo_id_);
    glDeleteRenderbuffers(1, &rbo_id_);
}

void
framebuffer::bind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_id_);
}

void
framebuffer::unbind()
{
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
framebuffer::bind_texture() const
{
	glBindTexture(GL_TEXTURE_2D, texture_id_);
}

void
framebuffer::unbind_texture()
{
	glBindTexture(GL_TEXTURE_2D, 0);
}
