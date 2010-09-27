cmake_minimum_required(VERSION 2.4.6)

project(bot2-sick)

include(cmake/pods.cmake)

find_package(PkgConfig REQUIRED)
pkg_check_modules(BOT2_CORE REQUIRED bot2-core)

add_subdirectory(src/sick)
