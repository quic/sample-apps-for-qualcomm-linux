# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear

# Use the official Ubuntu 24.04 image for ARM64 as the base image
FROM --platform=arm64 ubuntu:24.04

# Install Python and pip with minimal dependencies
RUN apt-get update && \
    apt-get install -y\
        git wget && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /var/tmp/*

# Set working directory
WORKDIR /app/web-ui/

# Copy only requirements first to leverage Docker cache
COPY requirements.txt .

# Create virtual environment and install dependencies
RUN wget https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-aarch64.sh && \
    bash Miniconda3-latest-Linux-aarch64.sh -b && \
    rm Miniconda3-latest-Linux-aarch64.sh
RUN . ~/miniconda3/bin/activate && \
    conda tos accept && \
    conda create -n py312 python=3.12 && \
    conda activate py312 && \
    pip install -r requirements.txt

# Copy the rest of the application
COPY pages/ ./pages/
COPY static/ ./static/
COPY run.sh .
COPY app.py ./Home.py
RUN chmod +x run.sh

# Streamlit config
RUN mkdir -p ~/.streamlit && \
    echo "[general]\nemail = \"\"" > ~/.streamlit/credentials.toml
