

# Gstreamer Video Streaming

---

A gstreamer application that runs inference and sinks to (1) file and (2) hls sink.
Currently, this is set up for a linux environment to run a face detection model


# Table of contents

1. [Overview](#Overview)
2. [Run](#Run)
3. [Reference](#Reference)


<a name="Overview"></a>
## Overview

---
`
- the project has been dockerized to make dependencies easier.
- the `Makefile` has terminal commands to help you use the project
- [confluence-page](https://focusbug.atlassian.net/wiki/spaces/SPYD/pages/4030589)

__installations__

- run this on the device to install any software dependencies

```bash
./build/setup.sh
```

__docker__

- build and run

```bash
make build start
make enter
```

<a name="Run"></a>
## Run

- the app runs off two config files: `src/configs/config.json` to configure the modules and `src/config/config.yml` to configure the gstreamer pipeline


__configs__

- look at the configuration file: `src/configs/config.json`

```json
{
  "messaging": {                                      // properties /src/modules/messaging
    "producer": {
      "topic": "overlay-bbox",                              // default topic that producer will publish to
      "bootstrap": [
        { "bootstrap.servers": "192.168.1.73:9092" }        // address and port of kafka server
      ]
    },
    "consumer": {                         
      "topic": "spyder-distance",                           // topic that consumer will consume from
      "bootstrap": [
        { "bootstrap.servers": "192.168.1.73:9092" },       // address and port of kafka server
        { "auto.offset.reset": "latest" },                  // on start, skip to latest message in kafka server (last offset)
        { "enable.auto.offset.store": "true" },             // auto configure offset
        { "session.timeout.ms": "15000" },                  // timeout after 15 seconds with no connection
        { "enable.partition.eof": "true" },                 // it is okay to get EOF (try and read but nothing is available=caught up) 
        { "max.poll.interval.ms": "10800000" },             // maximum time that we poll without having any messages before complaining
        { "debug": "consumer" },                            // low-level rdkakfa logging
        { "queued.max.messages.kbytes": "5000" },           // flush queue if larger than this size
        { "coordinator.query.interval.ms": "5000" },        // coordinator interval to get assignments etc 
        { "heartbeat.interval.ms": "3000"                   // check heartbeat with every every 3 seconds
        }
      ]
    }
  },
  "pipeline": {                                       // properties /src/modules/gst/pipeline
    "config_file": "/src/configs/writeSpyder_display.yml"        // pipeline configuration file describing gstreamer elements
  },
  "processing": {                                     // properties /src/modules/gst/processing
      "topic": "overlay-bbox",                              // extract metadata and produce to this topic 
      "device_id": "overlay-001",                           // name of this device
      "paired_device_id": "spyder-1001",                    // name of the spyder device it is to be connected with
      "model": "facenet",                                   // name of the ai model (adds to metadata for historical logging)
      "model_type": "face-detection",                       // name of the ai model type (adds to metadata for historical logging)
      "publish": false,                                     // if a probe is attached to extract metadata, it will produce results
      "save": false,                                        // if a probe is attached to extract metadata, it will save images
      "bbox_line_thickness": 4,                             // overlay: thickness of the lines written to overlay (e.g. bounding box)
      "minimum_iou_score": 60,                              // overlay: thickness of the lines written to overlay (e.g. bounding box)
      "min_confidence_to_display": 40,                      // overlay: minimum confidence to display in ANY metadata (software threshold)
      "font_size": 2                                        // overlay: font size for on overlay
    }
  }
}
```

- and here is a sample config_overlay.yml

```yaml

hls:
  enable: True                                            // if true, then will cache hls in /src/outputs/video
  minutes_cached: 5                                       // total minutes for the cached files (saves 20 files) 
  caps: "video/x-raw,width=(int)1920,height=(int)1080"    // capabilities to apply (uses a conversion to force them into this)

source:                                             // all the gstreamer elements must be under this heading
  - element:                                        // each 'element' describes a gstreamer element (try $ gst-inspect-1.0 v4l2src)
    name: v4l2src                                   // gstreamer name of the element, try this: $ gst-inspect-1.0 v4l2src
    alias: source                                   // must be a unique name in this file (same as property - name=source)
    property:                                       // element properties: e.g. gst-launch-1.0 v4l2src device=/dev/video0
      - device=/dev/video0
    callback:
      type: signal                                  // type=(probe, signal): [probe=callback on pad; signal=callback on element signal]
      element_signal: prepare-format                // name of the element signal, try this: $ gst-inspect-1.0 v4l2src
      function_name: some_custom_function           // function from processing.cpp to add
  - element:
    name: queue
    alias: src_queue
  - element:
    name: videorate
    alias: src_rate
  - element:
    name: videoscale
    alias: src_scale
  - element:
    name: capsfilter
    alias: src_caps
    property:
      - caps=video/x-raw,framerate=(fraction)24/1,width=(int)1920,height=(int)1080
  - element:
    name: videoconvert
    alias: src_convert
  - element:
    name: capsfilter
    alias: sink_caps
    property:
      - caps=video/x-raw,format=(string)YV12
  - element:
    name: videoconvert
    alias: sink_conv
    callback:                                       // add a callback on this element
      type: probe                                   // type=(probe, signal): [probe=callback on pad; signal=callback on element signal]
      pad: sink                                     // pad to which the probe is added
      function_name: display_spider_detections      // name of the function in processing.cpp that will be added
  - element:
    name: queue
    alias: sink_queue
  - element:
    name: fpsdisplaysink
    alias: sink
    property:
      - sync=false
    callback:                                       // callback added to element
      type: signal                                  // type=signal
      element_signal: fps-measurements              // name of the element signal, try this: $ gst-inspect-1.0 fpsdisplaysink
      function_name: some_custom_function           // function from processing.cpp to add
```

__run__


- for more, view instructions in video-pipeline/CMakeLists.txt

```bash
# ssh into the docker container
make enter
# inside docker, go to the project directory
cd /src
# build cpp project
cmake -B build -S .
# go to directory with executable
cd build 
./gstreamer

```

---

<a name="Reference"></a>
## Reference

- https://wiki.archlinux.org/title/GStreamer

