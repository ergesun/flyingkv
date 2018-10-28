aux_source_directory(.                 SOURCE_FILES)
aux_source_directory(./rpc             ND_RPC_SRCS)

add_executable(minikvd ${SOURCE_FILES} ${ND_SRCS} ${ND_RPC_SRCS})

set(LINK_LIBS
        kv
        codegen
        checkpoint
        wal
        protobuf
        rpc
        net
        utils
        common
        acc
        cjson
        sys
        glog
        gflags
        pthread
        )

set(DEPEN_LIBS
        kv
        codegen
        checkpoint
        wal
        net
        common
        rpc
        )

add_dependencies(minikvd ${DEPEN_LIBS})
target_link_libraries(minikvd ${LINK_LIBS})
