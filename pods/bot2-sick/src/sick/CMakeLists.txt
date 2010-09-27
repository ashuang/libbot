add_definitions(-std=gnu99)

include_directories(
    ${BOT2_CORE_INCLUDE_DIRS}
    )

add_executable(bot-sick
    main.c
    sick.c
    moxa.c)

target_link_libraries(bot-sick
    ${BOT2_CORE_LDFLAGS}
    )

install(TARGETS bot-sick RUNTIME DESTINATION bin)
