#!/bin/bash

COUNTER=0
export NAME="[wait_until_kafka_starts.sh] "
echo -n "${NAME} waiting for kafka to start: ${COUNTER}"
HOST_IP=$(hostname -I | cut -d ' ' -f1)
echo "Waiting for kafka server to start: ${HOST_IP}:9092"

until KAFKA_STARTED=$(nc -z "$HOST_IP" 9092); do
      let COUNTER++;
      echo -n ", ${COUNTER}"
      sleep 1
done &

wait

echo ""
echo "${NAME} Kafka has started: ${HOST_IP}:9092"

