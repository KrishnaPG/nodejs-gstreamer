# Dockerfile
FROM ubuntu:22.04

# Install dependencies
RUN apt update && \
    apt install -y --no-install-recommends \
        build-essential \
        cmake \
        libgstreamer1.0-dev \
        libglib2.0-dev \
        libjson-c-dev \
        libzmq5-dev \
        gstreamer1.0-plugins-base \
        gstreamer1.0-plugins-good \
        gstreamer1.0-plugins-bad \
        gstreamer1.0-plugins-ugly \
        gstreamer1.0-libav \
        libva-dev \
        libnvidia-decode-dev \
        uuid-dev \
        && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /app

# Copy source files
COPY . .

# Build
RUN make

# Mount shared memory path for shmsink
VOLUME ["/tmp"]

# Expose ports
EXPOSE 5555 5556

# Run the worker
CMD ["./worker"]