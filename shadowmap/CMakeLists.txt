set(ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(DEST_ASSETS "${CMAKE_CURRENT_BINARY_DIR}/assets")

add_custom_command(
    OUTPUT ${DEST_ASSETS}
    COMMAND ln -s ${ASSET_DIR} ${DEST_ASSETS}
    DEPENDS ${ASSET_DIR})

add_executable(shadowmap main.cc ${DEST_ASSETS})

target_link_libraries(
    shadowmap
    PUBLIC
    ${OPENGL_LIBRARIES}
    ${GLFW3_LIBRARY}
    ${GLEW_LIBRARIES}
    common)

target_include_directories(
    shadowmap
    PUBLIC
    "${PROJECT_SOURCE_DIR}/common")
