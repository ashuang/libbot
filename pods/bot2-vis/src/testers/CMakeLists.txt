add_definitions(-std=gnu99)

include_directories(${GLIB2_INCLUDE_DIRS} 
    ${GTK2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIR}
    ${LCM_INCLUDE_DIRS}
    ${BOT2_CORE_INCLUDE_DIRS}
    )

set(link_libraries 
    ${GLIB2_LDFLAGS}
    ${GTK2_LDLFLAGS}
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${LCM_LDFLAGS}
    ${BOT2_CORE_LDFLAGS}
    bot2-vis
    )

set(testers 
    gl_drawing_area_tester
    gl_image_area_tester
    param_widget_tester)

foreach(tester ${testers})
    add_executable(${tester} "${tester}.c")
    target_link_libraries(${tester} ${link_libraries})
endforeach()
