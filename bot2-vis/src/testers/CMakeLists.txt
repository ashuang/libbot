add_definitions(-std=gnu99)

include_directories(
    ${OPENGL_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIR}
    )

set(testers 
    gl_drawing_area_tester
    gl_image_area_tester
    param_widget_tester)

foreach(tester ${testers})
    add_executable(${tester} "${tester}.c")
    pods_use_pkg_config_packages(${tester} bot2-vis)
endforeach()
