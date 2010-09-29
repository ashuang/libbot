add_definitions(-std=gnu99)

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

pods_use_pkg_config_packages(bot-hokuyo 
    bot2-core lcm)

pods_install_executables(bot-hokuyo)
