#!/bin/bash

# Install CMake for project builds
# Example usage: ./install_cmake.sh

export WORKDIR="${HOME}/scripts"
export NAME="[install_cmake.sh]: "
echo "${NAME} STARTING "

# Bash failure reporting for the script
set -eE -o functrace
failure() {
  local lineno=$1
  local msg=$2
  echo "${NAME} Failed at $lineno: $msg"
}
trap '${NAME} failure ${LINENO} "$BASH_COMMAND"' ERR

apt-get purge --auto-remove -yqq cmake &&
rm -rf /root/.local/bin/cmake || echo "CMake doesnt exist"

VERSION="3.25.1"
SYSTEM_ARCHITECTURE="$(uname --machine)"
# https://github.com/Kitware/CMake/releases"
cd /tmp && \
    wget -qO- "https://github.com/Kitware/CMake/releases/download/v${VERSION}/cmake-${VERSION}-linux-${SYSTEM_ARCHITECTURE}.tar.gz" | \
    tar --strip-components=1 -xz -C /usr/local

echo "--(installed)-- cmake version $(cmake --version)";

#if [[ "$(which cmake)" == "/usr/local/bin/cmake" ]]; then
#  echo 'good';
#else
#  ln -s  /usr/local/bin/cmake /root/.local/bin/cmake
#fi;

echo "${NAME} FINISHED "
