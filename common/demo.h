#pragma once

#include <memory>

namespace gl
{
class window;

class demo
{
public:
    demo(int argc, char *argv[]);
    virtual ~demo();

    void run();
    virtual void update(float dt) = 0;
    virtual void render() = 0;

protected:
    void parse_arguments(int argc, char *argv[]);

    std::unique_ptr<gl::window> window_;
    int width_ = 800;
    int height_ = 800;
    bool dump_frames_ = false;
    int cycle_duration_ = 3; // seconds
    int frames_per_second_ = 40;
};

}
