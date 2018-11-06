aux_source_directory(. SRCS)
aux_source_directory(./checkpoints SC_SRCS)
add_library(checkpoint STATIC ${SRCS} ${SC_SRCS})