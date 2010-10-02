add_definitions(-std=gnu99)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${LCMTYPES_INCLUDE_DIRS})

# Create an executable program bot-param-server
add_executable(bot-param-server param_server.c lcm_util.c)
pods_use_pkg_config_packages(bot-param-server lcm glib-2.0)
target_link_libraries(bot-param-server bot2-param-client ${LCMTYPES_LIBS})

# Create an executable program bot-param-tool
add_executable(bot-param-tool param_tool.c)
pods_use_pkg_config_packages(bot-param-tool lcm)
target_link_libraries(bot-param-tool bot2-param-client ${LCMTYPES_LIBS})


# make executables public
pods_install_executables(bot-param-server bot-param-tool)

