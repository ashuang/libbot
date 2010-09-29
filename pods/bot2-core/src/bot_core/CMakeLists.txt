add_definitions(-std=gnu99)

include_directories(
    ${GLIB2_INCLUDE_DIRS} 
    ${LCM_INCLUDE_DIRS})

file(GLOB c_files *.c)
file(GLOB h_files *.h)
add_library(bot2-core SHARED ${c_files})

target_link_libraries(bot2-core
    ${GLIB2_LDFLAGS}
    ${LCM_LDFLAGS}
    ${LCMTYPES_LIBS}
    m)

# set the library API version.  Increment this every time the public API
# changes.
set_target_properties(bot2-core PROPERTIES SOVERSION 1)

# make the shared library public
pods_install_libraries(bot2-core)
pods_install_headers_to(${h_files} bot_core)

#create a pkg-config file for bot2-core
pods_create_pkg_config_file(${PROJECT_NAME}
    LIBS -lbot2-core -llcmtypes_bot2-core
    REQUIRES glib-2.0 lcm
    VERSION 0.0.1)
