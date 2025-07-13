#!/bin/bash

# RVOS DevContainer Setup Script (C only)
echo "🚀 Setting up RVOS development environment..."

# 更新包管理器
sudo apt update

# 安装RISC-V工具链和QEMU
echo "📦 Installing RISC-V toolchain and QEMU..."
sudo apt install -y \
    gcc-riscv64-unknown-elf \
    qemu-system-misc \
    gdb-multiarch \
    git \
    curl \
    wget \
    u-boot-tools \
    build-essential

echo "✅ RVOS development environment setup complete!"
echo "🔧 GCC RISC-V version: $(riscv64-unknown-elf-gcc --version | head -n1)"
echo "🖥️  QEMU version: $(qemu-system-riscv64 --version | head -n1)"
