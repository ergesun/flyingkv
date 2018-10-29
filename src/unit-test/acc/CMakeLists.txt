aux_source_directory(. SRCS)

add_executable(acc_test ${SRCS} ../wal/main.cc)
target_link_libraries(acc_test sys cjson common codegen protobuf gtest pthread)
