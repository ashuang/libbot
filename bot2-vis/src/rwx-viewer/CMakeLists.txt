add_definitions(-std=gnu99)

add_executable(bot-rwx-viewer
    main.c
    renderer_rwx.c)

target_link_libraries(bot-rwx-viewer gthread-2.0)

pods_use_pkg_config_packages(bot-rwx-viewer bot2-vis)

pods_install_executables(bot-rwx-viewer)
