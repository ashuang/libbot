add_definitions(-std=gnu99)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(LCM REQUIRED lcm)

include_directories(${GLIB2_INCLUDE_DIRS} 
    ${LCM_INCLUDE_DIRS}
    ${LCMTYPES_INCLUDE_DIRS})

add_executable(bot-procman-deputy
    lcm_util.c
    procinfo.c
    procman.c
    procman_deputy.c
    signal_pipe.c)

target_link_libraries(bot-procman-deputy
    ${GLIB2_LDFLAGS}
    ${LCM_LDFLAGS}
    ${LCMTYPES_LIBS})

pods_install_executables(bot-procman-deputy)
