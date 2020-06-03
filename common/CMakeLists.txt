add_library(common STATIC
    shader_program.cc
    util.cc
    window.cc
    demo.cc
    tween.cc
    shadow_buffer.cc
    multi_shadow_buffer.cc
    framebuffer.cc)

target_link_libraries(common
    PUBLIC
    ${OPENGL_LIBRARIES}
    ${GLFW3_LIBRARY}
    ${GLEW_LIBRARIES})

target_include_directories(common
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
