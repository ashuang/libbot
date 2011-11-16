find_package(PythonInterp REQUIRED)

file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/bot_procman/build_prefix.py
    "BUILD_PREFIX='${CMAKE_INSTALL_PREFIX}'\n")

# install all python packages in the src/ directory
pods_install_python_packages(${CMAKE_CURRENT_SOURCE_DIR}/src)

# executable scripts:  script-name  python-module
pods_install_python_script(bot-procman bot_procman.sheriff_gtk.sheriff_gtk)
