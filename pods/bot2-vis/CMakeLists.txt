cmake_minimum_required(VERSION 2.6.0)

project(bot2-vis)

include_directories(${PROJECT_SOURCE_DIR})

include(cmake/pods.cmake)
pods_config_search_paths()

find_package(PkgConfig REQUIRED)
find_package(GTK2 REQUIRED)
find_package(OpenGL REQUIRED)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(LCM REQUIRED lcm)
pkg_check_modules(BOT2_CORE REQUIRED bot2-core)

add_definitions(-std=gnu99)

add_subdirectory(bot_vis)
add_subdirectory(testers)
