aux_source_directory(. SRCS)
aux_source_directory(./checkpoints SC_SRCS)
add_library(checkpoint SHARED ${SRCS} ${SC_SRCS} icheckpoint.h ientries-traveller.h)