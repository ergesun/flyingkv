aux_source_directory(. SRCS)
aux_source_directory(./entries ENTRIES_SRCS)
aux_source_directory(./mini-kv MINIKV_SRCS)
add_library(kv STATIC ${SRCS} ${WAL_SRCS} ${ENTRIES_SRCS} ${MINIKV_SRCS} mini-kv/errors.h entries/common.h entries/wal-delete-entry.h entries/wal-delete-entry.cc entries/raw-pb-entry-entry.h entries/raw-pb-entry-entry.cc mini-kv/mini-kv-traveller.h mini-kv/mini-kv-traveller.cc)
