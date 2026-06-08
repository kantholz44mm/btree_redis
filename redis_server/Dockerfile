FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      build-essential \
      cmake \
      ninja-build \
      ca-certificates \
      git \
      pkg-config \
      libboost-all-dev \
      libssl-dev \
      && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY . /src

RUN cmake -S . -B build -G "Ninja" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=/out && \
    cmake --build build --target redis_server -j$(nproc) && \
    if [ -f /out/redis_server ]; then strip /out/redis_server || true; fi

FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      ca-certificates \
      libstdc++6 \
      libgcc1 \
      libboost-all-dev \
      && rm -rf /var/lib/apt/lists/*

RUN useradd -m appuser || true
WORKDIR /home/appuser

COPY --from=builder /out/redis_server /usr/local/bin/redis_server
RUN chown appuser:appuser /usr/local/bin/redis_server
USER appuser

EXPOSE 3000

ENTRYPOINT ["/usr/local/bin/redis_server"]