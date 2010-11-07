add_definitions(-std=gnu99)

include_directories(${LCMTYPES_INCLUDE_DIRS})

set(libname bot2-param-client)

add_library(${libname} SHARED
    param_client.c)
    
target_link_libraries(${libname}
   ${LCMTYPES_LIBS})

pods_use_pkg_config_packages(${libname} lcm glib-2.0)

# set the library API version.  Increment this every time the public API
# changes.
set_target_properties(${libname} PROPERTIES SOVERSION 1)

# make the header public
pods_install_headers(param_client.h DESTINATION param_client)

# make the library public
pods_install_libraries(${libname})

# create a pkg-config file for the library, to make it for other software to
# use it.
pods_install_pkg_config_file(${libname}
    LIBS -l${libname} ${LCMTYPES_LIBS}
    REQUIRES lcm glib-2.0
    VERSION 0.0.1)
