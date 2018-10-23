aux_source_directory(. SRCS)

add_executable(sys_test ${SRCS})
target_link_libraries(sys_test sys gtest pthread)
