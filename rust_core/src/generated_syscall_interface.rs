// 系统调用统一接口 - 由build.rs自动生成
// 警告：请勿手动编辑此文件！所有修改应该在build.rs中进行
#[allow(dead_code)]

// 系统调用号常量（与C头文件保持一致）
pub const __NR_EXIT: usize = 1;
pub const __NR_WRITE: usize = 2;
pub const __NR_READ: usize = 3;
pub const __NR_YIELD: usize = 4;
pub const __NR_GETPID: usize = 5;

// Rust用户态系统调用函数（与C函数同名）
#[no_mangle]
pub extern "C" fn exit(status: i32) -> () {
    syscall_raw(__NR_EXIT, &[status as usize])
}

#[no_mangle]
pub extern "C" fn write(fd: i32, buf: *const u8, len: usize) -> isize {
    syscall_raw(__NR_WRITE, &[fd as usize, buf as usize, len as usize])
}

#[no_mangle]
pub extern "C" fn read(fd: i32, buf: *mut u8, count: usize) -> isize {
    syscall_raw(__NR_READ, &[fd as usize, buf as usize, count as usize])
}

#[no_mangle]
pub extern "C" fn r#yield() -> () {
    syscall_raw(__NR_YIELD, &[])
}

#[no_mangle]
pub extern "C" fn getpid() -> i32 {
    syscall_raw(__NR_GETPID, &[])
}

// do_函数的extern声明（供内核调用，可以是C或Rust实现）
extern "C" {
    fn do_exit(status: i32) -> ();
    fn do_write(fd: i32, buf: *const u8, len: usize) -> isize;
    fn do_read(fd: i32, buf: *mut u8, count: usize) -> isize;
    fn do_yield() -> ();
    fn do_getpid() -> i32;
}

// 辅助函数：执行原始系统调用
fn syscall_raw<T>(nr: usize, args: &[usize]) -> T {
    let mut ret: isize;
    unsafe {
        match args.len() {
            0 => core::arch::asm!(
                "ecall",
                in("a7") nr,
                lateout("a0") ret,
                options(nostack)
            ),
            1 => core::arch::asm!(
                "ecall",
                in("a7") nr,
                in("a0") args[0],
                lateout("a0") ret,
                options(nostack)
            ),
            2 => core::arch::asm!(
                "ecall",
                in("a7") nr,
                in("a0") args[0],
                in("a1") args[1],
                lateout("a0") ret,
                options(nostack)
            ),
            3 => core::arch::asm!(
                "ecall",
                in("a7") nr,
                in("a0") args[0],
                in("a1") args[1],
                in("a2") args[2],
                lateout("a0") ret,
                options(nostack)
            ),
            _ => panic!("Too many arguments for syscall"),
        }
        core::mem::transmute_copy(&ret)
    }
}

// 添加新系统调用只需要在build.rs的syscalls数组中增加一行即可！
