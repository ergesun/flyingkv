aux_source_directory(.                 SOURCE_FILES)
aux_source_directory(./rpc             ND_RPC_SRCS)

add_executable(minikvd ${SOURCE_FILES} ${ND_SRCS} ${ND_RPC_SRCS})

set(LINK_LIBS
        kv
        codegen
        protobuf
        rpc
        net
        utils
        common
        sys
        glog
        gflags
        pthread
        )

set(DEPEN_LIBS
        codegen
        net
        common
        )

add_dependencies(minikvd ${DEPEN_LIBS})
target_link_libraries(minikvd ${LINK_LIBS})
