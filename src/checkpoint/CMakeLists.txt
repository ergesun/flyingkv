aux_source_directory(. SRCS)
aux_source_directory(./simple-checkpiont SC_SRCS)
add_library(checkpoint SHARED ${SRCS} ${SC_SRCS} icheckpoint.h checkpoint-factory.h checkpoint-factory.cpp)