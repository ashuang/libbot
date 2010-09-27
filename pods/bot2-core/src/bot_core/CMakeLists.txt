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

# make both the shared library public
install(TARGETS bot2-core LIBRARY DESTINATION lib)

install(FILES ${h_files} DESTINATION include/bot_core)

pods_create_pkg_config_file(${PROJECT_NAME}
    LIBS -lbot2-core -llcmtypes_bot2-core
    REQUIRES glib-2.0 lcm
    VERSION 0.0.1)
