aux_source_directory(. SRCS)
add_library(common STATIC ${SRCS} common-def.h)
