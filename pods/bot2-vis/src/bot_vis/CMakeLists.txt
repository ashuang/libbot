include_directories(${PROJECT_SOURCE_DIR}
    ${GLIB2_INCLUDE_DIRS} 
    ${GTK2_INCLUDE_DIRS}
    ${LCM_INCLUDE_DIRS}
    ${BOT2_CORE_INCLUDE_DIRS})

file(GLOB c_files *.c)
file(GLOB h_files *.h)

add_library(bot2-vis SHARED ${c_files})

target_link_libraries(bot2-vis
    ${GLIB2_LDFLAGS}
    ${GTK2_LDFLAGS}
    ${LCM_LDFLAGS}
    ${GLUT_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${BOT2_CORE_LDFLAGS})

# set the library API version.  Increment this every time the public API
# changes.
set_target_properties(bot2-vis PROPERTIES SOVERSION 1)

pods_install_libraries(bot2-vis)

pods_install_headers(${h_files} DESTINATION bot_vis)

pods_install_pkg_config_file(${PROJECT_NAME}
    LIBS -lbot2-vis
    REQUIRES gtk+-2.0 lcm bot2-core 
    VERSION 0.0.1)
