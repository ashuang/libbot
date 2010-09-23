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
    ${BOT2_CORE_LDFLAGS})

# set the library API version.  Increment this every time the public API
# changes.
set_target_properties(bot2-vis PROPERTIES SOVERSION 1)

# make the shared library public
install(TARGETS bot2-vis LIBRARY DESTINATION lib)

install(FILES ${h_files} DESTINATION include/bot_vis)

pods_create_pkg_config_file(${PROJECT_NAME}
    LIBS -lbot2-vis
    VERSION 0.0.1)
