# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

FROM --platform=arm64 ubuntu:24.04

# Update the package list
RUN apt-get update && \
    apt-get clean && \
    apt-get install -y cmake make gcc g++ wget unzip git && \
    apt-get install -y software-properties-common && \
    apt-add-repository -s ppa:ubuntu-qcom-iot/qcom-ppa && \
    apt install -y qcom-fastrpc1 qcom-libdmabufheap-dev qcom-fastrpc-dev qcom-dspservices-headers-dev && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /app/Text-To-Speech/
WORKDIR /app/Text-To-Speech/

COPY meloTTS_app.py OnnxRunnerHelper.py requirements.txt /app/Text-To-Speech/
RUN wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-aarch64.sh && \
    bash Miniconda3-latest-Linux-aarch64.sh -b && \
    rm Miniconda3-latest-Linux-aarch64.sh
RUN wget https://softwarecenter.qualcomm.com/api/download/software/sdks/Qualcomm_AI_Runtime_Community/All/2.38.0.250901/v2.38.0.250901.zip && \
    unzip v2.38.0.250901.zip && \
    chmod +x qairt/2.38.0.250901/bin/aarch64-oe-linux-gcc11.2/* && \
    mkdir -p /usr/lib/rfsa/adsp/ && \
    cp qairt/2.38.0.250901/bin/aarch64-oe-linux-gcc11.2/* /usr/bin && \
    cp qairt/2.38.0.250901/lib/hexagon-v73/unsigned/* /usr/lib/rfsa/adsp/ && \
    cp qairt/2.38.0.250901/lib/aarch64-oe-linux-gcc11.2/* /usr/lib/ && \
    . ~/miniconda3/bin/activate && \
    conda tos accept && \
    conda create -n py310 python=3.10.9 && \
    conda activate py310 && \
    pip install -r requirements.txt && \
    pip install flask && \
    python -m unidic download && \
    git clone --recursive https://github.com/microsoft/onnxruntime && \
    cd onnxruntime && \
    git checkout e5678a133f121ed3ea514960ac53a6dd060ac4c3 && \
    cd /app/Text-To-Speech/onnxruntime/tools/ci_build/ && \
    python build.py --use_qnn --qnn_home=/app/Text-To-Speech/qairt/2.38.0.250901/ --build_wheel --skip_submodule_sync --config Release --build_dir /app/Text-To-Speech/onnxruntime/build/ --allow_running_as_root --parallel 8 --skip_tests && \
    pip install /app/Text-To-Speech/onnxruntime/build/Release/dist/onnxruntime_qnn-1.23.0-cp310-cp310-linux_aarch64.whl && \
    cd /app/Text-To-Speech/ && \
    rm -rf v2.38.0.250901.zip qairt onnxruntime

RUN mv /root/miniconda3/envs/py310/lib/libstdc++.so.6 /root/miniconda3/envs/py310/lib/libstdc++.so.6.bak
WORKDIR /app/Text-To-Speech/
COPY run.sh /app/Text-To-Speech/run.sh
RUN chmod +x run.sh
