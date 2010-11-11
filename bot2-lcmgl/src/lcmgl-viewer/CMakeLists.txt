add_definitions(-std=gnu99)

include_directories(${PROJECT_SOURCE_DIR}/src
    ${GTK2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIR}
    ${LCM_INCLUDE_DIRS}
    ${BOT2_VIS_INCLUDE_DIRS}
    )

add_executable(bot-lcmgl-viewer
    main.c
    udp_util.c
    view_menu.c
    )

target_link_libraries(bot-lcmgl-viewer
    ${GTK2_LDFLAGS}
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${LCM_LDFLAGS}
    ${BOT2_VIS_LDFLAGS}
    bot2-lcmgl-render
    )

pods_install_executables(bot-lcmgl-viewer)
