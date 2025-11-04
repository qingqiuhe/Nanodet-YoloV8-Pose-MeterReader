# 基于环境基础镜像构建项目
FROM registry.cn-hangzhou.aliyuncs.com/qingqiuhe/meter-reader:base

RUN rm -rf /app

# Copy project files
COPY . /app

# Build the project
RUN cd /app && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make

# Expose port for the web service
EXPOSE 8080

# Set the entrypoint
WORKDIR /app

ENTRYPOINT ["/app/meter_reader_api"]