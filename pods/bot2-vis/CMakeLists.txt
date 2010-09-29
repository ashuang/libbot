cmake_minimum_required(VERSION 2.6.0)

project(bot2-vis)

include_directories(${PROJECT_SOURCE_DIR}/src)

include(cmake/pods.cmake)

find_package(PkgConfig REQUIRED)
find_package(GTK2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(LCM REQUIRED lcm)
pkg_check_modules(BOT2_CORE REQUIRED bot2-core)

add_definitions(-std=gnu99)

add_subdirectory(src/bot_vis)
add_subdirectory(src/testers)
add_subdirectory(src/rwx-viewer)
