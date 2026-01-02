# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

FROM --platform=arm64 ubuntu:24.04

# Update the package list
RUN apt-get update && \
    apt-get clean && \
    apt-get install -y cmake make gcc g++ wget unzip git aria2 && \
    apt-get install -y software-properties-common
RUN apt-add-repository -s ppa:ubuntu-qcom-iot/qcom-ppa && \
    apt install -y qcom-fastrpc1 qcom-libdmabufheap-dev qcom-fastrpc-dev qcom-dspservices-headers-dev && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /app/Speech-To-Text/
WORKDIR /app/Speech-To-Text/
RUN git clone https://github.com/quic/ai-engine-direct-helper.git --recursive && \
    cd ai-engine-direct-helper && \
    git pull --recurse-submodules && \
    git checkout 3fd2c54

COPY whisper_base_en.py /app/Speech-To-Text/ai-engine-direct-helper/samples/python/whisper_base_en/whisper_base_en.py
COPY utils.patch requirements.txt /app/Speech-To-Text/ai-engine-direct-helper/
WORKDIR /app/Speech-To-Text/ai-engine-direct-helper
RUN git apply utils.patch
RUN wget https://repo.anaconda.com/miniconda/Miniconda3-py312_25.7.0-2-Linux-aarch64.sh && \
    bash Miniconda3-py312_25.7.0-2-Linux-aarch64.sh -b && \
    rm Miniconda3-py312_25.7.0-2-Linux-aarch64.sh
RUN . ~/miniconda3/bin/activate && \
    conda tos accept && \
    conda create -n py312 python=3.12 && \
    conda activate py312 && \
    pip install -r requirements.txt
RUN wget https://softwarecenter.qualcomm.com/api/download/software/sdks/Qualcomm_AI_Runtime_Community/All/2.38.0.250901/v2.38.0.250901.zip && \
    unzip v2.38.0.250901.zip && chmod +x qairt/2.38.0.250901/bin/aarch64-oe-linux-gcc11.2/* && \
    export  QNN_SDK_ROOT=/app/Speech-To-Text/ai-engine-direct-helper/qairt/2.38.0.250901/ && \
    . ~/miniconda3/bin/activate && \
    conda activate py312 && \
    python setup.py bdist_wheel && \
    pip install dist/qai_appbuilder-2.38.0-cp312-cp312-linux_aarch64.whl && \
    mkdir -p /app/Speech-To-Text/ai-engine-direct-helper/samples/python/qai_libs/ && \
    cp qairt/2.38.0.250901/lib/hexagon-v73/unsigned/* /app/Speech-To-Text/ai-engine-direct-helper/samples/python/qai_libs/ && \
    cp qairt/2.38.0.250901/lib/aarch64-oe-linux-gcc11.2/* /app/Speech-To-Text/ai-engine-direct-helper/samples/python/qai_libs/ && \
    rm -rf v2.38.0.250901.zip qairt
RUN mv /root/miniconda3/envs/py312/lib/libstdc++.so.6 /root/miniconda3/envs/py312/lib/libstdc++.so.6.bak
WORKDIR /app/Speech-To-Text/ai-engine-direct-helper/samples/python
COPY run.sh /app/Speech-To-Text/ai-engine-direct-helper/samples/python/
RUN chmod +x run.sh
