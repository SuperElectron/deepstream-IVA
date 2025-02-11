#################################################
# Makefile to build and run docker project
#################################################
## set bash session environment variables
PROJECT_DIR?=$(shell pwd)
DOCKER_DIR:=$(PROJECT_DIR)/.build/build
include .env

# kafka topic environment variable
TOPIC?=test
ifeq ($(shell uname -m),x86_64)
 	ARCH=x86_64
 	ARCHITECTURE=amd
 	PROD_TAG=amd
 else
 	ARCH=aarch64
 	ARCHITECTURE=arm
 	DOCKER_BASE=-l4t
 	PROD_TAG=arm64
endif

#################################################
## HELPERS
help:
	@echo "----------------------"
	@echo "++   PROJECT VARS"
	@echo "++   	ARCHITECTURE   |    $(ARCHITECTURE)"
	@echo "++   	PROJECT_NAME   |    $(PROJECT_NAME)"
	@echo "++   	DOCKER_BASE    |    $(DOCKER_BASE)"
	@echo "++   	HOST_IP        |   $(HOST_IP)"
	@echo "----------------------"
	@echo "++   PROJECT COMMANDS"
	@echo "++ 	[create Doxygen docs]		make docs"
	@echo "++ 	[remove merged branches]     	make clean_git"
	@echo "++ 	[build the docker image]     	make build"
	@echo "++ 	[build the docker container] 	make start"
	@echo "++ 	[ssh into docker container]  	make enter"
	@echo "++ 	[stop the docker container]  	make stop"
	@echo "++ 	[clean up all docker memory] 	make clean"

docs: clean_docs
	@echo "Install doxygen locally, then these commands will work"
	mkdir -p docs/html && doxygen;
	@echo "View docs in your browser: $(PROJECT_DIR)/docs/html/index.html"
clean_docs:
	rm -r docs/html || echo "Directory is removed"

clean_git:
	git branch -D `git branch --merged | grep -v \* | xargs`

#################################################
## DOCKER PROJECT COMMANDS

# BUILD COMMANDS
network:
	docker network create \
      --driver=bridge \
      --subnet="$(SUBNET_BASE).0/24" \
      --gateway="$(SUBNET_BASE).1" \
      $(PROJECT_NAME)_server || echo "Network alive"

build: build_kafka build_iva network

build_iva:
	docker buildx build --build-arg ARCH=$(ARCH) -t $(IVA_BASE_IMG)-base $(DOCKER_DIR)/$(IVA_MODULE)
build_iva_prod:
	docker buildx build \
		--build-arg IVA_BASE_IMG=$(IVA_BASE_IMG)-base \
		--build-arg BRANCH=$(BRANCH) \
		--build-arg TOKEN=$(TOKEN) \
		-t $(IVA_BASE_IMG):$(PROD_TAG) \
		-f $(DOCKER_DIR)/$(IVA_MODULE)/prod.Dockerfile \
		$(DOCKER_DIR)/$(IVA_MODULE)

build_kafka:
	docker buildx build \
		--build-arg KAFKA_BROKER=$(HOST_IP):9092 \
		--build-arg ARCHITECTURE=$(ARCHITECTURE) \
		-t GIT_CR/$(KAFKA_MODULE):$(PROD_TAG) \
		$(DOCKER_DIR)/$(KAFKA_MODULE)

build_kafkacat:
	docker buildx build -t GIT_CR/$(KAFKACAT_MODULE):$(PROD_TAG) $(DOCKER_DIR)/$(KAFKA_MODULE)/$(KAFKACAT_MODULE)
build_camera:
	docker buildx build -t -t GIT_CR/$(CAMERA_MODULE):$(PROD_TAG) $(DOCKER_DIR)/$(CAMERA_MODULE)

# RUN COMMANDS
start: network start_iva start_kafka

start_iva:
	docker run -it --rm --name $(IVA_MODULE) \
		--net $(PROJECT_NAME)_server \
		--ip $(SUBNET_BASE).3 \
	    --user=0 \
	    --runtime nvidia \
	    --gpus all \
	    -e DISPLAY=$(DISPLAY) \
	    -e TZ=$(shell cat /etc/timezone) \
	    -v "/etc/timezone:/etc/timezone:ro" \
	    -v "/etc/localtime:/etc/localtime:ro" \
	    -v /tmp/.X11-unix/:/tmp/.X11-unix \
        -v ~/.Xauthority:/root/.Xauthority \
	    -v "/dev:/dev" \
	    -v $(PROJECT_DIR)/$(IVA_MODULE):/src \
	    -v $(PROJECT_DIR)/.iva:/root/.iva \
	    -w /src \
		$(IVA_BASE_IMG)-base

start_iva_prod:
	xhost +;
	docker run -it --rm --name $(IVA_MODULE)_prod \
		--net $(PROJECT_NAME)_server \
		--ip $(SUBNET_BASE).4 \
	    --user=0 \
	    --runtime nvidia \
	    --gpus all \
	    -e DISPLAY=$(DISPLAY) \
	    -e TZ=$(shell cat /etc/timezone) \
	    -v "/etc/timezone:/etc/timezone:ro" \
	    -v "/etc/localtime:/etc/localtime:ro" \
	    -v /tmp/.X11-unix/:/tmp/.X11-unix \
        -v ~/.Xauthority:/root/.Xauthority \
	    -v "/dev:/dev" \
	    -v `pwd`/.cache/:/tmp/.cache/ \
	    -w /src \
		$(IVA_BASE_IMG):$(PROD_TAG) bash

start_kafkacat:
	docker run -d --name $(KAFKACAT_MODULE) \
	    --net=host \
	    -u 0 \
	    $(KAFKACAT_MODULE) bash -c "sleep infinity"

#		-v $(DOCKER_DIR)/kafka/configs/kafka-server-SSL.properties:/opt/kafka/config/server.properties
start_kafka:
	docker run -it --name $(KAFKA_MODULE) \
		--net=host \
		-w /opt/kafka \
		$(KAFKA_MODULE)

start_camera:
	docker run -it --rm --name $(CAMERA_MODULE) \
		--net $(PROJECT_NAME)_server \
		--ip $(SUBNET_BASE).2 \
		--user dev \
		-e PYTHONUNBUFFERED=1 \
		-p 8554:8554 \
		-v $(PROJECT_DIR)/.cache/sample_videos/:/home/dev/sample_videos \
		$(CAMERA_MODULE)

# RUN EXECUTIVE COMMAND IN DOCKER CONTAINER (open bash terminal, run program, ... etc)
enter:
	docker exec -it $(IVA_MODULE) bash

# STOP COMMANDS
stop:
	docker kill $(IVA_MODULE) $(KAFKA_MODULE) $(KAFKACAT_MODULE)
	docker container rm $(IVA_MODULE) $(KAFKA_MODULE) $(KAFKACAT_MODULE)

# CLEAN COMMANDS
clean: stop
	docker volume prune -f;
	docker image prune -a;

# KAFKA HELPERS
kafka_helpers:
	@echo "docker exec -it kafkacat kafkacat -b $(HOST_IP):9092 -L"
	@echo "docker exec -it kafkacat bash -c 'kafkacat -b $(HOST_IP):9092 -t $(TOPIC) -C'"
	@echo "	docker exec -it kafkacat bash -c 'kafkacat -b $(HOST_IP):9092 -t $(TOPIC) -P'"
