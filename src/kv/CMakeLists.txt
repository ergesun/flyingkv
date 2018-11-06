aux_source_directory(. SRCS)
aux_source_directory(./mini-kv MINIKV_SRCS)
add_library(kv STATIC ${SRCS} ${WAL_SRCS} ${MINIKV_SRCS})
