# --- STAGE 1: BUILDER ---
FROM ubuntu:24.04 AS builder

WORKDIR /build_recorder
ENV DEBIAN_FRONTEND=noninteractive

# Install only what is needed to compile
RUN sed -i 's|http://archive.ubuntu.com/ubuntu/|http://repo.telkomuniversity.ac.id/ubuntu/|g' /etc/apt/sources.list && \
    apt-get update && apt-get install -y \
    build-essential \
    cmake \
    nlohmann-json3-dev \
    libssl-dev \
    libprotobuf-dev \
    libcurl4-openssl-dev \
    libexpat1-dev \
    libavcodec-dev libavformat-dev libavutil-dev \
    libswresample-dev \
    liblog4cxx-dev

# Copy the entire SDK and source code for compilation
COPY agora_sdk/ /build_recorder/agora_sdk/
COPY example/recorder_local/ /build_recorder/example/recorder_local/

# Run the build process
WORKDIR /build_recorder/example/recorder_local
RUN mkdir build && cd build && \
    cmake .. && \
    make

# --- STAGE 2: RUNNER (Production Image) ---
FROM ubuntu:24.04

WORKDIR /app
ENV DEBIAN_FRONTEND=noninteractive

# Install only runtime dependencies (no compilers)
RUN sed -i \
    -e 's|http://archive.ubuntu.com/ubuntu/|http://repo.telkomuniversity.ac.id/ubuntu/|g' \
    -e 's|http://security.ubuntu.com/ubuntu/|http://repo.telkomuniversity.ac.id/ubuntu/|g' \
    /etc/apt/sources.list && \
    rm -f /etc/apt/sources.list.d/*.list && \
    apt-get update && \
    apt-get install -y \
        libssl3 \
        libcurl4 \
        libexpat1 \
        libavcodec60 \
        libavformat60 \
        libavutil58 \
        libswresample4 \
        liblog4cxx15 \
        fonts-noto-cjk \
        fontconfig \
        nodejs \
        npm && \
    rm -rf /var/lib/apt/lists/*

# 1. Copy the Agora SDK (.so files)
COPY agora_sdk/ /app/agora_sdk/

# 2. Copy the FRESHLY BUILT binary from the builder stage
RUN mkdir -p /app/recorder-bin
COPY --from=builder /build_recorder/example/recorder_local/build/recorder_local /app/recorder-bin/recorder

# 3. Copy other required files
COPY example/recorder.json /app/recorder-bin/recorder.json
COPY controller/ /app/controller/

# Set library path and permissions
RUN chmod +x /app/recorder-bin/recorder
ENV FONTCONFIG_PATH=/etc/fonts
ENV LD_LIBRARY_PATH="/app/agora_sdk:${LD_LIBRARY_PATH}"

RUN fc-cache -fv

# Final setup
WORKDIR /app/controller
RUN npm install

CMD ["npm", "start"]
