#include "util.h"

#include <GL/glew.h>

#include <vector>
#include <cstdio>

void dump_frame_to_file(const char *path, int width, int height)
{
    auto *out = std::fopen(path, "wb");
    if (!out)
        return;

    std::vector<char> frame_data(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, frame_data.data());

    std::fprintf(out, "P6\n%d %d\n255\n", width, height);
    const auto *p = frame_data.data();
    for (auto i = 0; i < width * height; ++i) {
        std::fputc(*p++, out);
        std::fputc(*p++, out);
        std::fputc(*p++, out);
        ++p;
    }

    std::fclose(out);
}
