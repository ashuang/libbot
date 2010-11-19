add_definitions(-std=gnu99)

include_directories(${GTK2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIR}
    )

add_executable(bot-plugin-viewer
    plugin_viewer.cpp)

target_link_libraries(bot-plugin-viewer
    ${GTK2_LDFLAGS}
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    bot2-vis
    )

pods_install_executables(bot-plugin-viewer)
