FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
  # g++ \
  clang \
  clangd \
  make