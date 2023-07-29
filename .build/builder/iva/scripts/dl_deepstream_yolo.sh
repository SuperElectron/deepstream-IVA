#!/bin/bash

### download Deepstream Yolo
# Note: must set CUDA_VER before running script
# Example usage:          export CUDA_VER=11.6;
#                         ./dl_deepstream_yolo.sh
export NAME="[dl_deepstream_yolo.sh] "
export WORKDIR="${HOME}"

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
    CUDA_VER=11.6
  fi;
fi

cd "${WORKDIR}" && \
  wget https://github.com/marcoslucianops/DeepStream-Yolo/archive/refs/heads/master.zip -O "${WORKDIR}/DeepStream-Yolo.zip"

cd "${WORKDIR}" && unzip DeepStream-Yolo.zip && rm -rf DeepStream-Yolo.zip && mv "${WORKDIR}/DeepStream-Yolo-master" "${WORKDIR}/DeepStream-Yolo"
cd "${WORKDIR}/DeepStream-Yolo" && CUDA_VER=$CUDA_VER make -C nvdsinfer_custom_impl_Yolo
mv "${WORKDIR}/DeepStream-Yolo/nvdsinfer_custom_impl_Yolo/libnvdsinfer_custom_impl_Yolo.so" /usr/local/lib/libnvdsinfer_custom_impl_Yolo.so
rm -rf "${WORKDIR}/DeepStream-Yolo"
chmod 777 /usr/local/lib/libnvdsinfer_custom_impl_Yolo.so
export YOLO_SO_LIB=/usr/local/lib/libnvdsinfer_custom_impl_Yolo.so
echo "--(note)-- Deepstream Yolo nvinfer parsing lib location: ${YOLO_SO_LIB} "
echo "${NAME} FINISHED "
