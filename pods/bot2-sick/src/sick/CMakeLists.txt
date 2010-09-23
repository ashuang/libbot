add_definitions(-std=gnu99)

include_directories(${GLIB2_INCLUDE_DIRS} 
    ${LCM_INCLUE_DIRS}
    ${BOT2_CORE_INCLUDE_DIRS})

add_executable(bot-sick
    main.c
    sick.c
    moxa.c)

target_link_libraries(bot-sick
    ${GLIB2_LDFLAGS}
    ${LCM_LDFLAGS}
    ${BOT2_CORE_LDFLAGS})

install(TARGETS bot-sick RUNTIME DESTINATION bin)
