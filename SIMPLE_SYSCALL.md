# 极简系统调用架构说明

## 📁 文件结构（仅3个文件）

```
include/syscalls.h         # 系统调用定义（唯一需要修改）
kernel/syscall.c          # 内核系统调用实现
user/syscalls.c           # 用户态系统调用接口
```

## 🚀 添加新系统调用（3步）

### 步骤1：定义系统调用
在 `include/syscalls.h` 的 `SYSCALL_LIST` 中添加一行：

```c
#define SYSCALL_LIST \
    SYSCALL(exit,   void, int status) \
    SYSCALL(write,  long, int fd, const void *buf, size_t len) \
    SYSCALL(read,   long, int fd, void *buf, size_t count) \
    SYSCALL(yield,  void) \
    SYSCALL(getpid, int) \
    SYSCALL(open,   int, const char *pathname, int flags) \
    SYSCALL(sleep,  int, int seconds)  // <-- 新系统调用
```

### 步骤2：实现内核函数
在 `kernel/syscall.c` 中实现 `do_*` 函数：

```c
int do_sleep(int seconds)
{
    printk("Sleeping for %d seconds\n", seconds);
    // 实现睡眠逻辑
    return 0;
}
```

### 步骤3：添加用户接口
在 `user/syscalls.c` 中添加用户态包装：

```c
int sleep(int seconds) {
    return (int)syscall_raw(__NR_sleep, seconds, 0, 0, 0, 0, 0);
}
```

然后重新编译即可！

## ✨ 特点

- **极简**: 只有3个文件需要关心
- **自动**: 系统调用号、声明、表都自动生成
- **统一**: 所有系统调用使用相同的调用约定
- **类型安全**: 编译时检查参数类型
- **易维护**: 添加/删除系统调用只需修改少量代码

## 🔧 工作原理

1. **`include/syscalls.h`** 使用宏魔法自动生成：
   - 系统调用号 (`__NR_*`)
   - 用户态函数声明
   - 内核态函数声明

2. **`kernel/syscall.c`** 实现：
   - 所有 `do_*` 函数的具体逻辑
   - 系统调用表（函数指针数组）
   - 统一的分发函数 `do_syscall()`

3. **`user/syscalls.c`** 提供：
   - 统一的 `syscall_raw()` 汇编接口
   - 所有用户态系统调用包装函数

这样，你只需要在一个地方定义系统调用，然后实现对应的 `do_` 函数，就能让整个系统调用链正常工作！

## 🎯 使用示例

```c
// 用户程序
#include "syscalls.h"

int main() {
    write(1, "Hello World!\n", 13);
    int pid = getpid();
    sleep(2);
    exit(0);
}
```

相比之前的25个复杂文件，现在只需要关心3个核心文件，简洁明了！
