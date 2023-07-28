

# Gstreamer Video Streaming

---

A gstreamer application that runs inference and sinks to (1) file and (2) hls sink.
Currently, this is set up for a linux environment to run a face detection model


# Table of contents

1. [Overview](#Overview)
2. [Run](#Run)
3. [Security](#Security)
4. [Reference](#Reference)

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
 //add docs
```

- and here is a sample config_overlay.yml

```yaml
// add docs
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

<a name="Security"></a>
## Security
https://docs.cryptlex.com/node-locked-licenses/using-lexactivator/using-lexactivator-with-c-c++-and-objective-c


---

<a name="Reference"></a>
## Reference

- https://wiki.archlinux.org/title/GStreamer

