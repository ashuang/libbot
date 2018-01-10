add_definitions(-std=gnu99)

add_executable(bot-lcmgl-viewer
    main.c
    udp_util.c
    view_menu.c
    )

target_link_libraries(bot-lcmgl-viewer gthread-2.0)

pods_use_pkg_config_packages(bot-lcmgl-viewer bot2-vis bot2-lcmgl-renderer)


pods_install_executables(bot-lcmgl-viewer)
