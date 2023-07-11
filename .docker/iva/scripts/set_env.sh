#!/bin/bash

# usage: source set_env.sh
DISPLAY=:1
TZ=$(cat /etc/timezone)
ARCHITECTURE=$(uname -m)
GST_PLUGIN_PATH=/opt/nvidia/deepstream/deepstream/lib/gst-plugins:/usr/lib/${ARCHITECTURE}-linux/gstreamer-1.0/deepstream/