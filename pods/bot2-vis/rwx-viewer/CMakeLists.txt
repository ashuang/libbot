add_definitions(-std=gnu99)

include_directories(${GTK2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR})

add_executable(bot-rwx-viewer
    main.c
    renderer_rwx.c)

target_link_libraries(bot-rwx-viewer
    ${GTK2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    bot2-vis
    -lglut)

install(TARGETS bot-rwx-viewer RUNTIME DESTINATION bin)
