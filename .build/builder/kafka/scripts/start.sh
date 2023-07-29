#!/bin/bash

# Example usage: ./start.sh

JAVA_HOME=/usr/lib/jvm/java-1.8.0-openjdk-amd64
/opt/kafka/bin/zookeeper-server-start.sh /opt/kafka/config/zookeeper.properties &

sleep 15;

exec /opt/kafka/bin/kafka-server-start.sh /opt/kafka/config/server.properties