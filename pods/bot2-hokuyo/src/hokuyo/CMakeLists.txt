add_definitions(-std=gnu99)

include_directories(${GLIB2_INCLUDE_DIRS}
    ${LCM_INCLUDE_DIRS}
    ${BOT2_CORE_INCLUDE_DIRS})

add_executable(bot-hokuyo
    hokuyo.c
    liburg/serial_ctrl.c
    liburg/serial_utils.c
    liburg/ring_buffer.c
    liburg/serial_errno.h
    liburg/delay.c
    liburg/ticks.c
    liburg/urg_ctrl.c
    liburg/scip_handler.c
    liburg/urg_errno.c
    liburg/scip_handler.h
    liburg/urg_errno.h)

target_link_libraries(bot-hokuyo
    ${GLIB2_LDFLAGS}
    ${LCM_LDFLAGS}
    ${BOT2_CORE_LDFLAGS})

install(TARGETS bot-hokuyo RUNTIME DESTINATION bin)
