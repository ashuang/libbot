add_definitions(-std=gnu99)

include_directories(
    ${GTK2_INCLUDE_DIRS} 
    ${OPENGL_INCLUDE_DIR}
    ${BOT2_VIS_INCLUDE_DIRS}
    ${LCM_INCLUDE_DIRS}
    ${LCMTYPES_INCLUDE_DIRS})

file(GLOB c_files *.c)
file(GLOB h_files *.h)
add_library(lcmgl-render SHARED ${c_files})

target_link_libraries(lcmgl-render
    ${GTK2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${BOT2_VIS_LDFLAGS}
    ${LCM_LDFLAGS}
    ${LCMTYPES_LIBS})

# set the library API version.  Increment this every time the public API
# changes.
set_target_properties(lcmgl-render PROPERTIES SOVERSION 1)

# make both the shared library public
install(TARGETS lcmgl-render LIBRARY DESTINATION lib)

install(FILES ${h_files} DESTINATION include/lcmgl)

pods_create_pkg_config_file(${PROJECT_NAME}
    LIBS -llcmgl-render
    VERSION 0.0.1)
