# syntax = docker/dockerfile:1.2
# Use build argument to specify the architecture-specific library path
ARG ARCH=x86_64

# Build stage for aarch64 (ARM64)
FROM nvcr.io/nvidia/deepstream-l4t:6.1-iot as aarch64_builder
COPY ansible/libraries/x86_64/libLexActivator.so /usr/local/lib/

# Build stage for x86_64 (AMD64)
FROM nvcr.io/nvidia/deepstream:6.1-devel as x86_64_builder
COPY ansible/libraries/aarch64/libLexActivator.so /usr/local/lib/

# Final stage for the multi-platform image
FROM ${ARCH}_builder as builder
LABEL maintainer="AlphaWise <matmccann@gmail.com>"

# Set Nvidia capabilities environment variables
ENV NVIDIA_DRIVER_CAPABILITIES all
ENV NVIDIA_VISIBLE_DEVICES all
ENV DEBIAN_FRONTEND noninteractive
# Set required variables for arm64 builds
ENV VCPKG_FORCE_SYSTEM_BINARIES 1
ENV PATH "/root/.local/bin:${PATH}"
# Set Gsteamer paths for installed libraries
ENV GST_PLUGIN_PATH=/opt/nvidia/deepstream/deepstream/lib/gst-plugins:/usr/lib/${ARCH}-linux-gnu/gstreamer-1.0/deepstream/

RUN chmod 777 /usr/local/lib/libLexActivator.so

# copy local folder to docker image
COPY apt /root/apt
# install apt packages in docker iamge
RUN --mount=type=cache,target=/var/cache/apt apt-get update -yqq --fix-missing && \
    apt-get install -yqq --no-install-recommends \
    $(cat /root/apt/general.apt) \
    $(cat /root/apt/gstreamer.apt) \
    $(cat /root/apt/project.apt) && \
    rm -rf /root/apt

# Base installations for the container
COPY scripts /root/scripts
RUN chmod 777 /root/scripts -R
RUN cd /root/scripts && ./install_cmake.sh
RUN cd /root/scripts && ./install_vcpkg.sh
RUN cd /root/scripts && ./install_messaging.sh
RUN cd /root/scripts && ./dl_deepstream_yolo.sh
RUN rm -rf /root/scripts

# COPY security libraries


# clean up downloaded packages
RUN rm -rf /var/lib/apt/lists/*
RUN rm -rf ~/.cache/gstreamer-1.0/registry.*.bin && apt-get update -yqq

# Command executed for container spin up ( docker-compose, docker run)
HEALTHCHECK CMD ping -c3 -i 10 127.0.0.1
# default location for `docker exec`
WORKDIR /src
# default command
CMD ["bash"]