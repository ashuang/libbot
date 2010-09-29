add_definitions(-std=gnu99)

add_executable(bot-sick
    main.c
    sick.c
    moxa.c)

pods_use_pkg_config_packages(bot-sick bot2-core)

pods_install_executables(bot-sick)
