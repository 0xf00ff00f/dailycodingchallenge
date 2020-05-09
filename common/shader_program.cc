#include "shader_program.h"

#include "panic.h"

#include <array>
#include <fstream>
#include <sstream>

#include <glm/gtc/type_ptr.hpp>

namespace gl {

namespace {

static std::vector<char> load_file(const char *path)
{
    std::ifstream file(path);
    if (!file.is_open())
        panic("failed to open %s\n", std::string(path).c_str());

    auto *buf = file.rdbuf();

    const std::size_t size = buf->pubseekoff(0, file.end, file.in);
    buf->pubseekpos(0, file.in);

    std::vector<char> data(size + 1);
    buf->sgetn(data.data(), size);
    data[size] = 0;

    file.close();

    return data;
}

}

shader_program::shader_program()
    : id_{ glCreateProgram() }
{
}

void shader_program::add_shader(GLenum type, const char *path)
{
    const auto shader_id = glCreateShader(type);

    const auto source = load_file(path);
    const auto source_ptr = source.data();
    glShaderSource(shader_id, 1, &source_ptr, nullptr);
    glCompileShader(shader_id);

    int status;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &status);
    if (!status) {
        std::array<GLchar, 64 * 1024> buf;
        GLsizei length;
        glGetShaderInfoLog(shader_id, buf.size() - 1, &length, buf.data());
        panic("failed to compile shader %s:\n%.*s", path, length, buf.data());
    }

    glAttachShader(id_, shader_id);
}

void shader_program::link()
{
    glLinkProgram(id_);

    int status;
    glGetProgramiv(id_, GL_LINK_STATUS, &status);
    if (!status)
        panic("failed to link shader program\n");
}

void shader_program::bind() const
{
    glUseProgram(id_);
}

int shader_program::uniform_location(std::string_view name) const
{
    return glGetUniformLocation(id_, name.data());
}

void shader_program::set_uniform(int location, int value) const
{
    glUniform1i(location, value);
}

void shader_program::set_uniform(int location, float value) const
{
    glUniform1f(location, value);
}

void shader_program::set_uniform(int location, const glm::vec2 &value) const
{
    glUniform2fv(location, 1, glm::value_ptr(value));
}

void shader_program::set_uniform(int location, const glm::vec3 &value) const
{
    glUniform3fv(location, 1, glm::value_ptr(value));
}

void shader_program::set_uniform(int location, const glm::vec4 &value) const
{
    glUniform4fv(location, 1, glm::value_ptr(value));
}

void shader_program::set_uniform(int location, const std::vector<float> &value) const
{
    glUniform1fv(location, value.size(), value.data());
}

void shader_program::set_uniform(int location, const std::vector<glm::vec2> &value) const
{
    glUniform2fv(location, value.size(), reinterpret_cast<const float *>(value.data()));
}

void shader_program::set_uniform(int location, const std::vector<glm::vec3> &value) const
{
    glUniform3fv(location, value.size(), reinterpret_cast<const float *>(value.data()));
}

void shader_program::set_uniform(int location, const std::vector<glm::vec4> &value) const
{
    glUniform4fv(location, value.size(), reinterpret_cast<const float *>(value.data()));
}

void shader_program::set_uniform(int location, const glm::mat3 &value) const
{
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void shader_program::set_uniform(int location, const glm::mat4 &value) const
{
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

}
