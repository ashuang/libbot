cmake_minimum_required(VERSION 2.6.0)

project(bot2-lcm-utils)

include(cmake/pods.cmake)

add_subdirectory(src/logfilter)
add_subdirectory(src/logsplice)
add_subdirectory(src/who)
add_subdirectory(src/tunnel)
