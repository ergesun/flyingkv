#!/bin/bash

BIN_DIR=$(cd `dirname $0`; pwd)
ROOT_DIR=${BIN_DIR}/..

BUILD_OBJS_FOLDER="cmake-objs"
BUILD_OBJS_DIR=${BIN_DIR}/../${BUILD_OBJS_FOLDER}

# load color utils
. ${BIN_DIR}/utils/color-utils.sh

function print_usage {
    echo_red "Usage: clean COMMAND."
    echo_yellow "where COMMAND is one of:"
    echo_yellow "  -gen  \t clean generated files."
    echo_yellow "  -all  \t clean all generated targets(reset to just git clone state)."
    echo_yellow "  -h    \t show help."
}

for p in "$@"
do
    if [ "$p" = "-gen" ]; then
        echo "cleaning codegen..."
        rm -rf ${ROOT_DIR}/src/codegen
    elif [ "$p" = "-all" ]; then
        echo "cleaning codegen..."
        rm -rf ${ROOT_DIR}/src/codegen
    elif [ "$p" == "-h" ]; then
        print_usage
        exit 0
    else
        echo_red "Not support opt \"$p\""
        print_usage
        exit 1
    fi
done

echo "cleaning cmake objs..."
rm -rf ${BUILD_OBJS_DIR}

echo "cleaning build..."
rm -rf ${ROOT_DIR}/build

echo "cleaning libs..."
rm -rf ${BIN_DIR}/../lib

echo "clean over."
