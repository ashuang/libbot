add_definitions(-std=gnu99)

include_directories(${GLIB2_INCLUDE_DIRS} 
    ${LCM_INCLUDE_DIRS})

add_executable(bot-lcm-logfilter
    lcm-logfilter.c)

target_link_libraries(bot-lcm-logfilter
    ${GLIB2_LDFLAGS}
    ${LCM_LDFLAGS})

install(TARGETS bot-lcm-logfilter RUNTIME DESTINATION bin)
