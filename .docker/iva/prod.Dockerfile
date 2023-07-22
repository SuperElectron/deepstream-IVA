# syntax = docker/dockerfile:1.2
ARG IVA_BASE_IMG
ARG BRANCH
FROM $IVA_BASE_IMG
ARG TOKEN
RUN cd /tmp && git clone -b ${BRANCH} https://superelectron:$TOKEN@github.com/superelectron/deepstream-iva.git
RUN cp -r /tmp/deepstream-iva/iva/* /src && rm -rf /tmp/deepstream-iva && mkdir /src/build
RUN cd /src/build && cmake ..
RUN cd /src/build && make -j -l$(nproc/2)
RUN mv /src/build/iva /tmp/iva && rm -rf /src/* && mv /tmp/iva /src/iva

#RUN mkdir -p /tmp/deepstream-iva/build
#RUN cd /tmp/deepstream-iva/build && cmake ..
#RUN cd /tmp/deepstream-iva/build && make -j -l5
#RUN mv /tmp/deepstream-iva/build /src/build


## copy local folder to docker image
#COPY apt /start/apt
## install apt packages in docker iamge
#RUN --mount=type=cache,target=/var/cache/apt apt-get update -yqq --fix-missing && \
#    apt-get install -yqq --no-install-recommends \
#    $(cat /start/apt/general.pkglist) \
#    $(cat /start/apt/gstreamer.pkglist) \
#    $(cat /start/apt/project.pkglist)
#
## Base installations for the container
#COPY scripts /start/scripts
#RUN chmod 777 /start -R
#RUN cd /start/scripts && ./install_cmake.sh
#RUN cd /start/scripts && ./install.sh
#RUN cd /start/scripts && ./install_messaging.sh
#RUN cd /start/scripts && ./install_cudart.sh
#RUN --mount=type=cache,target=/var/cache/apt apt-get install -y cuda
## clean up downloaded packages
#RUN rm -rf /var/lib/apt/lists/*
#RUN rm -rf ~/.cache/gstreamer-1.0/registry.*.bin && apt-get update -yqq
#
## Command executed for container spin up ( docker-compose, docker run)
#HEALTHCHECK CMD ping -c3 -i 10 127.0.0.1
## default location for `docker exec`
#WORKDIR /src
#RUN mkdir -p /models
## default command
#CMD ["bash"]
