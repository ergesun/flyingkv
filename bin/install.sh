#!/bin/bash
BIN_DIR=$(cd `dirname $0`; pwd)

. ${BIN_DIR}/utils/color-utils.sh

ROOT_DIR=${BIN_DIR}/..
. ${ROOT_DIR}/conf/minikv-common-def.sh

echo_yellow "installing minikv..."

if [ ! -d "${ROOT_DIR}/bin" -o ! -d "${ROOT_DIR}/conf" ]; then
  echo_red "ERROR: you should build、generate supervisor conf and pack dist first."
  exit 1
fi

mkdir -p ${INST_ROOT_DIR}

cp -r ${ROOT_DIR}/*      ${INST_ROOT_DIR}

if [ $? -eq 0 ]; then
    echo_green "install minikv successfully!"
else
    echo_red "install minikv failed!"
fi
