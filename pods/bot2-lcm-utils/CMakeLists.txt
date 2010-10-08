cmake_minimum_required(VERSION 2.6.0)

project(bot2-lcm-utils)

include(cmake/pods.cmake)

add_subdirectory(src/logfilter)
add_subdirectory(src/logsplice)
add_subdirectory(src/who)
add_subdirectory(src/tunnel)
add_subdirectory(java)


# bot-spy
set(script_name bot-spy)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${script_name} 
    "#!/bin/sh\n"
    "CLASSPATH=`pkg-config --variable=classpath lcm-java`\n"
    "for d in ${CMAKE_INSTALL_PREFIX}/share/java $*; do\n"
    "  echo Checking $d\n"
    "  for f in $d/lcmtypes_*.jar $d/lcmspy_plugins_*.jar; do\n"
    "    if [ -e $f ]; then\n"
    "      echo \"   Found $f\"\n"
    "      CLASSPATH=\$CLASSPATH:\$f\n"
    "    fi\n"
    "  done\n"
    "done\n"
    "exec java -ea -cp \$CLASSPATH lcm.spy.Spy $*\n")

# install it...
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${script_name} DESTINATION bin)
