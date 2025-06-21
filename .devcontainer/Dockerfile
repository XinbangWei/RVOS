
# Use a standard Ubuntu base image for broad compatibility.
FROM ubuntu:22.04

# Avoid interactive prompts during package installation.
ENV DEBIAN_FRONTEND=noninteractive

# Install all necessary system dependencies in a single layer to optimize image size.
# This includes the C toolchain, GDB for RISC-V, QEMU, and the RISC-V C cross-compiler.
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gdb-multiarch \
    qemu-system-misc \
    gcc-riscv64-unknown-elf \
    curl \
    git \
    ca-certificates \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# Set up environment variables for the Rust toolchain.
ENV RUSTUP_HOME=/usr/local/rustup \
    CARGO_HOME=/usr/local/cargo \
    PATH=/usr/local/cargo/bin:$PATH

# Install Rust, the RISC-V target, and essential components like clippy and rustfmt.
RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --default-toolchain stable \
    && rustup target add riscv64gc-unknown-none-elf \
    && rustup component add clippy rustfmt

# Create a non-root user 'vscode' for better security and to avoid permission issues.
# Give this user passwordless sudo privileges.
RUN useradd -m -s /bin/bash vscode \
    && echo "vscode ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/vscode

# Switch to the non-root user.
USER vscode

# Set a default working directory inside the container.
WORKDIR /workspaces/RVOS
