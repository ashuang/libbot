add_definitions(-std=gnu99)

include_directories(
    ${GTK2_INCLUDE_DIRS} 
    ${OPENGL_INCLUDE_DIR}
    ${BOT2_VIS_INCLUDE_DIRS}
    ${LCM_INCLUDE_DIRS})

file(GLOB c_files *.c)
file(GLOB h_files *.h)

add_library(bot2-lcmgl-renderer SHARED ${c_files})

target_link_libraries(bot2-lcmgl-renderer
    ${GTK2_LDFLAGS}
    ${OPENGL_LIBRARIES}
    ${BOT2_VIS_LDFLAGS}
    ${LCM_LDFLAGS}
    ${LCMTYPES_LIBS})

# set the library API version.  Increment this every time the public API
# changes.
set_target_properties(bot2-lcmgl-renderer PROPERTIES SOVERSION 1)

pods_install_libraries(bot2-lcmgl-renderer)

pods_install_headers(${h_files} DESTINATION bot_lcmgl_render)

pods_install_pkg_config_file(bot2-lcmgl-renderer
    LIBS -lbot2-lcmgl-render
    REQUIRES gtk+-2.0 bot2-vis lcm
    VERSION 0.0.1)
