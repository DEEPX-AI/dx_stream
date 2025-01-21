FROM ubuntu:20.04

ARG RT_FILE

ENV TZ=Asia/Seoul
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ >/etc/timezone

ENV DEBIAN_FRONTEND=noninteractive
ENV PATH="/usr/local/bin:${PATH}"

RUN apt-get update && apt-get install -y sudo software-properties-common && \
    add-apt-repository universe && \
    add-apt-repository multiverse && \
    apt-get update

RUN apt-get install -y x11-apps libx11-6 xauth libxext6 libxrender1 libxtst6 libxi6

USER root

WORKDIR /tmp
COPY ${RT_FILE} /tmp/${RT_FILE}
RUN tar -xzvf ./${RT_FILE} && \
    cd $(basename ${RT_FILE} .tar.gz) && \
    ./install.sh --dep && \
    ./build.sh --install /usr/local

COPY build.sh /usr/share/dx-stream/src/build.sh
COPY install.sh /usr/share/dx-stream/src/install.sh
COPY run_demo.sh /usr/share/dx-stream/src/run_demo.sh
COPY gst-dxstream-plugin /usr/share/dx-stream/src/gst-dxstream-plugin
COPY dx_stream /usr/share/dx-stream/src/dx_stream

WORKDIR /usr/share/dx-stream/src
RUN ./install.sh
RUN ./build.sh --install

ENTRYPOINT [ "/usr/local/bin/dxrtd" ]
