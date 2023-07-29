
# SystemD service for linux systems

This will start the docker container on boot
- requires that docker starts on boot

## Set Up
- assumes that the container doesn't require a docker network!
  - if it does, then you need to add a command to create the docker network before it starts

```bash
sudo cp docker-kafka.service /etc/systemd/system/docker-kafka.service
sudo chmod +x /etc/systemd/system/docker-kafka.service
# enable the service
sudo systemctl enable docker-kafka.service
sudo systemctl daemon-reload
sudo systemctl status docker-kafka.service
```
- locations: /home/focusbug-overlay/code/focusBug-overlay
