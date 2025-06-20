use core::ffi::c_void;

// 外部C函数声明 - 来自RVOS系统
extern "C" {
    fn printf(format: *const u8, ...) -> i32;
    fn task_delay(ticks: u32);
    fn task_exit();
    fn uart_puts(s: *const u8);
}

// 辅助函数：将Rust字符串转换为C字符串
fn rust_str_to_c_str(s: &str) -> *const u8 {
    // 注意：这里我们假设字符串是以null结尾的
    // 在实际应用中可能需要更复杂的处理
    s.as_ptr()
}

// 辅助函数：安全地调用printf
fn rust_printf(message: &str) {
    unsafe {
        printf(rust_str_to_c_str(message));
    }
}

// 辅助函数：安全地调用uart_puts
fn rust_uart_puts(message: &str) {
    unsafe {
        uart_puts(rust_str_to_c_str(message));
    }
}

// Rust版本的用户任务1 - 简单循环任务
#[no_mangle]
pub extern "C" fn rust_user_task1(param: *mut c_void) {
    rust_uart_puts("Rust Task 1: Created!\n\0");

    let mut counter = 0u32;
    loop {
        // 使用Rust的格式化字符串功能（需要no_std兼容的方式）
        rust_uart_puts("Rust Task 1: Running...\n\0");

        counter += 1;
        if counter > 5 {
            rust_uart_puts("Rust Task 1: Finishing after 5 iterations\n\0");
            break;
        }

        unsafe {
            task_delay(1000); // 延迟1000个时钟周期
        }
    }

    rust_uart_puts("Rust Task 1: Finished!\n\0");
    unsafe {
        task_exit();
    }
}

// Rust版本的用户任务2 - 带参数的任务
#[no_mangle]
pub extern "C" fn rust_user_task2(param: *mut c_void) {
    // 在no_std环境中，我们不能使用format!宏
    // 所以我们用简单的方式处理
    rust_uart_puts("Rust Task 2: Created!\n\0");

    rust_uart_puts("Rust Task 2: Working...\n\0");

    rust_uart_puts("Rust Task 2: Finished!\n\0");

    unsafe {
        task_exit();
    }
}

// Rust版本的系统调用测试任务
#[no_mangle]
pub extern "C" fn rust_syscall_test_task(param: *mut c_void) {
    rust_uart_puts("Rust Task: System call test starting...\n\0");

    // 在这里我们可以展示Rust的内存安全特性
    // 例如：安全的数组操作、借用检查等

    let test_array = [1u32, 2, 3, 4, 5];
    let mut sum = 0u32;

    // Rust的迭代器 - 比C循环更安全
    for &value in test_array.iter() {
        sum += value;
    }

    rust_uart_puts("Rust Task: Array sum calculated safely\n\0");

    // 模拟一些计算工作
    for i in 0..3 {
        rust_uart_puts("Rust Task: Processing...\n\0");
        unsafe {
            task_delay(500);
        }
    }

    rust_uart_puts("Rust Task: System call test completed!\n\0");
    unsafe {
        task_exit();
    }
}

// 展示Rust内存安全特性的任务
#[no_mangle]
pub extern "C" fn rust_memory_safe_task(param: *mut c_void) {
    rust_uart_puts("Rust Memory Safety Demo: Starting\n\0");

    // 展示Rust的Option类型使用
    let maybe_value: Option<u32> = Some(42);

    match maybe_value {
        Some(value) => {
            rust_uart_puts("Rust: Found value in Option\n\0");
        }
        None => {
            rust_uart_puts("Rust: Option is None\n\0");
        }
    }

    // 展示Result类型的错误处理
    let result: Result<u32, &str> = Ok(100);

    match result {
        Ok(_value) => {
            rust_uart_puts("Rust: Operation succeeded\n\0");
        }
        Err(_error) => {
            rust_uart_puts("Rust: Operation failed\n\0");
        }
    }

    rust_uart_puts("Rust Memory Safety Demo: Completed\n\0");
    unsafe {
        task_exit();
    }
}
