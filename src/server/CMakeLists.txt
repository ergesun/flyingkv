aux_source_directory(.                 SOURCE_FILES)
aux_source_directory(./node            ND_SRCS)
aux_source_directory(./rpc             ND_RPC_SRCS)
aux_source_directory(./rflog           RFLOG_SRCS)

add_executable(minikvd ${SOURCE_FILES} ${ND_SRCS} ${ND_RPC_SRCS} ${RFLOG_SRCS})

set(LINK_LIBS
        protobufgen
        protobuf
        rpc
        net
        fsio
        utils
        common
        sys
        glog
        gflags
        pthread
        )

set(DEPEN_LIBS
        protobufgen
        net
        common
        )

add_dependencies(minikvd ${DEPEN_LIBS})
target_link_libraries(minikvd ${LINK_LIBS})
