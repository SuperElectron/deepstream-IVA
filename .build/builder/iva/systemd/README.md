
# SystemD service for linux systems

This will start the docker container on boot
- requires that docker starts on boot

## Set Up
- assumes that the container doesn't require a docker network!
  - if it does, then you need to add a command to create the docker network before it starts

```bash
sudo cp docker-database.service /etc/systemd/system/docker-video.service
sudo chmod +x /etc/systemd/system/docker-video.service
# enable the service
sudo systemctl enable docker-video.service
sudo systemctl daemon-reload
sudo systemctl status docker-video.service

```


- locations: /home/focusbug-overlay/code/focusBug-overlay
