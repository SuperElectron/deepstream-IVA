#!/bin/bash

# Example usage: ./update_configs.sh -h
# Example usage: ./update_configs.sh -k '172.24.0.7:9092' -z 'localhost:2181' -m 10000 -t 1000 -c /start/configs/kafka-server.properties
# Example usage: ./update_configs.sh -k '172.23.0.7:9092' -z 'localhost:2181' -m 10000 -t 1000

export WORKDIR="/start/scripts"
export NAME="[update_configs.sh]: "
echo "${NAME} STARTING "

# Bash failure reporting for the script
set -eE -o functrace
failure() {
  local lineno=$1
  local msg=$2
  echo "${NAME} Failed at $lineno: $msg"
}
trap '${NAME} failure ${LINENO} "$BASH_COMMAND"' ERR

# Accepted args for user to pass
usage() {
  echo ""
  echo "Usage: <must pass all flags displayed below in options"
  echo "$0 OPTIONS"
  echo "  -k -> kafka server (172.24.0.7:9092)"
  echo "  -z -> zookeeper server (localhost:2181)"
  echo "  -m -> kafka: server.properties: log.flush.interval.message (10000)"
  echo "  -t -> kafka: server.properties: log.flush.interval.ms (1000)"
  echo "  -c -> kafka: server.properties file to change:  (/start/configs/kafka-server.properties)"
  echo "HELP:"
  echo "$0 -h -> HELP!  This displays this usage"
  echo ""
  echo "Sample usage:"
  echo "$0 -t "
  echo ""
}

while getopts 'k:z:t:m:c:h' option
do
  case "${option}" in
    k) KAFKA_SERVER_IP=${OPTARG};;
    z) ZK_SERVER_IP=${OPTARG};;
    m) LOG_FLUSH_INTERVAL_MESSAGES=${OPTARG};;
    t) LOG_FLUSH_INTERVAL_MS=${OPTARG};;
    c) CONF_FILE=${OPTARG};;
    h) usage
    exit 1;;
   esac
done

######################################
## Script start
######################################

if [ -z "$KAFKA_SERVER_IP" ]; then
  KAFKA_SERVER_IP=172.23.0.7:9092
  echo "KAFKA_SERVER_IP arg not passed, using default: arg=(${KAFKA_SERVER_IP})";
fi

if [ -z "$ZK_SERVER_IP" ]; then
  ZK_SERVER_IP=localhost:2181
  echo "ZK_SERVER_IP arg not passed, using default: arg=(${ZK_SERVER_IP})";
fi

if [ -z "$LOG_FLUSH_INTERVAL_MESSAGES" ]; then
  LOG_FLUSH_INTERVAL_MESSAGES=10000
  echo "LOG_FLUSH_INTERVAL_MESSAGES arg not passed, using default: arg=(${LOG_FLUSH_INTERVAL_MESSAGES})";
fi

if [ -z "$LOG_FLUSH_INTERVAL_MS" ]; then
  LOG_FLUSH_INTERVAL_MS=1000
  echo "LOG_FLUSH_INTERVAL_MS arg not passed, using default: arg=(${LOG_FLUSH_INTERVAL_MS})";
fi

if [ -z "$CONF_FILE" ]; then
  CONF_FILE=/opt/kafka/config/server.properties
  echo "CONF_FILE arg not passed, using default: arg=(${CONF_FILE})";
fi

echo "${NAME} Update config file with properties "

sed -i "/^listeners=/c\listeners=PLAINTEXT://${KAFKA_SERVER_IP}" $CONF_FILE
sed -i "/^advertised.listeners=/c\advertised.listeners=PLAINTEXT://$KAFKA_SERVER_IP" $CONF_FILE
sed -i "/^listener.security.protocol.map=PLAINTEXT:PLAINTEXT=/c\listener.security.protocol.map=PLAINTEXT:PLAINTEXT" $CONF_FILE

sed -i "/^log.flush.interval.messages=/c\log.flush.interval.messages=${LOG_FLUSH_INTERVAL_MESSAGES}" $CONF_FILE
sed -i "/^log.flush.interval.ms=/c\log.flush.interval.ms=${LOG_FLUSH_INTERVAL_MS}" $CONF_FILE
sed -i "/^zookeeper.connect=/c\zookeeper.connect=${ZK_SERVER_IP}" $CONF_FILE

echo "${NAME} FINISHED "
