aux_source_directory(. SRCS)

add_executable(utils_test ${SRCS})
target_link_libraries(utils_test utils common codegen protobuf gtest pthread)
