#![no_std]

// 导入Rust用户任务模块
pub mod user_task;

// 导出C兼容的函数接口
pub use user_task::*;

// 导入fdt和string模块
pub mod fdt;
pub mod string;

// 包含自动生成的系统调用接口（由build.rs生成）
include!("generated_syscall_interface.rs");

// 导出系统调用函数供C代码调用
// 注意：不再需要pub use syscall_interface::*，因为函数直接包含在此模块中

// Panic处理函数 (仅在非测试编译时使用)
#[cfg(not(test))]
#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> ! {
    // TODO: 可以调用C的printk来打印panic信息
    loop {}
}