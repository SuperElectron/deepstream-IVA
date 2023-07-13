# syntax = docker/dockerfile:1.2
FROM ubuntu:22.04
LABEL maintainer="FocusBug <matmcann@gmail.com>"
USER root

# docker buildx feature that enables caching (just for reference)
# https://github.com/moby/buildkit/blob/master/frontend/dockerfile/docs/reference.md
ARG ARCHITECTURE=amd
ARG KAFKA_BROKER
ARG ZOOKEEPER_BROKER
ENV DEBIAN_FRONTEND noninteractive
ENV JAVA_HOME "/usr/lib/jvm/java-1.8.0-openjdk-${ARCHITECTURE}64"

EXPOSE 22 2181 2888 3888 9200

RUN rm -f /etc/apt/apt.conf.d/docker-clean
RUN export PATH=$JAVA_HOME/bin:$PATH
COPY apt /start/apt
COPY scripts /start/scripts
RUN chmod +x /start -R

# Install apt packages
RUN --mount=type=cache,target=/var/cache/apt apt-get update -yqq
RUN --mount=type=cache,target=/var/cache/apt apt-get install -yqq --no-install-recommends  \
      $(cat /start/apt/general.pkglist)  \
      $(cat /start/apt/kafka.pkglist)  \
      $(cat /start/apt/supervisord.pkglist)

# Supervisord set up
RUN mkdir -p /var/lock/apache2 /var/run/apache2 /var/run/sshd /var/log/supervisor
COPY configs/supervisord.conf /etc/supervisor/conf.d/supervisord.conf
RUN chmod +x /etc/supervisor/conf.d/supervisord.conf

### Kafka+Zookeeper set up
RUN /start/scripts/install_kafka.sh
COPY configs/kafka-server.properties /opt/kafka/config/server.properties
COPY configs/zookeeper.properties /opt/kafka/config/zookeeper.properties
#RUN /start/scripts/update_configs.sh -k ${KAFKA_BROKER} -m 10000 -t 1000

# copy certs
COPY configs/certs /tmp/certs
RUN chmod 777 /tmp/certs

# clean up downloaded apt packages in container
RUN rm -rf /var/lib/apt/lists/*
# Finish up with containerized features
WORKDIR "/opt/kafka"
HEALTHCHECK CMD echo stat | nc -i 30 127.0.0.1 2181
CMD ["supervisord", "-c", "/etc/supervisor/conf.d/supervisord.conf"]