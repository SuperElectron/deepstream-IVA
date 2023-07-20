# syntax = docker/dockerfile:1.2
ARG DOCKER_BASE
FROM nvcr.io/nvidia/deepstream$DOCKER_BASE:6.1.1-samples

LABEL maintainer="AlphaWise <matmccann@gmail.com>"

# Set Nvidia capabilities environment variables
ENV NVIDIA_DRIVER_CAPABILITIES all
ENV NVIDIA_VISIBLE_DEVICES all
ENV DEBIAN_FRONTEND noninteractive
# Set required variables for arm64 builds
ENV VCPKG_FORCE_SYSTEM_BINARIES 1
ENV PATH "/root/.local/bin:${PATH}"
# Set Gsteamer paths for installed libraries
ENV ARCHITECTURE="$(uname -m)"
ENV GST_PLUGIN_PATH=/opt/nvidia/deepstream/deepstream/lib/gst-plugins:/usr/lib/${ARCHITECTURE}-linux-gnu/gstreamer-1.0/deepstream/

# copy local folder to docker image
COPY apt /start/apt
# install apt packages in docker iamge
RUN --mount=type=cache,target=/var/cache/apt apt-get update -yqq --fix-missing && \
    apt-get install -yqq --no-install-recommends \
    $(cat /start/apt/general.pkglist) \
    $(cat /start/apt/gstreamer.pkglist) \
    $(cat /start/apt/project.pkglist)

# Base installations for the container
COPY scripts /start/scripts
RUN chmod 777 /start -R
RUN cd /start/scripts && ./install_cmake.sh
RUN cd /start/scripts && ./install.sh
RUN cd /start/scripts && ./install_messaging.sh
RUN cd /start/scripts && ./install_cudart.sh
RUN --mount=type=cache,target=/var/cache/apt apt-get install -y cuda
# clean up downloaded packages
RUN rm -rf /var/lib/apt/lists/*
RUN rm -rf ~/.cache/gstreamer-1.0/registry.*.bin && apt-get update -yqq

RUN cp /usr/local/cuda/targets/x86_64-linux/lib/libcudart.so /opt/nvidia/deepstream/deepstream/lib/libcudart.so

# Command executed for container spin up ( docker-compose, docker run)
HEALTHCHECK CMD ping -c3 -i 10 127.0.0.1
# default location for `docker exec`
WORKDIR /src
RUN mkdir -p /models
# default command
CMD ["bash"]