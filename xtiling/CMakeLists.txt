set(ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(DEST_ASSETS "${CMAKE_CURRENT_BINARY_DIR}/shaders")
 
add_custom_command(
    OUTPUT ${DEST_ASSETS}
    COMMAND ln -s ${ASSET_DIR} ${DEST_ASSETS}
    DEPENDS ${ASSET_DIR})

add_executable(xtiling main.cc ${DEST_ASSETS})
target_link_libraries(xtiling common)
