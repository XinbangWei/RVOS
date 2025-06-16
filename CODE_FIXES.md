# RVOS 代码修复总结

## 关键修复代码记录

本文档记录了RVOS项目中所有重要的代码修复，这些修复解决了上下文切换、任务调度和系统调用等关键问题。

### 1. timer.c - 定时器中断修复

#### 关键修复: 启用机器模式全局中断
```c
void timer_init()
{
    /* enable machine-mode timer interrupts. */
    w_mie(r_mie() | MIE_MTIE);
    
    /* enable machine-mode global interrupts. */
    w_mstatus(r_mstatus() | MSTATUS_MIE);  // 关键修复: 之前被注释掉
    
    // 设置初始定时器中断间隔 - 使用更短的间隔便于调试
    timer_load(get_mtime() + TIMER_INTERVAL / 100);  // 0.01秒间隔
}
```

#### 抢占式调度尝试 (当前方案)
```c
void timer_handler()
{
    spin_lock();
    _tick++;
    
    // 运行到期的定时器
    run_timer_list();
    
    // 简化的抢占式调度：每10个tick直接调用schedule
    if (_tick % 10 == 0) {
        printf("Preemptive scheduling at tick %d\n", _tick);
        schedule();  // 注意: 这可能导致栈问题
    }
    
    // 确保设置下一个定时器中断
    timer_load(get_mtime() + TIMER_INTERVAL / 100);
    
    spin_unlock();
}
```

### 2. task.c - 任务管理和调度修复

#### 修复后的schedule()函数
```c
void schedule()
{
    printf("[DEBUG] schedule() called, current_task_id=%d\n", current_task_id);
    spin_lock();
    
    if (_top <= 0)
    {
        printf("[DEBUG] No tasks available (_top=%d)\n", _top);
        spin_unlock();
        return;
    }
    
    int next_task = -1;
    uint8_t highest_priority = 255;
    
    // 只有当前任务ID有效时才更新状态
    if (current_task_id >= 0 && current_task_id < MAX_TASKS && 
        tasks[current_task_id].state == TASK_RUNNING)
    {
        tasks[current_task_id].state = TASK_READY;
        printf("[DEBUG] Set current task %d to READY\n", current_task_id);
    }

    // 找到最高优先级
    for (int i = 0; i < _top; i++)
    {
        if (tasks[i].state == TASK_READY && tasks[i].priority < highest_priority)
        {
            highest_priority = tasks[i].priority;
        }
    }
    
    // 在最高优先级中轮转选择下一个任务
    for (int i = 0; i < _top; i++)
    {
        int start_idx = (current_task_id >= 0) ? current_task_id + 1 : 0;
        int idx = (start_idx + i) % _top;
        if (tasks[idx].state == TASK_READY && tasks[idx].priority == highest_priority)
        {
            next_task = idx;
            break;
        }
    }

    if (next_task == -1)
    {
        for (int i = 0; i < MAX_TASKS; i++)
        {
            if (tasks[i].state == TASK_READY && tasks[i].priority == highest_priority)
            {
                next_task = i;
                break;
            }
        }
    }

    if (next_task == -1)
    {
        printf("[DEBUG] No ready tasks found, returning\n");
        spin_unlock();
        return;
    }

    printf("[DEBUG] Switching to task %d (func=%p)\n", next_task, (void*)tasks[next_task].func);
    current_task_id = next_task;
    current_ctx = &(tasks[current_task_id].ctx);

    tasks[current_task_id].state = TASK_RUNNING;
    switch_to(current_ctx);
    spin_unlock();
}
```

#### 任务创建修复 (确保栈对齐)
```c
int task_create(void (*start_routin)(void *param), void *param, uint8_t priority, uint32_t timeslice)
{
    spin_lock();
    if (_top >= MAX_TASKS)
    {
        spin_unlock();
        return -1;
    }

    tasks[_top].func = start_routin;
    tasks[_top].param = param;
    
    // 确保栈指针16字节对齐
    tasks[_top].ctx.sp = (reg_t)&task_stack[_top][STACK_SIZE] & ~0xF;
    tasks[_top].ctx.pc = (reg_t)start_routin;
    
    // 设置参数寄存器
    tasks[_top].ctx.a0 = param;
    
    // 初始化 mstatus 为用户模式
    tasks[_top].ctx.mstatus = (0 << 11) | (1 << 7); // MPP = 0, MPIE = 1

    tasks[_top].priority = priority;
    tasks[_top].state = TASK_READY;
    tasks[_top].timeslice = timeslice;
    tasks[_top].remaining_timeslice = timeslice;

    printf("创建任务: %p\n", (void *)tasks[_top].ctx.pc);

    _top++;
    spin_unlock();
    return 0;
}
```

### 3. trap.c - 中断处理

#### 软件中断处理 (备选方案)
```c
case 3:  // 软件中断
{
    int id = r_mhartid();
    *(uint32_t *)CLINT_MSIP(id) = 0;  // 清除软件中断
    schedule();
    break;
}
```

#### 添加调试信息的异常处理
```c
case 5:
    uart_puts("Fault load!\n");
    printf("Load fault at address: 0x%x, bad address: 0x%x\n", epc, r_mtval());
    break;
```

### 4. riscv.h - 添加缺失的寄存器读取函数

```c
static inline reg_t r_mtval()
{
    reg_t x;
    asm volatile("csrr %0, mtval" : "=r" (x) );
    return x;
}
```

### 5. 关键数据结构定义

#### context结构 (os.h)
```c
struct context {
    // 保存所有通用寄存器
    reg_t ra;
    reg_t sp;
    // ... 其他寄存器
    reg_t pc;       // 程序计数器
    reg_t mstatus;  // 机器状态寄存器
};
```

#### task_t结构
```c
typedef struct
{
    struct context ctx;
    void *param;
    void (*func)(void *param);
    uint8_t priority;
    task_state state;
    uint32_t timeslice;
    uint32_t remaining_timeslice;
} task_t;
```

### 6. 测试用例

#### 正常工作的任务
```c
void user_task0(void *param)
{
    uart_puts("Task 0: Created!\n");
    while (1)
    {
        uart_puts("Task 0: Running...\n");
        task_delay(DELAY);  // 主动让出CPU
    }
}
```

#### 问题任务 (导致系统卡死)
```c
void just_while(void)
{
    while (1)
        ;  // 无限循环，不主动让出CPU
}
```

### 修复验证状态

✅ **已验证工作**:
- 上下文切换机制
- 协作式多任务调度
- 任务创建和销毁
- 系统调用处理
- 定时器中断触发

❌ **仍然存在问题**:
- 抢占式调度失效
- 无法从无限循环任务中切换出来

### 关键学习点

1. **RISC-V ABI要求**: 栈指针必须16字节对齐
2. **中断控制**: MSTATUS_MIE必须启用才能响应中断
3. **任务状态管理**: 需要仔细处理READY、RUNNING、EXITED状态转换
4. **调试重要性**: 详细的调试输出对于问题诊断至关重要

### 后续开发方向

1. **研究xv6-riscv**: 学习成熟项目的抢占式调度实现
2. **深入RISC-V手册**: 理解中断和上下文切换的硬件行为
3. **使用更好的调试工具**: GDB等调试器进行深入分析
4. **简化问题**: 创建最小化的抢占测试用例

---

*文档创建时间: 2025年6月5日*  
*这些修复代码已经在当前代码库中实现并测试验证*
