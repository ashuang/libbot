add_definitions(-std=gnu99)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${LCMTYPES_INCLUDE_DIRS})

# Create an executable program param-tester
add_executable(param-tester param_tester.c)
pods_use_pkg_config_packages(param-tester lcm glib-2.0)
target_link_libraries(param-tester bot2-param-client ${LCMTYPES_LIBS})

# make executable public
#pods_install_executables(param-tester)
