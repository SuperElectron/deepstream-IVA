#!/bin/bash

export THRESHOLD=0.5
export VIDEO1="/tmp/.cache/sample_videos/people-walking-trim-h264.mp4"
export DETECTION_MODEL="/tmp/.cache/configs/model/detection.yml"
export TRACKER="/opt/nvidia/deepstream/deepstream/samples/configs/deepstream-app/config_tracker_NvDCF_perf.yml"
export TRACKER_L="/opt/nvidia/deepstream/deepstream/lib/libnvds_nvmultiobjecttracker.so"

mkdir -p /tmp/gst;

GST_DEBUG_DUMP_DOT_DIR=/tmp/gst gst-launch-1.0 -e \
  nvstreammux name=mux batch-size=1 width=1280 height=720 nvbuf-memory-type=3 ! \
  nvinfer config-file-path=$DETECTION_MODEL ! \
  nvtracker ll-config-file=$TRACKER ll-lib-file=$TRACKER_L enable_batch_process=1 tracker-width=640 tracker-width=480 ! \
  nvvideoconvert ! nvdsosd display-clock=1 clock-font-size=30 display-text=1 ! nvvideoconvert ! \
  nvstreamdemux name=demux \
filesrc location="$VIDEO1" ! \
    qtdemux name=qtmux0 ! \
    h264parse config-interval=-1 ! \
    nvv4l2decoder ! \
    nvvideoconvert ! \
    mux.sink_0 \
demux.src_0 ! \
    queue ! \
    nvvideoconvert ! \
    x264enc ! \
    flvmux ! filesink location=out_0.mp4