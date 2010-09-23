cmake_minimum_required(VERSION 2.4.6)

project(bot2-procman)

include(cmake/pods.cmake)
pods_config_search_paths()

# automatically build LCM types.  This also defines a number of CMake
# variables, see cmake/lcmtypes.cmake for details
include(cmake/lcmtypes.cmake)
lcmtypes_build(PY_DEST_DIR ${PROJECT_SOURCE_DIR}/python/src)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(LCM REQUIRED lcm)

add_subdirectory(src/deputy)
add_subdirectory(python)
