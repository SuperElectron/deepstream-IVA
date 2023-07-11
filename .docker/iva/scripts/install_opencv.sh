#!/bin/bash

# Install CMake for project builds
# Example usage: ./install_opencv.sh

export WORKDIR="/start/scripts"
export NAME="[install_opencv.sh]: "
echo "${NAME} STARTING "

# Bash failure reporting for the script
set -eE -o functrace
failure() {
  local lineno=$1
  local msg=$2
  echo "${NAME} Failed at $lineno: $msg"
}
trap '${NAME} failure ${LINENO} "$BASH_COMMAND"' ERR

if [ -z "$CUDA_ARCH_BIN" ]; then
  echo "--(WARNING)-- Could not find ENV var (CUDA_ARCH_BIN='${CUDA_ARCH_BIN}').
    Visit https://developer.nvidia.com/cuda-gpus and include it the ENV before running this script"
  exit 1;
else
  echo "--(INFO)-- Detected (CUDA_ARCH_BIN='${CUDA_ARCH_BIN}')."
fi

if [ -z "$ENABLE_OPENCV_DNN_CUDA" ]; then
  echo "--(INFO)-- Could not find ENV var (ENABLE_OPENCV_DNN_CUDA='${ENABLE_OPENCV_DNN_CUDA}').
    Set: ENABLE_OPENCV_DNN_CUDA=OFF"
    ENABLE_OPENCV_DNN_CUDA=OFF
else
  echo "--(INFO)-- Detected (ENABLE_OPENCV_DNN_CUDA='${ENABLE_OPENCV_DNN_CUDA}').
    Set: ENABLE_OPENCV_DNN_CUDA=ON"
  ENABLE_OPENCV_DNN_CUDA=ON
fi


# clone opencv latest versions
cd  ~; git clone https://github.com/opencv/opencv.git
cd  ~; git clone https://github.com/opencv/opencv_contrib.git
# build and install opencv (check https://developer.nvidia.com/cuda-gpus and match gpu (try $ nvidia-smi) to its  CUDA_ARCH_BIN)
cd ~/opencv && mkdir build
cd ~/opencv/build && cmake \
  -D CMAKE_BUILD_TYPE=RELEASE \
  -D CMAKE_INSTALL_PREFIX=/usr/local \
  -D WITH_CUDA=ON \
  -D WITH_CUDNN=ON \
  -D WITH_CUBLAS=ON \
  -D WITH_TBB=ON \
  -D OPENCV_DNN_CUDA="${ENABLE_OPENCV_DNN_CUDA}" \
  -D OPENCV_ENABLE_NONFREE=ON \
  -D CUDA_ARCH_BIN="${ARCH_BIN}" \
  -D CUDA_ARCH_PTX="" \
  -D BUILD_TESTS=OFF \
  -D OPENCV_EXTRA_MODULES_PATH="~/opencv_contrib/modules" \
  -D BUILD_EXAMPLES=OFF \
  -D ENABLE_COVERAGE=OFF \
  -D WITH_MSMF=OFF \
  -D WITH_WIN32UI=OFF \
  -D INSTALL_C_EXAMPLES=OFF \
  -D INSTALL_PYTHON_EXAMPLES=OFF \
  -D HAVE_opencv_python3=OFF \
  ..

cd ~/opencv/build && make -j -l4
cd ~/opencv/build && make install

## creates the necessary links and cache (/etc/ld)
ldconfig

echo "${NAME} FINISHED "
