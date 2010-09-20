include_directories(${GLIB2_INCLUDE_DIRS} 
    ${LCM_INCLUE_DIRS}
    ${BOT2_CORE_INCLUDE_DIRS})

set(ldpc_sources 
    ldpc/getopt.cpp
    ldpc/ldpc_create_pchk.cpp
    ldpc/ldpc_fec.cpp
    ldpc/ldpc_fec_iterative_decoding.cpp
    ldpc/ldpc_matrix_sparse.cpp
    ldpc/ldpc_rand.cpp
    ldpc/ldpc_scheme.cpp
    ldpc/ldpc_wrapper.cpp
    ldpc/tools.cpp
    )

add_executable(bot-lcm-tunnel
    introspect.c
    lcm_tunnel_params_t.c
    ssocket.c
    lcm_tunnel.cpp
    lcm_tunnel_server.cpp
    ${ldpc_sources}
    )

add_executable(ldpc-wrapper-test
    ldpc/ldpc_wrapper_test.cpp
    ${ldpc_sources}
    )

set_source_files_properties(introspect.c lcm_tunnel_params_t.c ssocket.c 
    PROPERTIES COMPILE_FLAGS "-std=gnu99")

target_link_libraries(bot-lcm-tunnel
    ${GLIB2_LDFLAGS}
    ${LCM_LDFLAGS}
    ${BOT2_CORE_LDFLAGS})

install(TARGETS bot-lcm-tunnel RUNTIME DESTINATION bin)
