add_definitions(-std=gnu99)

add_executable(bot-lcmgl-viewer
    main.c
    udp_util.c
    view_menu.c
    )

pods_use_pkg_config_packages(bot-lcmgl-viewer bot2-vis bot2-lcmgl-renderer glib-2.0 gthread-2.0)


pods_install_executables(bot-lcmgl-viewer)
