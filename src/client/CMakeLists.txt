aux_source_directory(. SRCS)
aux_source_directory(./rpc RPC_SRCS)
add_library(flyingkvc STATIC ${SRCS} ${RPC_SRCS} flyingkv-client.h flyingkv-client.cc config.h)
