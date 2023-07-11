#!/bin/bash

# Install cpp-modern-kafka and its deps
# Example usage:  ./install_messaging.sh

export NAME="[install_messaging.sh]: "

set -eE -o functrace
failure() {
  local lineno=$1
  local msg=$2
  echo "${NAME} Failed at $lineno: $msg"
}
trap '${NAME} failure ${LINENO} "$BASH_COMMAND"' ERR

## Script start
echo "${NAME} STARTING "
export WORKDIR="/start"
echo "${NAME} Install 'librdkafka' with vcpkg "
cd "${WORKDIR}/vcpkg"; ./vcpkg install librdkafka;
export LIBRDKAFKA_ROOT="${WORKDIR}/vcpkg/installed/x64-linux"

echo "${NAME} Install 'modern-cpp-kafka' apt dependencies "
export KAFKA_VERSION_TAG=2022.08.01
export REPO_NAME=modern-cpp-kafka

echo "${NAME} Clone 'modern-cpp-kafka' repo "
cd ${WORKDIR} && \
 curl -OL "https://github.com/morganstanley/${REPO_NAME}/archive/refs/tags/v${KAFKA_VERSION_TAG}.zip" && \
 unzip "v${KAFKA_VERSION_TAG}.zip" && rm -rf "v${KAFKA_VERSION_TAG}.zip";

cd /start; mv "${REPO_NAME}-${KAFKA_VERSION_TAG}" "${REPO_NAME}"
echo "/start/${REPO_NAME}/include"
export MODERN_CPP_INCLUDE_DIR="/start/${REPO_NAME}/include"
echo "*** CMake project integration ***"
echo "LIBRDKAFKA_ROOT: ${LIBRDKAFKA_ROOT}"
echo "MODERN_CPP_INCLUDE_DIR: ${MODERN_CPP_INCLUDE_DIR}"

echo "${NAME} FINISHED "
