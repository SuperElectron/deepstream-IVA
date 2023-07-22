# syntax = docker/dockerfile:1.2
ARG IVA_BASE_IMG
FROM $IVA_BASE_IMG
ARG TOKEN
ARG BRANCH
RUN cd /tmp && git clone -b ${BRANCH} https://superelectron:$TOKEN@github.com/superelectron/deepstream-iva.git
RUN cp -r /tmp/deepstream-iva/iva/* /src && rm -rf /tmp/deepstream-iva && mkdir /src/build
RUN cd /src/build && cmake ..
RUN cd /src/build && make -j -l$(nproc/2)
RUN mv /src/build/iva /tmp/iva && rm -rf /src/* && mv /tmp/iva /src/iva

CMD ["/src/.iva"]
#CMD ["bash"]