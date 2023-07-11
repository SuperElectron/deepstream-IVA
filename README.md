
# Table of contents

1. [Overview](#Overview)
2. [Run](#Run)
3. [Reference](#Reference)


---

<a name="Overview"></a>
## Overview


__Docs__

```bash
sudo apt-get install -y graphviz doxygen
make docs
```


---


__ReadMe__

- [models](docs/models.md)


__Makefile__

- the project has been dockerized to make dependencies easier.
- the `Makefile` has terminal commands to help you use the project


__Installations__

- run this on the device to install any software dependencies (outside docker)

```bash
./build/setup.sh
```

__Project Start__

- build and start the docker project

```bash
make build
# start kafka
make start_kafka
# start kafkacat (for debugging)
make start_kafkacat
# start video-pipeline
make start_video

```

- after you `run the docker container`, you can ssh into the container.
- inside the docker container all installations are complete, so you can run the project

```bash
make enter
```

<a name="Run"></a>
## Run

- view that the containers are alive with `docker ps`
- check the health status of a docker container with `docker inspect`

```bash
docker ps 
CONTAINER ID   IMAGE                   COMMAND                  CREATED             STATUS                             PORTS    NAMES
b086c5e144cf   kafka:latest            "supervisord -c /etc…"   10 seconds ago      Up 10 seconds (health: starting)   ...      kafka
e89142ee257c   video-pipeline:latest   "bash -c 'sleep infi…"   10 seconds ago      Up 10 seconds (health: starting)   ...      video-pipeline

docker inspect --format='{{json .State.Health}}' video-pipeline-prod

```

- enter the video-pipeline service
```bash
# enable the display
xhost +
# go into video-pipeline container
make enter
# build
cd build && cmake ..
make -j -l4
# run
./gstreamer 
```

- for more info on the video-pipeline, view this service's [README.md](video-pipeline/README.md)



---

<a name="Reference"></a>
## Reference

- https://wiki.archlinux.org/title/GStreamer
- [Deepstream Tao Apps](https://github.com/NVIDIA-AI-IOT/deepstream_tao_apps)
- [Deepstream - download-models](https://github.com/NVIDIA-AI-IOT/deepstream_tao_apps/tree/release/tao3.0#2-download-models)
- [Deepstream SDK - DS_TAO_integration](https://docs.nvidia.com/metropolis/deepstream/6.1/dev-guide/text/DS_TAO_integration.html)

__Deepstream Docs__

- [Deepstream SDK docs](https://docs.nvidia.com/metropolis/deepstream/6.1/dev-guide/index.html)
- [Deepstream API docs](https://docs.nvidia.com/metropolis/deepstream/6.1/sdk-api/index.html)

__check your cuda versions__

```bash
dpkg -l | grep nvinfer

find / -name libnvinfer.so

nm -D /usr/lib/x86_64-linux-gnu/libnvinfer.so | grep tensorrt_version

find / -name NvInferVersion.h
cat /usr/include/x86_64-linux-gnu/NvInferVersion.h | grep NV_TENSORRT | head -n 3
```