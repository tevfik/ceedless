# ceedless — multi-stage build: compile CLI with Go, deploy on slim runtime.
# Total image ≈ 150 MB (debian-slim + gcc + gcovr + static Go binary).

FROM golang:1.22-bookworm AS cli-builder
WORKDIR /src
COPY cli/ ./cli/
RUN cd cli && \
    CGO_ENABLED=0 go build -trimpath -ldflags "-s -w" -o /out/ceedless .

FROM debian:stable-slim
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        gcc make gcovr ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# Framework sources are read by the CLI at runtime.
COPY include /opt/ceedless/include
COPY src     /opt/ceedless/src
COPY --from=cli-builder /out/ceedless /usr/local/bin/ceedless

ENV CEEDLESS_HOME=/opt/ceedless
ENV CC=gcc
WORKDIR /work

CMD ["ceedless", "test"]
