cmake_minimum_required(VERSION 2.4.6)

set(POD_NAME bot2-lcmgl)

include(cmake/pods.cmake)

include(cmake/lcmtypes.cmake)
lcmtypes_build(JAVA_DEST_DIR ${PROJECT_SOURCE_DIR}/java/src
    PY_DEST_DIR ${PROJECT_SOURCE_DIR}/python/src)

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)

list(APPEND OPENGL_LIBRARIES GL )
set(GLUT_CFLAGS "")
set(GLUT_LIBRARIES -lglut)

pkg_check_modules(LCM REQUIRED lcm)
pkg_check_modules(GTK2 REQUIRED gtk+-2.0)
pkg_check_modules(BOT2_VIS REQUIRED bot2-vis)

add_definitions(-std=gnu99)

add_subdirectory(src/bot_lcmgl_client)
add_subdirectory(src/bot_lcmgl_render)
add_subdirectory(src/lcmgl-viewer)

add_subdirectory(java)
add_subdirectory(python)
