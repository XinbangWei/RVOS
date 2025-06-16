# RVOS 技术问题详细记录

## Issue #1: 抢占式多任务调度失效

### 问题概述
系统在执行包含无限循环的任务时无法进行抢占式调度，导致系统卡死。

### 详细现象
1. 系统正常启动，前5个任务（0-4）能正常执行
2. Task 5 (`just_while`) 开始执行后系统完全卡死
3. 没有进一步的控制台输出
4. 定时器中断能够触发，但无法有效进行任务切换

### 技术栈信息
- **架构**: RISC-V 32位 (rv32g)
- **模拟器**: QEMU system-riscv32
- **编译器**: riscv64-unknown-elf-gcc 13.2.0
- **机器模式**: Machine mode (M-mode)
- **中断控制器**: CLINT (Core Local Interruptor)

### 相关代码分析

#### 1. 定时器中断配置 (timer.c)
```c
void timer_init()
{
    // 启用机器模式定时器中断
    w_mie(r_mie() | MIE_MTIE);
    
    // 启用机器模式全局中断 - 关键修复
    w_mstatus(r_mstatus() | MSTATUS_MIE);
    
    // 设置初始定时器中断间隔
    timer_load(get_mtime() + TIMER_INTERVAL / 100);
}
```

#### 2. 定时器中断处理 (timer.c)
当前实现尝试在定时器中断中直接调用schedule():
```c
void timer_handler()
{
    spin_lock();
    _tick++;
    
    run_timer_list();
    
    // 抢占式调度尝试
    if (_tick % 10 == 0) {
        printf("Preemptive scheduling at tick %d\n", _tick);
        schedule();  // 问题可能出现在这里
    }
    
    timer_load(get_mtime() + TIMER_INTERVAL / 100);
    spin_unlock();
}
```

#### 3. 中断处理流程 (trap.c)
```c
reg_t trap_handler(reg_t epc, reg_t cause, struct context *ctx)
{
    if (cause & 0x80000000) {
        switch (cause_code) {
        case 7:  // 定时器中断
            timer_handler();
            break;
        }
    }
    return return_pc;
}
```

#### 4. 问题任务 (user.c)
```c
void just_while(void)
{
    while (1) {
        // 无限循环，永不主动让出CPU
    }
}
```

### 已尝试的解决方案

#### 方案1: 软件中断抢占
```c
// 在timer_handler中
if (_tick % 5 == 0) {
    int id = r_mhartid();
    *(uint32_t *)CLINT_MSIP(id) = 1;  // 触发软件中断
}

// 在trap_handler中
case 3:  // 软件中断
    int id = r_mhartid();
    *(uint32_t *)CLINT_MSIP(id) = 0;  // 清除软件中断
    schedule();
    break;
```
**结果**: 软件中断正确触发，但schedule()调用后仍然卡死。

#### 方案2: 定时器回调调度
```c
void schedule_wrapper(void *arg)
{
    schedule();
    timer_create(schedule_wrapper, NULL, 5);  // 重新创建
}
```
**结果**: 回调函数签名匹配问题，且基本逻辑与方案1类似。

#### 方案3: 直接定时器调度
在timer_handler中直接调用schedule()。
**结果**: 依然卡死，可能存在栈或上下文问题。

### 调试输出分析

#### 正常运行时的输出:
```
Task 0: Created!
Task 0: Running...
[DEBUG] back_to_os() called, about to call schedule()
[DEBUG] schedule() called, current_task_id=1
[DEBUG] Switching to task 2 (func=0x800041cc)
```

#### 卡死时的最后输出:
```
[DEBUG] schedule() called, current_task_id=4
[DEBUG] Switching to task 5 (func=0x8000416c)
```

之后没有任何输出，说明问题出现在Task 5开始执行后的某个时刻。

### 技术假设

#### 假设1: 中断上下文栈问题
从定时器中断上下文调用schedule()可能导致栈状态不一致。
- 中断发生时，硬件自动保存部分寄存器到栈
- schedule()进行任务切换时，可能与中断栈管理冲突

#### 假设2: 寄存器保存不完整
中断发生在just_while无限循环中时，可能某些寄存器状态没有正确保存。
- RISC-V中断处理的寄存器保存可能不完整
- 任务上下文保存与中断上下文保存的冲突

#### 假设3: 时机问题
定时器中断在循环的特定位置触发时可能导致问题。
- 编译器优化可能影响循环行为
- 特定指令序列与中断的交互问题

### 潜在解决方向

#### 1. 延迟调度机制
不在中断处理程序中直接调用schedule()，而是设置标志:
```c
volatile int need_schedule = 0;

void timer_handler() {
    if (_tick % 10 == 0) {
        need_schedule = 1;
    }
}

// 在适当的地方检查和处理调度请求
```

#### 2. 改进上下文切换
确保中断上下文和任务上下文的完全分离:
- 使用独立的中断栈
- 改进entry.S中的上下文保存逻辑

#### 3. 简化测试
创建更简单的抢占测试用例:
```c
void simple_loop(void) {
    for (int i = 0; i < 1000000; i++) {
        // 简单循环，便于调试
    }
}
```

### 相关文件清单

**核心文件**:
- `timer.c`: 定时器和抢占式调度逻辑
- `trap.c`: 中断处理和分发
- `entry.S`: 底层上下文切换
- `task.c`: 任务管理和调度器
- `user.c`: 测试任务定义

**关键函数**:
- `timer_handler()`: 定时器中断处理
- `schedule()`: 任务调度器核心
- `switch_to()`: 底层上下文切换
- `trap_handler()`: 顶层中断分发

### 下一步行动建议

1. **深入分析entry.S**: 确保中断上下文和任务上下文的正确处理
2. **添加更多调试**: 在关键点添加调试输出，定位确切的卡死位置
3. **参考标准实现**: 研究其他RISC-V操作系统的抢占式调度实现
4. **使用GDB调试**: 在QEMU中使用GDB进行更深入的调试

---

*创建时间: 2025年6月5日*
*最后更新: 2025年6月5日*
