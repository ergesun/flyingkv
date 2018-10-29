aux_source_directory(.                 SOURCE_FILES)
aux_source_directory(./rpc             ND_RPC_SRCS)

add_executable(flyingkvd ${SOURCE_FILES} ${ND_RPC_SRCS})

set(LINK_LIBS
    kv.a
    wal.a
    checkpoint.a
    acc.a
    rpc.a
    net.a
    utils.a
    common.a
    codegen.a
    protobuf.a
    cjson.a
    sys.a
    glog.a
    gflags.a
    pthread
    )

set(DEPEN_LIBS
    kv
    wal
    checkpoint
    acc
    rpc
    net
    utils
    common
    sys
    codegen
    cjson
    )

add_dependencies(flyingkvd ${DEPEN_LIBS})
target_link_libraries(flyingkvd ${LINK_LIBS})
