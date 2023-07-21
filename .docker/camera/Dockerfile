FROM ubuntu:20.04
LABEL maintainer="AlphaWise <matmccann@gmail.com>"

ENV DEBIAN_FRONTEND noninteractive

# Python module path for the root import location
ENV PYTHONPATH /

# Docker exec path for login and bash execution root location (mount application here)
WORKDIR /camera

COPY apt /start/apt
COPY scripts /start/scripts
RUN chmod 755 /start -R
# Base installations for the container
RUN --mount=type=cache,target=/var/cache/apt apt-get update -yqq --fix-missing && \
    apt-get install -yqq --no-install-recommends \
    $(cat /start/apt/general.pkg) \
    $(cat /start/apt/supervisord.pkg) \
    $(cat /start/apt/gstreamer.pkg) \
    $(cat /start/apt/nginx.pkg)  && \
    rm -rf /var/lib/apt/lists/*

# Supervisord logging directories
RUN mkdir -p /var/lock/apache2 /var/run/apache2 /var/run/sshd /var/log/supervisor

#COPY conf/nginx-default.conf /etc/nginx/sites-available/default
COPY conf/nginx_rtsp.conf /etc/nginx/nginx.conf
EXPOSE 80/tcp
EXPOSE 8554

COPY conf/supervisord.conf /etc/supervisor/conf.d/supervisor.conf
COPY scripts/rtsp_server.py /camera/rtsp_server.py

# Command executed for container spin up (docker run)
CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/conf.d/supervisor.conf"]