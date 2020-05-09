#pragma once

#include "noncopyable.h"

#include <GL/glew.h>

namespace gl {

template <typename T>
class buffer : private noncopyable
{
public:
    buffer(GLenum target, const T *data, size_t size)
        : target_(target)
    {
        glGenBuffers(1, &id_);

        bind();
        glBufferData(target_, size * sizeof(T), data, GL_STATIC_DRAW);
    }

    buffer(GLenum target, size_t size)
        : buffer(target, nullptr, size)
    {
    }

    ~buffer()
    {
        glDeleteBuffers(1, &id_);
    }

    buffer(const buffer&) = delete;
    buffer& operator=(const buffer&) = delete;

    GLuint handle() const
    {
        return id_;
    }

    void bind() const
    {
        glBindBuffer(target_, id_);
    }

    void unbind() const
    {
        glBindBuffer(target_, 0);
    }

    void set_sub_data(size_t offset, const T *data, size_t size) const
    {
        bind();
        glBufferSubData(target_, offset * sizeof(T), size * sizeof(T), data);
    }

    void get_sub_data(size_t offset, T *data, size_t size) const
    {
        bind();
        glGetBufferSubData(target_, offset * sizeof(T), size * sizeof(T), data);
    }

    T *map() const
    {
        bind();
        return static_cast<T *>(glMapBuffer(target_, GL_WRITE_ONLY));
    }

    void unmap() const
    {
        bind();
        glUnmapBuffer(target_);
    }

private:
    GLenum target_;
    GLuint id_;
};

}
