cmake_minimum_required(VERSION 2.4.6)

project(lcmgl)

include(cmake/pods.cmake)
pods_config_search_paths()

include(cmake/lcmtypes.cmake)
lcmtypes_build()

find_package(PkgConfig REQUIRED)
find_package(GTK2 REQUIRED)
find_package(OpenGL REQUIRED)

pkg_check_modules(LCM REQUIRED lcm)
pkg_check_modules(BOT2_VIS REQUIRED bot2-vis)

add_definitions(-std=gnu99)

add_subdirectory(lcmgl-client)
add_subdirectory(lcmgl-render)
add_subdirectory(lcmgl-viewer)
