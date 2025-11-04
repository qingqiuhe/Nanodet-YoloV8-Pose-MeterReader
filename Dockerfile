# Stage 1: Build the React frontend
FROM node:16 as frontend-builder
WORKDIR /app

# Copy package files and install dependencies
COPY web-ui/package*.json ./
RUN npm install

# Copy the rest of the frontend source code
COPY web-ui/ ./

# Build the frontend application
RUN npm run build

# Stage 2: Build the C++ backend and create the final image
FROM registry.cn-hangzhou.aliyuncs.com/qingqiuhe/meter-reader:base

# Remove any existing app directory to ensure a clean state
RUN rm -rf /app

# Copy the entire project context
COPY . /app

# Build the C++ project
RUN cd /app && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make

# Copy the built frontend from the builder stage
COPY --from=frontend-builder /app/build /app/web-ui/build

# Expose port for the web service
EXPOSE 8080

# Set the entrypoint
WORKDIR /app

ENTRYPOINT ["/app/meter_reader_api"]