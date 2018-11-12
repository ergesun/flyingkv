aux_source_directory(. SOURCE_FILES)

add_executable(flyingkveg ${SOURCE_FILES})

set(LINK_LIBS
        flyingkvc.a
        rpc.a
        net.a
        utils.a
        common.a
        codegen.a
        protobuf.a
        sys.a
        glog.a
        gflags.a
        pthread
        )

set(DEPEN_LIBS
        rpc
        net
        utils
        common
        sys
        codegen
        )

add_dependencies(flyingkveg ${DEPEN_LIBS})
target_link_libraries(flyingkveg ${LINK_LIBS})
