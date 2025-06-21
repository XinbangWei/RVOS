#![no_std]

// 导入Rust用户任务模块
pub mod user_task;

// 导出C兼容的函数接口
pub use user_task::*;

// Panic处理函数 (仅在非测试编译时使用)
#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    // TODO: 可以调用C的printf来打印panic信息
    loop {}
}

