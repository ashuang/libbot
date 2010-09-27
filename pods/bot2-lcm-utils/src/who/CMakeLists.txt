add_definitions(-std=gnu99)

include_directories(${GLIB2_INCLUDE_DIRS} 
    ${LCM_INCLUDE_DIRS})

add_executable(bot-lcm-who
    lcm-who.c
    signal_pipe.c)

target_link_libraries(bot-lcm-who
    ${GLIB2_LDFLAGS}
    ${LCM_LDFLAGS})

install(TARGETS bot-lcm-who RUNTIME DESTINATION bin)
