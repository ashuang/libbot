add_definitions(-std=gnu99)

include_directories(
    ${BOT2_CORE_INCLUDE_DIRS} 
    ${LCM_INCLUDE_DIRS}
    )

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
    ${BOT2_CORE_LDFLAGS}
    ${LCM_LDFLAGS}
    )

install(TARGETS bot-hokuyo RUNTIME DESTINATION bin)
