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

WORKDIR /deepx/dx-runtime/
COPY ${RT_FILE} /deepx/dx-runtime/${RT_FILE}
RUN tar -xzvf ./${RT_FILE} && \
    cd $(basename ${RT_FILE} .tar.gz) && \
    ./install.sh --dep && \
    ./build.sh --install /usr/local

WORKDIR /deepx/dx-runtime
RUN rm -rf ./dx_rt*

COPY build.sh /deepx/dx-runtime/dx_stream/build.sh
COPY install.sh /deepx/dx-runtime/dx_stream/install.sh
COPY run_demo.sh /deepx/dx-runtime/dx_stream/run_demo.sh
COPY setup_sample_models.sh /deepx/dx-runtime/dx_stream/setup_sample_models.sh
COPY setup_sample_videos.sh /deepx/dx-runtime/dx_stream/setup_sample_videos.sh
COPY setup.sh /deepx/dx-runtime/dx_stream/setup.sh
COPY gst-dxstream-plugin /deepx/dx-runtime/dx_stream/gst-dxstream-plugin
COPY dx_stream /deepx/dx-runtime/dx_stream/dx_stream
COPY scripts /deepx/dx-runtime/dx_stream/scripts

WORKDIR /deepx/dx-runtime/dx_stream
RUN ./install.sh
RUN ./build.sh --install

ENTRYPOINT [ "/usr/local/bin/dxrtd" ]
