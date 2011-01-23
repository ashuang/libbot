add_definitions(-std=gnu99)
# Create a shared library libbot2-frames-renderer.so with a single source file
add_library(bot2-frames-renderer SHARED
    coord_frames_renderer.c)
    
pods_use_pkg_config_packages(bot2-frames-renderer bot2-core bot2-param-client bot2-vis bot2-frames)

# set the library API version.  Increment this every time the public API
# changes.
set_target_properties(bot2-frames-renderer PROPERTIES SOVERSION 1)

# make the header public
# install it to include/bot_frames
pods_install_headers(coord_frames_renderer.h DESTINATION bot_frames)

# make the library public
pods_install_libraries(bot2-frames-renderer)

# create a pkg-config file for the library, to make it for other software to
# use it.
pods_install_pkg_config_file(bot2-frames-renderer
    CFLAGS
    LIBS -lbot2-frames-renderer
    REQUIRES bot2-core bot2-param-client bot2-frames bot2-vis
    VERSION 0.0.1)

    
    
    
    
#build the test-viewer
add_executable(test-viewer test_viewer.c)
pods_use_pkg_config_packages(test-viewer bot2-vis bot2-lcmgl-render bot2-frames-renderer)