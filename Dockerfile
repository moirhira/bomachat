FROM debian:bullseye-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    make \
    netcat \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY . .

RUN make -C src all


FROM debian:bullseye-slim AS runtime

WORKDIR /app

COPY --from=builder build/src/bomachat .

EXPOSE 6667

CMD ["sh", "-c", "./bomachat ${BOMA_PORT} ${BOMA_PASSWORD}"]