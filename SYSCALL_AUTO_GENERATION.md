# 系统调用统一接口 - 单一源自动生成方案

## 概述

这个方案实现了"一处定义，处处可用"的系统调用管理机制。所有系统调用的定义只需要在一个地方（`rust_core/build.rs`）维护，所有相关的C头文件和Rust接口都会自动生成。

## 设计原理

### 单一数据源
在 `rust_core/build.rs` 中定义系统调用数组：
```rust
let syscalls = [
    ("exit", 1, "void", vec![("int", "status")]),
    ("write", 2, "long", vec![("int", "fd"), ("const char *", "buf"), ("size_t", "len")]),
    ("read", 3, "long", vec![("int", "fd"), ("char *", "buf"), ("size_t", "count")]),
    ("yield", 4, "void", vec![]),
    ("getpid", 5, "int", vec![]),  // 新增系统调用只需加这一行！
];
```

### 自动生成的文件

1. **C头文件** (自动生成到 `include/kernel/`):
   - `syscall_numbers.h` - 系统调用号宏定义
   - `user_syscalls.h` - C用户态函数声明  
   - `do_functions.h` - 内核态do_函数声明

2. **Rust接口** (自动生成到 `rust_core/src/`):
   - `generated_syscall_interface.rs` - 完整的Rust系统调用实现

## 使用方法

### 添加新的系统调用

只需要在 `rust_core/build.rs` 的 `syscalls` 数组中增加一行：

```rust
("new_syscall", 6, "return_type", vec![("param_type", "param_name")]),
```

然后重新构建：
```bash
cd rust_core && cargo build --target riscv64gc-unknown-none-elf
```

### 使用系统调用

**C代码:**
```c
#include "kernel/user_syscalls.h"

void my_function() {
    write(1, "Hello", 5);
    int pid = getpid();
    yield();
}
```

**Rust代码:**
```rust
// 这些函数通过lib.rs自动导入
write(1, b"Hello".as_ptr(), 5);
let pid = getpid();
r#yield();
```

## 核心优势

1. **单一维护点**: 所有系统调用只需在build.rs中定义一次
2. **自动一致性**: C和Rust接口自动保持同步，系统调用号不会冲突
3. **零维护开销**: 添加系统调用时不需要手动更新多个文件
4. **编译时验证**: 类型和接口在编译时自动检查

## 文件映射关系

```
build.rs中的定义
    ↓
├── syscall_numbers.h (#define __NR_XXX)
├── user_syscalls.h (extern declarations) 
├── do_functions.h (kernel side declarations)
└── generated_syscall_interface.rs (Rust implementation)
```

## 实现细节

- **自动类型转换**: C类型(`int`, `size_t`, `void*`)自动映射到Rust类型(`i32`, `usize`, `*mut u8`)
- **关键字处理**: `yield`等Rust关键字自动处理为`r#yield`
- **ABI兼容性**: 使用`#[no_mangle] extern "C"`确保C/Rust互操作
- **内联汇编**: 使用RISC-V ecall指令实现系统调用

## 未来扩展

- 支持更多参数类型和复杂结构体
- 添加系统调用参数验证代码生成
- 支持条件编译的系统调用特性

通过这个方案，我们真正实现了"添加系统调用只需要改一行代码"的目标！
