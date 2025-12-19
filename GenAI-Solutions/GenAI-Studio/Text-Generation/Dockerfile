# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

# Use the official Ubuntu 24.04 image for ARM64 as the base image
FROM --platform=arm64 ubuntu:24.04

# Install system dependencies
RUN apt-get update && \
    apt-get install -y \
        cmake make gcc g++ wget unzip git nlohmann-json3-dev && \
    apt-get install -y software-properties-common && \
    apt-add-repository -s ppa:ubuntu-qcom-iot/qcom-ppa && \
    apt install -y qcom-fastrpc1 qcom-libdmabufheap-dev qcom-fastrpc-dev qcom-dspservices-headers-dev && \
    rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /app/Text-Generation/

# Copy source files
COPY src/ ./src/

# Move into source directory
WORKDIR /app/Text-Generation/src/

RUN wget https://github.com/CrowCpp/Crow/releases/download/v1.2.1.2/Crow-1.2.1-Linux.tar.gz && \
    tar -xzf Crow-1.2.1-Linux.tar.gz && mv Crow-1.2.1-Linux crow && \
    git clone https://github.com/chriskohlhoff/asio.git && \
    wget https://softwarecenter.qualcomm.com/api/download/software/sdks/Qualcomm_AI_Runtime_Community/All/2.40.0.251030/v2.40.0.251030.zip && \
    unzip v2.40.0.251030.zip && mkdir -p build && cd build && cmake .. && make -j8 && cp llamachat /usr/bin/ && chmod +x /usr/bin/llamachat && \
    mkdir -p /usr/lib/rfsa/adsp && \
    cp /app/Text-Generation/src/qairt/2.40.0.251030/lib/hexagon-v73/unsigned/* /usr/lib/rfsa/adsp/ && \
    cp /app/Text-Generation/src/qairt/2.40.0.251030/lib/aarch64-oe-linux-gcc11.2/* /usr/lib/ && \
    chmod +x /app/Text-Generation/src/qairt/2.40.0.251030/bin/aarch64-oe-linux-gcc11.2/* && \
    cp /app/Text-Generation/src/qairt/2.40.0.251030/bin/aarch64-oe-linux-gcc11.2/* /usr/bin/ && \
    cd /app/ && rm -rf Text-Generation

# Clean up source directory
WORKDIR /app/

# Copy and prepare run script
COPY run.sh .
RUN chmod +x run.sh
