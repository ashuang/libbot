# Pods macros

# setup include, linker, and pkg-config paths
macro(pods_config_search_paths)
    if(NOT DEFINED __pods_setup)
        # add build/lib/pkgconfig to the pkg-config search path
        set(ENV{PKG_CONFIG_PATH} ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)

        # add build/include to the compiler include path
        include_directories(${CMAKE_INSTALL_PREFIX}/include)

        # add build/lib to the link path
        link_directories(${CMAKE_INSTALL_PREFIX}/lib)

        # abuse RPATH
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH true)

        set(__pods_setup true)
    endif(NOT DEFINED __pods_setup)
endmacro(pods_config_search_paths)


# TODO document this
function(pods_create_pkg_config_file)
    list(GET ARGV 0 pc_name)
    # TODO error check

    set(pc_version 0.0.1)
    set(pc_description ${pc_name})
    set(pc_requires "")
    set(pc_libs "")
    set(pc_cflags "")
    set(pc_fname "${CMAKE_CURRENT_BINARY_DIR}/${pc_name}.pc")

    set(modewords LIBS CFLAGS REQUIRES VERSION DESCRIPTION)
    set(curmode "")

    # parse function arguments and populate pkg-config parameters
    list(REMOVE_AT ARGV 0)
    foreach(word ${ARGV})
        list(FIND modewords ${word} mode_index)
        if(${mode_index} GREATER -1)
            set(curmode ${word})
        elseif(curmode STREQUAL LIBS)
            set(pc_libs "${pc_libs} ${word}")
        elseif(curmode STREQUAL CFLAGS)
            set(pc_cflags "${pc_cflags} ${word}")
        elseif(curmode STREQUAL REQUIRES)
            set(pc_requires "${pc_requires} ${word}")
        elseif(curmode STREQUAL VERSION)
            set(pc_version ${word})
            set(curmode "")
        elseif(curmode STREQUAL DESCRIPTION)
            set(pc_description "${word}")
            set(curmode "")
        else(${mode_index} GREATER -1)
            message("WARNING incorrect use of pods_add_pkg_config (${word})")
            break()
        endif(${mode_index} GREATER -1)
    endforeach(word)

    # write the .pc file out
    file(WRITE ${pc_fname}
        "prefix=${CMAKE_INSTALL_PREFIX}\n"
        "exec_prefix=\${prefix}\n"
        "libdir=\${exec_prefix}/lib\n"
        "includedir=\${prefix}/include\n"
        "\n"
        "Name: ${pc_name}\n"
        "Description: ${pc_description}\n"
        "Requires: ${pc_requires}\n"
        "Version: ${pc_version}\n"
        "Libs: -L\${exec_prefix}/lib ${pc_libs}\n"
        "Cflags: ${pc_cflags}\n")

    # mark the .pc file for installation to the lib/pkgconfig directory
    install(FILES ${pc_fname} DESTINATION lib/pkgconfig)
endfunction(pods_create_pkg_config_file)


# TODO document this
function(pods_install_python_script script_name py_module)
    find_package(PythonInterp REQUIRED)

    # which python version?
    execute_process(COMMAND 
        ${PYTHON_EXECUTABLE} -c "import sys; sys.stdout.write(sys.version[:3])"
        OUTPUT_VARIABLE pyversion)

    # where do we install .py files to?
    set(python_install_dir 
        ${CMAKE_INSTALL_PREFIX}/lib/python${pyversion}/site-packages)

    # write the script file
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${script_name} "#!/bin/sh\n"
        "export PYTHONPATH=${python_install_dir}:\${PYTHONPATH}\n"
        "exec python -m ${py_module}\n")

    # install it...
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${script_name} DESTINATION bin)
endfunction()

function(pods_install_python_packages py_src_dir)
    find_package(PythonInterp REQUIRED)

    # which python version?
    execute_process(COMMAND 
        ${PYTHON_EXECUTABLE} -c "import sys; sys.stdout.write(sys.version[:3])"
        OUTPUT_VARIABLE pyversion)

    # where do we install .py files to?
    set(python_install_dir 
        ${CMAKE_INSTALL_PREFIX}/lib/python${pyversion}/site-packages)

    if(ARGC GREATER 1)
        message(FATAL_ERROR "NYI")
    else()
        # get a list of all .py files
        file(GLOB_RECURSE py_files RELATIVE ${py_src_dir} ${py_src_dir}/*.py)

        # add rules for byte-compiling .py --> .pyc
        foreach(py_file ${py_files})
            get_filename_component(py_dirname ${py_file} PATH)
            set(py_absname "${py_src_dir}/${py_file}")
            add_custom_command(OUTPUT "${py_absname}c" 
                COMMAND ${PYTHON_EXECUTABLE} -m compileall ${py_absname} 
                DEPENDS ${py_absname})
            list(APPEND pyc_files "${py_absname}c")

            # install python file and byte-compiled file
            install(FILES ${py_absname} "${py_absname}c" 
                DESTINATION "${python_install_dir}/${py_dirname}")
        endforeach()
        string(REGEX REPLACE "[^a-zA-Z0-9]" "_" san_src_dir "${py_src_dir}")
        add_custom_target("pyc_${san_src_dir}" ALL DEPENDS ${pyc_files})
    endif()
endfunction()

