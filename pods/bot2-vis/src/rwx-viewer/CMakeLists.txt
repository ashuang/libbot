add_definitions(-std=gnu99)

include_directories(${GTK2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIR}
    )

add_executable(bot-rwx-viewer
    main.c
    renderer_rwx.c)

target_link_libraries(bot-rwx-viewer
    ${GTK2_LDFLAGS}
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    bot2-vis
    )

pods_install_executables(bot-rwx-viewer)
