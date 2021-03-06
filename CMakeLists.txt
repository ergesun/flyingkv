# cmake 3.2 start to support cpp11
cmake_minimum_required(VERSION 3.2)
project(flyingkv)

# debug/release编译控制
SET (CMAKE_C_FLAGS                  "-Wall -std=c99")
SET (CMAKE_C_FLAGS_DEBUG            "-O0 -g")
SET (CMAKE_C_FLAGS_MINSIZEREL       "-Os -DNDEBUG")
SET (CMAKE_C_FLAGS_RELEASE          "-O2 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO   "-O2 -g")
SET (CMAKE_CXX_FLAGS                "-Wall")
SET (CMAKE_CXX_FLAGS_DEBUG          "-O0 -g")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O2 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

# 自定义宏
option(DEFINE_MACRO "Option for [debug logs] macro" OFF)
if(DEFINE_MACRO)
    add_definitions(-DDEBUG_LOG)
endif(DEFINE_MACRO)

add_definitions(-DPUBLIC=public:)
if(${PROJECT_TYPE} MATCHES "UT")
    add_definitions(-DPRIVATE=public: -DPROTECTED=public:)
else()
    add_definitions(-DPRIVATE=private: -DPROTECTED=protected:)
endif()

# 使用c++11标准
set(CMAKE_CXX_STANDARD 11)

# 添加gcc flags
## -Wno-unused-result：忽略未接返回值的warning
## -Wno-unused-function：忽略定义的函数未使用的警告
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-result")

# 显示make详细信息
#set(CMAKE_VERBOSE_MAKEFILE ON)

set(SRC_ROOT ${PROJECT_SOURCE_DIR}/src)
set(UT_ROOT ${SRC_ROOT}/unit-test)
#set(TL_ROOT ${PROJECT_SOURCE_DIR}/tools)
set(EXTERN_COMPILE_THIRDPARTY_ROOT ${PROJECT_SOURCE_DIR}/third_party)

# 第三方库
include_directories(${EXTERN_COMPILE_THIRDPARTY_ROOT}/include)
message(STATUS "EXTERN_COMPILE_THIRDPARTY_ROOT = ${EXTERN_COMPILE_THIRDPARTY_ROOT}")
link_directories(${EXTERN_COMPILE_THIRDPARTY_ROOT}/lib)

##---------------------------------for clion debug ut-------------------------------------------
#add_definitions(-DPRIVATE=public: -DPROTECTED=public:)
#add_definitions(-DUNIT_TEST -DGTEST_LANG_CXX11=1 -DGTEST_HAS_TR1_TUPLE=0 -DGTEST_USE_OWN_TR1_TUPLE=0)
#
#set(BUILD_OUTPUT_ROOT_DIR ${PROJECT_SOURCE_DIR}/build/ut/debug)
#set(LIBRARY_OUTPUT_PATH ${BUILD_OUTPUT_ROOT_DIR}/lib)
#set(EXECUTABLE_OUTPUT_PATH ${BUILD_OUTPUT_ROOT_DIR})
#
#link_directories(${BUILD_OUTPUT_ROOT_DIR}/lib)
#
#add_subdirectory(${SRC_ROOT}/3rd)
#add_subdirectory(${SRC_ROOT}/net)
#add_subdirectory(${SRC_ROOT}/rpc)
#add_subdirectory(${SRC_ROOT}/common)
#add_subdirectory(${SRC_ROOT}/acc)
#add_subdirectory(${SRC_ROOT}/utils)
#add_subdirectory(${SRC_ROOT}/codegen)
#add_subdirectory(${SRC_ROOT}/sys)
#add_subdirectory(${SRC_ROOT}/wal)
#add_subdirectory(${SRC_ROOT}/checkpoint)
#add_subdirectory(${SRC_ROOT}/kv)
#
#add_subdirectory(${UT_ROOT}/lc-wal)
##----------------------------------------------------------------------------------------------

if(${PROJECT_TYPE} MATCHES "UT")
    message(STATUS "UT_MODULE=${UT_MODULE}")
    add_definitions(-DGTEST_LANG_CXX11=1 -DGTEST_HAS_TR1_TUPLE=0 -DGTEST_USE_OWN_TR1_TUPLE=0)

    message(STATUS "UT_ROOT/UT_MODULE = ${UT_ROOT}/${UT_MODULE}")
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(BUILD_OUTPUT_ROOT_DIR ${PROJECT_SOURCE_DIR}/build/ut/release)
    else()
        set(BUILD_OUTPUT_ROOT_DIR ${PROJECT_SOURCE_DIR}/build/ut/debug)
    endif()
    set(LIBRARY_OUTPUT_PATH ${BUILD_OUTPUT_ROOT_DIR}/lib)
    set(EXECUTABLE_OUTPUT_PATH ${BUILD_OUTPUT_ROOT_DIR})
    link_directories(${BUILD_OUTPUT_ROOT_DIR}/lib)

    add_subdirectory(${SRC_ROOT}/3rd)
    add_subdirectory(${SRC_ROOT}/net)
    add_subdirectory(${SRC_ROOT}/rpc)
    add_subdirectory(${SRC_ROOT}/common)
    add_subdirectory(${SRC_ROOT}/acc)
    add_subdirectory(${SRC_ROOT}/utils)
    add_subdirectory(${SRC_ROOT}/codegen)
    add_subdirectory(${SRC_ROOT}/sys)
    add_subdirectory(${SRC_ROOT}/wal)
    add_subdirectory(${SRC_ROOT}/checkpoint)
    add_subdirectory(${SRC_ROOT}/kv)
    add_subdirectory(${UT_ROOT}/${UT_MODULE})
elseif(${PROJECT_TYPE} MATCHES "EXAMPLES")
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(BUILD_OUTPUT_ROOT_DIR ${PROJECT_SOURCE_DIR}/build/flyingkv/release)
    else()
        set(BUILD_OUTPUT_ROOT_DIR ${PROJECT_SOURCE_DIR}/build/flyingkv/debug)
    endif()
    set(LIBRARY_OUTPUT_PATH ${BUILD_OUTPUT_ROOT_DIR}/lib)
    set(EXECUTABLE_OUTPUT_PATH ${BUILD_OUTPUT_ROOT_DIR})
    link_directories(${BUILD_OUTPUT_ROOT_DIR}/lib)

    add_subdirectory(${SRC_ROOT}/net)
    add_subdirectory(${SRC_ROOT}/rpc)
    add_subdirectory(${SRC_ROOT}/common)
    add_subdirectory(${SRC_ROOT}/utils)
    add_subdirectory(${SRC_ROOT}/codegen)
    add_subdirectory(${SRC_ROOT}/sys)
    add_subdirectory(${SRC_ROOT}/client)
    add_subdirectory(${SRC_ROOT}/examples)
else()
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        set(BUILD_OUTPUT_ROOT_DIR ${PROJECT_SOURCE_DIR}/build/flyingkv/release)
    else()
        set(BUILD_OUTPUT_ROOT_DIR ${PROJECT_SOURCE_DIR}/build/flyingkv/debug)
    endif()
    set(LIBRARY_OUTPUT_PATH ${BUILD_OUTPUT_ROOT_DIR}/lib)
    set(EXECUTABLE_OUTPUT_PATH ${BUILD_OUTPUT_ROOT_DIR})
    link_directories(${BUILD_OUTPUT_ROOT_DIR}/lib)
    add_subdirectory(${SRC_ROOT})
endif()
