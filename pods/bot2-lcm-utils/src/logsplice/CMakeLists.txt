add_definitions(-std=gnu99)

include_directories(${GLIB2_INCLUDE_DIRS} 
    ${LCM_INCLUE_DIRS})

add_executable(bot-lcm-logsplice
    lcm-logsplice.c)

target_link_libraries(bot-lcm-logsplice
    ${GLIB2_LDFLAGS}
    ${LCM_LDFLAGS})

install(TARGETS bot-lcm-logsplice RUNTIME DESTINATION bin)
