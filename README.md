# RVOS

A simple RISC-V operating system implementation.

## 快速开始

1. 准备环境
   - 安装 [Docker](https://www.docker.com/) 并确保服务正在运行。
   - 安装 [Visual Studio Code](https://code.visualstudio.com/) 并添加 "Dev Containers" 扩展。

2. 打开项目容器
   - 在 VS Code 中点击左下角的“Reopen in Container”按钮。
   - VS Code 会根据 `.devcontainer` 配置构建并启动容器。

3. 构建并运行
   - 在容器终端中运行 `make`，或使用 VS Code 的 “Terminal → Run Task → build-os”。

4. 调试
   - 在 VS Code 中选择 “Run and Debug → Debug RISC-V OS”。
   - 这会自动执行 `stop-qemu`、`start-qemu-server` 任务，然后启动 GDB 链接到 QEMU。

5. 关闭 QEMU
   - 调试结束后，`stop-qemu` 会自动执行，也可以手动运行任务。

6. 清理构建产物（可选）
   - 在项目根目录运行 `make clean`。