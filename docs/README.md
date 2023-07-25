# IVA

- intelligent video analytics

---

This readme will help you get the project running, and help you with resources.

# Table of contents

1. [Overview](#Overview)
2. [Run](#Run)
3. [References](#References)


<a name="Overview"></a>
## Overview


1. the `Makefile` has useful commands to get you started with the project
   - there are 4 main containers: 
     - `iva`: the main container that performs video analytics
     - `camera`: an RTSP server that allows you to test analytics over RTSP connection
     - `kafka`: gives you a kakfa server if you want to test sending messages over the network
     - `kafakcat`: acts as a consumer to receive messages from `iva`

2. The `.cache` folder has all the main project files
    - contents includes a `README.md` for each sub-folder which explains HOW-TO

__Makefile__

- view available commands
```bash
make help 
```

__.cache folder__

- the file `.cache/configs/config.json` configures the `iva` application.  View `.cache/README.md` for more information


---

<a name="Run"></a>
# Run

When you run the docker containers for the first time, these commands below help you.  If you have the docker container already up and running, then you may only need to enter the docker container

__start__

- start all the docker containers
```bash
make start
make enter
./iva
```

__stop__

- stop all the docker containers
```bash
make stop
make clean  
```

---

<a name="References"></a>
# References

- [Docker commands](https://docs.docker.com/engine/reference/commandline/cli/)
- Module README
  - [camera](.cache/camera.md)
  - [iva](.cache/iva.md)
  - [kafka](.cache/kafka.md)
