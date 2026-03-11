FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    pkg-config \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DGST_METADATA_BUILD_EXAMPLES=ON \
    -DGST_METADATA_BUILD_TESTS=ON \
    && cmake --build build -j$(nproc)

# Run unit tests
RUN cd build && ctest --output-on-failure

# --- Integration test stage -------------------------------------------------
FROM ubuntu:24.04 AS test

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libgstreamer1.0-0 \
    libgstreamer-plugins-base1.0-0 \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    && rm -rf /var/lib/apt/lists/*

# Copy built plugins
COPY --from=builder /src/build/examples/libgstmetawriters.so /usr/lib/gstreamer-1.0/
COPY --from=builder /src/build/examples/libgstmetareaders.so /usr/lib/gstreamer-1.0/

# Copy integration test script
COPY test/integration_test.sh /test/
RUN chmod +x /test/integration_test.sh

ENTRYPOINT ["/test/integration_test.sh"]
