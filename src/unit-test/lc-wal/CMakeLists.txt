aux_source_directory(. SRCS)
aux_source_directory(./entry ENTRY_SRCS)
aux_source_directory(./entry-handler ENTRY_HANDLER_SRCS)

add_executable(wal_test ${SRCS} ${ENTRY_SRCS} ${ENTRY_HANDLER_SRCS})
target_link_libraries(wal_test wal common utils codegen sys glog gflags protobuf gtest pthread)
