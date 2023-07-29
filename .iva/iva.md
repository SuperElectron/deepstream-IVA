
# iva

Describes how to configure and run the `iva` container 

---


# Table of contents

1. [Overview](#Overview)
2. [Config](#Configs)
3. [Camera Service](#Camera-Service)

---

<a name="Overview"></a>
## Overview


- All the project files for `iva` and `camera` are in the `.cache` directory, and is mounted to `/tmp/.cache` in the docker containers in the Makefile
- View the folder contents and notes
```bash
.cache
├── configs                       // iva configs
│       ├── config.json             // main iva configuration
│       └── model                   // model configs
│              ├── detection.yml    // describes primary inference
│              ├── nvds             // folder with inference config file 
│              └── tracker.yml      // describes object detection tracker
│
├── licenses                      // iva licenses
│       ├── license.dat             // project licenses
│       └── security.json           // company license 
├── sample_videos                 // video files mounted to docker containers (iva, camera)
│        ├── sample_720p.mp4         // add more videos as you please!
│        └── test.mp4                // camera docker container default video file
└── saved_media                   // location where any saved images or payloads are stored   
```


---

<a name="Configs"></a>
## Configs

- here is a sample `.cache/configs/config.json`

```json
{
  "messaging": {
    "kafka_server_ip": "192.168.1.73:9092",
    "enable": false
  },
  "pipeline": {
    "src_type": "file",
    "sink_type": "display",
    "sources": [
      "/tmp/.cache/sample_videos/sample_720p.mp4"
    ],
    "sinks": [
      "rtsp1.mp4",
      "rtsp2.mp4"
    ],
    "input_width": 1280,
    "input_height": 720,
    "sync": true
  },
  "processing": {
    "topic": "test",
    "device_id": "device-1000",
    "model": "model-name",
    "model_type": "FACE",
    "publish": false,
    "save": false,
    "display_detections": true,
    "bbox_line_thickness": 2,
    "min_confidence_to_display": 50,
    "font_size": 1
  }
}

```

- `messaging`: configures the kafka producer
  - `kafka_server_ip`: is the ip address of the kafka server.  By default, it will be the same as found in the Makefile
  - `enable`: set to false and messages will not be sent to server container
```bash
$ make help
----------------------
++   PROJECT VARS
...
++      HOST_IP        |   192.168.1.71
 
```

- `pipeline`: configures the video parameters for runtime
  - `src_type`: may be one of (file, rtsp)
  - `sink_type`: may be one of (file, rtmp)
  - `source`: the filepath(s) or rtsp url(s) that you wish to play
  - `sinks`: the filepath(s) or rtsp url(s) that you wish to play
  - `input_width`: the expected image size from the source
  - `input_height`: the expected image size from the source
  - `sync`: synchronize the pipeline to the expected FPS.  
    - Suggested to set `false` as it enables buffering
    - If running `src_type`=file and `sink_type`=file, set to false and it will run the pipeline as fast as possible (a good measure of max capable FPS)
    - If you want to see realistic detections, set to `true` and it will force the pipeline to its expected FPS and drop frames to keep it this way

- `processing`: configures how post processing on the AI model's outputs are done
  - `topic`: the kafka topic to publish data to (if save=true)
  - `device_id`: a reference field added to metadata:  Should be a unique identifier to the device (e.g. device-00001)
  - `model`: a reference field added to metadata: Model version (e.g. face_detection-v1.3)
  - `model_type`: a reference field added to metadata: description of the model (e.g. face_detection)
  - `publish`: send model outputs to kafka module.  Note this queues up data for kafka, when messaging.kafka=true
  - `save`: save payload to `/tmp/.cache/saved_media` for the length of play.
  - `display_detections`: write the bounding box and text onto the video feed.
  - `bbox_line_thickness`: line thickeness for bounding box if written to image
  - `min_confidence_to_display`: a threshold for writing bounding box and text to display
  - `font_size`: size of text written to display

---

<a name="Camera-Service"></a>
## Camera Service

- there is a `camera` service that acts as an RTSP camera for testing
```bash
make build_camera start_camera
 
```


