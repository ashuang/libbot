add_definitions(-std=gnu99)

include_directories(
    ${GLIB2_INCLUDE_DIRS} 
    ${LCM_INCLUDE_DIRS})

file(GLOB c_files *.c)
file(GLOB h_files *.h)

set(libname bot2-lcmgl-client)

add_library(${libname} SHARED ${c_files})

target_link_libraries(${libname}
    ${GLIB2_LDFLAGS}
    ${LCM_LDFLAGS}
    ${LCMTYPES_LIBS})

# set the library API version.  Increment this every time the public API
# changes.
set_target_properties(${libname} PROPERTIES SOVERSION 1)

# make both the shared library public
pods_install_libraries(${libname})

pods_install_headers(${h_files} DESTINATION bot_lcmgl_client)

pods_install_pkg_config_file(bot2-lcmgl-client
    LIBS -l${libname} -lz
    REQUIRES glib-2.0 lcm
    VERSION 0.0.1)
