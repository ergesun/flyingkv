aux_source_directory(. SRCS)
aux_source_directory(./simple-wal SW_SRCS)
add_library(wal SHARED ${SRCS} ${SW_SRCS} simple-wal/simple-wal.h simple-wal/simple-wal.cpp)
