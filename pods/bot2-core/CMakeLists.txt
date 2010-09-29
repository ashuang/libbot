cmake_minimum_required(VERSION 2.6.0)

project(bot2-core)

include(cmake/pods.cmake)

include(cmake/lcmtypes.cmake)
lcmtypes_build(C_AGGREGATE_HEADER bot_core.h
    JAVA_DEST_DIR ${PROJECT_SOURCE_DIR}/java/src)

add_subdirectory(src/bot_core)

add_subdirectory(java)
