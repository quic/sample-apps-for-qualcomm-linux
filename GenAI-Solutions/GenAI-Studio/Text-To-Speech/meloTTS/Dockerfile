# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

FROM --platform=arm64 ubuntu:24.04

# Define the Build Argument (v73 for IQ9, v75 for IQ8)
ARG DSP_ARCH=v73

# Persist DSP_ARCH as ENV so it's available at runtime
ENV DSP_ARCH=${DSP_ARCH}

# Update the package list and install base dependencies
RUN apt-get update && \
    apt-get clean && \
    apt-get install -y cmake make gcc g++ wget unzip git software-properties-common && \
    apt-add-repository -s ppa:ubuntu-qcom-iot/qcom-ppa && \
    apt install -y qcom-fastrpc1 qcom-libdmabufheap-dev qcom-fastrpc-dev qcom-dspservices-headers-dev && \
    rm -rf /var/lib/apt/lists/*

RUN mkdir -p /app/Text-To-Speech/
WORKDIR /app/Text-To-Speech/

# Copy source files
COPY meloTTS_app.py OnnxRunnerHelper.py requirements.txt /app/Text-To-Speech/

# Install Miniconda
RUN wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-aarch64.sh && \
    bash Miniconda3-latest-Linux-aarch64.sh -b && \
    rm Miniconda3-latest-Linux-aarch64.sh

# Main build logic using conditional variables based on DSP_ARCH
ARG DSP_ARCH
RUN if [ "$DSP_ARCH" = "v75" ]; then \
        QAIRT_VER="2.44.0.260225"; \
        HEX_FOLDER="hexagon-v75"; \
        PY_VER="3.11"; \
        CONDA_ENV="py311"; \
        ORT_TAG="3a728b75062256951b6e19ce718907cf1a1d4cf0"; \
        WHEEL_NAME="onnxruntime_qnn-1.24.3-cp311-cp311-linux_aarch64.whl"; \
        EXTRA_CMAKE="--cmake_extra_defines onnxruntime_USE_KLEIDIAI=OFF onnxruntime_USE_SVE=OFF"; \
    else \
        QAIRT_VER="2.38.0.250901"; \
        HEX_FOLDER="hexagon-v73"; \
        PY_VER="3.10.9"; \
        CONDA_ENV="py310"; \
        ORT_TAG="e5678a133f121ed3ea514960ac53a6dd060ac4c3"; \
        WHEEL_NAME="onnxruntime_qnn-1.23.0-cp310-cp310-linux_aarch64.whl"; \
        EXTRA_CMAKE=""; \
    fi && \
    \
    # 1. Setup QAIRT
    wget https://softwarecenter.qualcomm.com/api/download/software/sdks/Qualcomm_AI_Runtime_Community/All/${QAIRT_VER}/v${QAIRT_VER}.zip && \
    unzip v${QAIRT_VER}.zip && \
    chmod +x qairt/${QAIRT_VER}/bin/aarch64-oe-linux-gcc11.2/* && \
    mkdir -p /usr/lib/rfsa/adsp/ && \
    cp qairt/${QAIRT_VER}/bin/aarch64-oe-linux-gcc11.2/* /usr/bin && \
    cp qairt/${QAIRT_VER}/lib/${HEX_FOLDER}/unsigned/* /usr/lib/rfsa/adsp/ && \
    cp qairt/${QAIRT_VER}/lib/aarch64-oe-linux-gcc11.2/* /usr/lib/ && \
    \
    # 2. Setup Conda Environment
    . ~/miniconda3/bin/activate && \
    conda tos accept && \
    conda create -n ${CONDA_ENV} python=${PY_VER} -y && \
    conda activate ${CONDA_ENV} && \
    pip install -r requirements.txt && \
    pip install flask && \
    python -m unidic download && \
    \
    # 3. Build OnnxRuntime
    git clone --recursive https://github.com/microsoft/onnxruntime && \
    cd onnxruntime && \
    git checkout ${ORT_TAG} && \
    cd /app/Text-To-Speech/onnxruntime/tools/ci_build/ && \
    if [ "$DSP_ARCH" = "v75" ]; then pip install -U flatbuffers; fi && \
    python build.py --use_qnn --qnn_home=/app/Text-To-Speech/qairt/${QAIRT_VER}/ \
        --build_wheel --skip_submodule_sync --config Release \
        --build_dir /app/Text-To-Speech/onnxruntime/build/ \
        --allow_running_as_root --parallel 8 --skip_tests \
        ${EXTRA_CMAKE} && \
    pip install /app/Text-To-Speech/onnxruntime/build/Release/dist/${WHEEL_NAME} && \
    \
    # 4. Finalizing and cleanup
    mv /root/miniconda3/envs/${CONDA_ENV}/lib/libstdc++.so.6 /root/miniconda3/envs/${CONDA_ENV}/lib/libstdc++.so.6.bak && \
    cd /app/Text-To-Speech/ && \
    rm -rf v${QAIRT_VER}.zip qairt onnxruntime

WORKDIR /app/Text-To-Speech/

# Generate run.sh dynamically based on DSP_ARCH build arg
RUN if [ "$DSP_ARCH" = "v75" ]; then \
        printf '#!/bin/bash\n. ~/miniconda3/bin/activate py311\npython3 meloTTS_app.py\n' > run.sh; \
    else \
        printf '#!/bin/bash\n. ~/miniconda3/bin/activate py310\npython3 meloTTS_app.py\n' > run.sh; \
    fi && \
    chmod +x run.sh