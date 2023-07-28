#!/bin/bash
COUNTER=0

echo -n "${NAME} waiting for zookeeper to start: ${COUNTER}"

until ZK_STARTED=$(nc -z 127.0.0.1 2181); do
      let COUNTER++;
      echo -n ", ${COUNTER}"
      sleep 1
done &

wait

sleep 10

echo ""
echo "${NAME} Zookeeper has started!"

