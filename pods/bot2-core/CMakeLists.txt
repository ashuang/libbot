cmake_minimum_required(VERSION 2.4.6)

project(bot2-core)

include(cmake/pods.cmake)

include(cmake/lcmtypes.cmake)
lcmtypes_build(C_AGGREGATE_HEADER bot_core.h
    JAVA_DEST_DIR ${PROJECT_SOURCE_DIR}/java/src)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(LCM REQUIRED lcm)

add_subdirectory(src/bot_core)

add_subdirectory(java)
