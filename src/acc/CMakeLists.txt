aux_source_directory(. SRCS)
aux_source_directory(./resource-limit-control RLC_SRCS)
aux_source_directory(./limiters LIMITER_SRCS)
add_library(acc SHARED ${SRCS} ${RLC_SRCS} ${LIMITER_SRCS} ./limiters/limiter.h ./resource-limit-control/rlc.h conf-parser.h limiters/common-def.h)
