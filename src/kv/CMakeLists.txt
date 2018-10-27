aux_source_directory(. SRCS)
aux_source_directory(../wal/wal WAL_SRCS)
aux_source_directory(./mini-kv MINIKV_SRCS)
add_library(kv SHARED ${SRCS} ${WAL_SRCS} ${MINIKV_SRCS} ../wal/ientry.h mini-kv/ientry.cpp mini-kv/entry.h ../wal/iwal.h)
