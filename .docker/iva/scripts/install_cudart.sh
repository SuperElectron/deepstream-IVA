#!/bin/bash

# Example usage: ./install_libcudart.sh

export WORKDIR="/start/scripts"
export NAME="[install_libcudart.sh]: "
echo "${NAME} STARTING "

# Bash failure reporting for the script
set -eE -o functrace
failure() {
  local lineno=$1
  local msg=$2
  echo "${NAME} Failed at $lineno: $msg"
}
trap '${NAME} failure ${LINENO} "$BASH_COMMAND"' ERR

MACHINE="$(uname -m)"

if [[ "$MACHINE" == "x86_64" ]]; then
  echo "--(INFO)-- Skipping downloads because on X86 architecture"
else
  echo "--(INFO)-- Installing (arm64 architecture detected)"
  # Download and install libcudart.so: https://repo.download.nvidia.com/jetson/
  # https://repo.download.nvidia.com/jetson/common/pool/main/n/nvidia-nsight-sys/nvidia-nsight-sys_5.1-b147_arm64.deb
  cd /tmp &&
    wget https://repo.download.nvidia.com/jetson/common/pool/main/c/cuda-cudart/cuda-cudart-11-4_11.4.243-1_arm64.deb &&
    apt-get install -y ./cuda-cudart-11-4_11.4.243-1_arm64.deb &&
    apt-get update -yqq
  cd /tmp && rm cuda-cudart-11-4_11.4.243-1_arm64.deb
  cp /usr/local/cuda/targets/aarch64-linux/lib/libcudart.so /opt/nvidia/deepstream/deepstream/lib/
fi

apt-get update -yqq
# still need to run this: apt-get install -y cuda
echo "${NAME} FINISHED "
