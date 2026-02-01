FROM debian:bookworm AS builder
RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake g++ make git ca-certificates libssl-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY CMakeLists.txt .
COPY main.cxx .
COPY order_book_parser.h .
COPY src/ src/
COPY examples/ examples/
COPY tests/ tests/
COPY benchmarks/ benchmarks/
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --target nats_processor -j$(nproc)

FROM debian:bookworm-slim
RUN apt-get update && apt-get install -y --no-install-recommends libssl3 && rm -rf /var/lib/apt/lists/*
COPY --from=builder /src/build/nats_processor /usr/local/bin/nats_processor
COPY --from=builder /src/build/lib/libnats.so* /usr/lib/
RUN ldconfig
CMD ["nats_processor"]
