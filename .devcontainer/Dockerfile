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
    u-boot-tools \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user 'vscode' to enhance security and avoid permission issues.
# This user is granted passwordless sudo access.
RUN useradd -m -s /bin/bash vscode \
    && echo "vscode ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/vscode

# Switch the context to the non-root user.
USER vscode

# Set the default working directory for the container.
WORKDIR /workspaces/RVOS