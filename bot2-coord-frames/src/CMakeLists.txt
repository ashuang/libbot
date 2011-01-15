add_definitions(-std=gnu99)
# Create a shared library libbot2-coord-frames.so with a single source file
add_library(bot2-coord-frames SHARED
    coord_frames.c)
    
pods_use_pkg_config_packages(bot2-coord-frames bot2-core bot2-param-client)

# set the library API version.  Increment this every time the public API
# changes.
set_target_properties(bot2-coord-frames PROPERTIES SOVERSION 1)

# make the header public
# install it to include/bot2-coord-frames
pods_install_headers(coord_frames.h DESTINATION bot_coord_frames)

# make the library public
pods_install_libraries(bot2-coord-frames)

# create a pkg-config file for the library, to make it for other software to
# use it.
pods_install_pkg_config_file(bot2-coord-frames
    CFLAGS
    LIBS -lbot2-coord-frames
    REQUIRES bot2-core bot2-param-client
    VERSION 0.0.1)
