cmake_minimum_required(VERSION 2.4.6)

project(bot2-core)

include(cmake/pods.cmake)
pods_config_search_paths()

include(cmake/lcmtypes.cmake)
lcmtypes_build()

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(LCM REQUIRED lcm)

add_subdirectory(bot2-core)
