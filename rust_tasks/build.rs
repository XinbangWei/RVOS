fn main() {
    // 设置链接搜索路径
    println!("cargo:rustc-link-search=native=../build");
    
    // 告诉Cargo重新构建的条件
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/user_task.rs");
    
    // 设置目标架构
    println!("cargo:rustc-env=TARGET=riscv64gc-unknown-none-elf");
}
