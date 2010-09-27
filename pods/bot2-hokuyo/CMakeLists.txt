cmake_minimum_required(VERSION 2.6.0)

project(bot2-hokuyo)

include(cmake/pods.cmake)

find_package(PkgConfig REQUIRED)

pkg_check_modules(BOT2_CORE REQUIRED bot2-core)
pkg_check_modules(LCM REQUIRED lcm)

add_subdirectory(src/hokuyo)
