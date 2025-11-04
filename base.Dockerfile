# 环境基础镜像，包含安装opencv,ncnn,vulkan等依赖
# Base image
FROM ubuntu:20.04

# Set environment variables to non-interactive
ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    libopencv-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libvulkan-dev \
    glslang-tools

# Clone and build ncnn
RUN git clone https://github.com/Tencent/ncnn.git /opt/ncnn && \
    cd /opt/ncnn && \
    git submodule update --init && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && \
    make -j$(nproc) && \
    make install