#!/bin/bash

# Example usage: ./install.sh

export WORKDIR="/start"
export NAME="[install.sh]: "
echo "${NAME} STARTING "

# Bash failure reporting for the script
set -eE -o functrace
failure() {
  local lineno=$1
  local msg=$2
  echo "${NAME} Failed at $lineno: $msg"
}
trap '${NAME} failure ${LINENO} "$BASH_COMMAND"' ERR

SCALA_VERSION=2.13
KAFKA_VERSION=3.4.0

echo "${NAME} Installing kafka to /opt/kafka "
cd /tmp && \
  curl -OL "https://archive.apache.org/dist/kafka/${KAFKA_VERSION}/kafka_${SCALA_VERSION}-${KAFKA_VERSION}.tgz" && \
  tar -zxvf kafka_${SCALA_VERSION}-${KAFKA_VERSION}.tgz && \
  mv kafka_${SCALA_VERSION}-${KAFKA_VERSION} kafka && \
  mv kafka /opt/

echo "${NAME} Set up logging "
mkdir -p /tmp/kafka-logs
chmod 777 /tmp/kafka-logs

echo "${NAME} FINISHED "
