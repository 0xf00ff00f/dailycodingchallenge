#pragma once

#include "noncopyable.h"

#include <GL/glew.h>

class framebuffer : private noncopyable
{
public:
	framebuffer(int width, int height);
	~framebuffer();

	void bind() const;
	static void unbind();

	void bind_texture() const;
    static void unbind_texture();

    int width() const { return width_; }
    int height() const { return height_; }

private:
	void init_texture();

    int width_;
    int height_;
	GLuint texture_id_;
	GLuint fbo_id_, rbo_id_;
};
