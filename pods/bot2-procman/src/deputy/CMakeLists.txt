add_definitions(-std=gnu99)

add_executable(bot-procman-deputy
    lcm_util.c
    procinfo.c
    procman.c
    procman_deputy.c
    signal_pipe.c)

pods_use_pkg_config_packages(bot-procman-deputy 
    glib-2.0 
    gthread-2.0 
    lcm)
target_link_libraries(bot-procman-deputy ${LCMTYPES_LIBS})

pods_install_executables(bot-procman-deputy)
