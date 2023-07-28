#!/bin/bash

### download Deepstream Yolo
# Note: must set CUDA_VER before running script
# Example usage:          export CUDA_VER=11.6;
#                         ./dl_deepstream_yolo.sh
export NAME="[dl_deepstream_yolo.sh] "

set -eE -o functrace
failure() {
  local lineno=$1
  local msg=$2
  echo "${NAME} Failed at $lineno: $msg"
}
trap '${NAME} failure ${LINENO} "$BASH_COMMAND"' ERR

## Script start
echo "${NAME} STARTING "

if [ -z "$CUDA_VER" ]; then
  if [[ "$(uname -m)" == "aarch64" ]]; then
    CUDA_VER=11.4
  else
    CUDA_VER=11.7
    CUDA_VER=11.6
  fi;
fi

mkdir -p /start && cd /start && \
  wget https://github.com/marcoslucianops/DeepStream-Yolo/archive/refs/heads/master.zip -O /start/DeepStream-Yolo.zip

cd /start && unzip DeepStream-Yolo.zip && rm -rf DeepStream-Yolo.zip && mv /start/DeepStream-Yolo-master /start/DeepStream-Yolo
cd /start/DeepStream-Yolo && CUDA_VER=$CUDA_VER make -C nvdsinfer_custom_impl_Yolo
mv /start/DeepStream-Yolo/nvdsinfer_custom_impl_Yolo/libnvdsinfer_custom_impl_Yolo.so /usr/local/lib/libnvdsinfer_custom_impl_Yolo.so
rm -rf /start/DeepStream-Yolo
export YOLO_SO_LIB=/usr/local/lib/libnvdsinfer_custom_impl_Yolo.so
echo "--(note)-- Deepstream Yolo nvinfer parsing lib location: ${YOLO_SO_LIB} "
echo "${NAME} FINISHED "
