add_definitions(-std=gnu99)

include_directories(${GLIB2_INCLUDE_DIRS} 
    ${GTK2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${LCM_INCLUE_DIRS}
    ${BOT2_CORE_INCLUDE_DIRS})

add_executable(bot-lcmgl-viewer
    globals.c
    main.c
    renderer_lcmgl.c
    renderer_grid.c
    udp_util.c
    view_menu.c)

target_link_libraries(bot-lcmgl-viewer
    ${GLIB2_LDFLAGS}
    ${GTK2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${LCM_LDFLAGS}
    ${BOT2_CORE_LDFLAGS}
    bot2-vis
    -lglut)

install(TARGETS bot-lcmgl-viewer RUNTIME DESTINATION bin)
