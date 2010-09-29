add_definitions(-std=gnu99)

file(GLOB c_files *.c)
file(GLOB h_files *.h)

add_library(bot2-core SHARED ${c_files})

# set the library API version.  Increment this every time the public API
# changes.
set_target_properties(bot2-core PROPERTIES SOVERSION 1)

pods_use_pkg_config_packages(bot2-core 
    glib-2.0 lcm)

target_link_libraries(bot2-core
    ${LCMTYPES_LIBS}
    m)

pods_install_libraries(bot2-core)

pods_install_headers(${h_files} DESTINATION bot_core)

pods_install_pkg_config_file(${PROJECT_NAME}
    LIBS -lbot2-core -llcmtypes_bot2-core
    REQUIRES glib-2.0 lcm
    VERSION 0.0.1)
