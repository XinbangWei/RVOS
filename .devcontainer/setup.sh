#!/bin/bash

# RVOS DevContainer Setup Script
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
    wget

# 安装Rust（如果features没有正确安装）
if ! command -v rustc &> /dev/null; then
    echo "🦀 Installing Rust..."
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
    source ~/.cargo/env
fi

# 添加RISC-V target
echo "🎯 Adding RISC-V target for Rust..."
rustup target add riscv64gc-unknown-none-elf

# 安装useful cargo工具
echo "🔧 Installing useful Cargo tools..."
cargo install cargo-binutils
rustup component add llvm-tools-preview

echo "✅ RVOS development environment setup complete!"
echo "🦀 Rust version: $(rustc --version)"
echo "🔧 GCC RISC-V version: $(riscv64-unknown-elf-gcc --version | head -n1)"
echo "🖥️  QEMU version: $(qemu-system-riscv64 --version | head -n1)"
