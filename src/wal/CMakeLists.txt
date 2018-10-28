aux_source_directory(. SRCS)
aux_source_directory(./wals SW_SRCS)
add_library(wal SHARED ${SRCS} ${SW_SRCS})
