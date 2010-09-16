cmake_minimum_required(VERSION 2.4.6)

project(bot2-lidar)

include(cmake/pods.cmake)
pods_config_search_paths()

add_executable(hello src/hello.c)

install(TARGETS hello RUNTIME DESTINATION bin)
