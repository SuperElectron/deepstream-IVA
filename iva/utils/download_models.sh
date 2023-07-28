#!/bin/bash

# Example usage: ./download_models.sh

export NAME="[download_models.sh]: "
echo "${NAME} STARTING "

# Bash failure reporting for the script
set -eE -o functrace
failure() {
  local lineno=$1
  local msg=$2
  echo "${NAME} Failed at $lineno: $msg"
}
trap '${NAME} failure ${LINENO} "$BASH_COMMAND"' ERR

echo "${NAME} Cloning repo"
cd /opt/nvidia/deepstream/deepstream/sources/apps
git clone https://github.com/NVIDIA-AI-IOT/redaction_with_deepstream.git
cd redaction_with_deepstream
echo "${NAME} Building repo"
if [[ "$(uname -m)" == "aarch64" ]]; then
  CUDA_VER=11.4 make
else
  CUDA_VER=11.6 make
fi;

# export version 11.6 for dGPU and 11.4 for jetson
./deepstream-redaction-app -c configs/pgie_config_fd_lpd.txt -i /opt/nvidia/deepstream/deepstream/samples/streams/sample_720p.mp4 -o ./face_detect.mp4

echo "${NAME} Finished building model.  Will copy to project directory now ..."
mkdir -p /src/configs/model
cp -r /opt/nvidia/deepstream/deepstream/sources/apps/redaction_with_deepstream/fd_lpd_model/* /src/configs/model

echo "${NAME} Downloading sample videos for face detection"
mkdir -p /src/sample_videos
cd /src/sample_videos && \
  wget https://github.com/intel-iot-devkit/sample-videos/raw/master/head-pose-face-detection-female.mp4 && \
  wget https://github.com/intel-iot-devkit/sample-videos/raw/master/head-pose-face-detection-male.mp4 && \
  wget https://github.com/intel-iot-devkit/sample-videos/raw/master/store-aisle-detection.mp4

echo "${NAME} FINISHED"