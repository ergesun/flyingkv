aux_source_directory(. SRCS)
aux_source_directory(./client CLIENT_SRCS)
aux_source_directory(./server SERVER_SRCS)

add_executable(rpc_test ${SRCS} ${CLIENT_SRCS} ${SERVER_SRCS})

set(LINK_LIBS
        librpc.a
        libnet.a
        libutils.a
        libcommon.a
        libcodegen.a
        libprotobuf.a
        libsys.a
        libglog.a
        libgflags.a
        libgtest.a
        pthread
        )

set(DEPEN_LIBS
        rpc
        net
        utils
        common
        codegen
        sys
        )

add_dependencies(rpc_test ${DEPEN_LIBS})
target_link_libraries(rpc_test ${LINK_LIBS})
