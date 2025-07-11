fn main() {
    // 设置链接搜索路径
    println!("cargo:rustc-link-search=native=../build");
    
    // 告诉Cargo重新构建的条件
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/user_task.rs");
    println!("cargo:rerun-if-changed=src/syscall_interface.rs");
    
    // 设置目标架构
    println!("cargo:rustc-env=TARGET=riscv64gc-unknown-none-elf");
    
    // 自动生成所有需要的C头文件和Rust接口
    generate_all_syscall_files();
    
    println!("cargo:warning=Generated syscall headers and Rust interface");
}

fn generate_all_syscall_files() {
    use std::fs;
    
    // 系统调用定义：一处定义，处处生成
    let syscalls = [
        ("exit", 1, "void", vec![("int", "status")]),
        ("write", 2, "long", vec![("int", "fd"), ("const char *", "buf"), ("size_t", "len")]),
        ("read", 3, "long", vec![("int", "fd"), ("char *", "buf"), ("size_t", "count")]),
        ("yield", 4, "void", vec![]),
        ("getpid", 5, "int", vec![]),  // 新增系统调用示例
    ];
    
    // 生成系统调用号头文件
    let mut syscall_numbers = String::new();
    syscall_numbers.push_str("#ifndef _SYSCALL_NUMBERS_H\n");
    syscall_numbers.push_str("#define _SYSCALL_NUMBERS_H\n\n");
    for (name, num, _, _) in &syscalls {
        syscall_numbers.push_str(&format!("#define __NR_{} {}\n", name.to_uppercase(), num));
    }
    syscall_numbers.push_str("\n#endif\n");
    
    // 生成C用户态系统调用声明（使用_syscallN宏）
    let mut user_syscalls = String::new();
    user_syscalls.push_str("#ifndef _USER_SYSCALLS_H\n");
    user_syscalls.push_str("#define _USER_SYSCALLS_H\n\n");
    user_syscalls.push_str("#include <stddef.h>  // for size_t\n");
    user_syscalls.push_str("#include \"kernel/syscall_numbers.h\"\n");
    user_syscalls.push_str("// Note: _syscallN macros are now provided by Rust implementation\n");
    user_syscalls.push_str("// C programs should link with Rust library to get syscall functions\n\n");
    
    // 生成函数声明而不是宏调用
    for (name, _, ret_type, params) in &syscalls {
        user_syscalls.push_str(&format!("extern {} {}(", ret_type, name));
        if params.is_empty() {
            user_syscalls.push_str("void");
        } else {
            for (i, (param_type, param_name)) in params.iter().enumerate() {
                if i > 0 { user_syscalls.push_str(", "); }
                user_syscalls.push_str(&format!("{} {}", param_type, param_name));
            }
        }
        user_syscalls.push_str(");\n");
    }
    user_syscalls.push_str("\n#endif\n");
    
    // 生成do_函数声明
    let mut do_functions = String::new();
    do_functions.push_str("#ifndef _DO_FUNCTIONS_H\n");
    do_functions.push_str("#define _DO_FUNCTIONS_H\n\n");
    do_functions.push_str("#include <stddef.h>\n\n");
    do_functions.push_str("#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");
    for (name, _, ret_type, params) in &syscalls {
        do_functions.push_str(&format!("{} do_{}(", ret_type, name));
        if params.is_empty() {
            do_functions.push_str("void");
        } else {
            for (i, (param_type, param_name)) in params.iter().enumerate() {
                if i > 0 { do_functions.push_str(", "); }
                do_functions.push_str(&format!("{} {}", param_type, param_name));
            }
        }
        do_functions.push_str(");\n");
    }
    do_functions.push_str("\n#ifdef __cplusplus\n}\n#endif\n\n#endif\n");
    
    // 写入C头文件
    fs::write("../include/kernel/syscall_numbers.h", syscall_numbers).unwrap();
    fs::write("../include/kernel/user_syscalls.h", user_syscalls).unwrap();
    fs::write("../include/kernel/do_functions.h", do_functions).unwrap();
    
    // 生成Rust接口文件
    generate_rust_syscall_interface(&syscalls);
    
    println!("Generated syscall headers and Rust interface from single source definition");
}

fn generate_rust_syscall_interface(syscalls: &[(&str, usize, &str, Vec<(&str, &str)>)]) {
    use std::fs;
    
    let mut rust_code = String::new();
    
    // 文件头注释
    rust_code.push_str("// 系统调用统一接口 - 由build.rs自动生成\n");
    rust_code.push_str("// 警告：请勿手动编辑此文件！所有修改应该在build.rs中进行\n");
    rust_code.push_str("#[allow(dead_code)]\n\n");
    
    // 生成系统调用号常量
    rust_code.push_str("// 系统调用号常量（与C头文件保持一致）\n");
    for (name, num, _, _) in syscalls {
        rust_code.push_str(&format!("pub const __NR_{}: usize = {};\n", name.to_uppercase(), num));
    }
    rust_code.push_str("\n");
    
    // 生成Rust用户态系统调用函数
    rust_code.push_str("// Rust用户态系统调用函数（与C函数同名）\n");
    for (name, _num, ret_type, params) in syscalls {
        // 生成函数签名
        rust_code.push_str("#[no_mangle]\n");
        rust_code.push_str(&format!("pub extern \"C\" fn {}(", 
            if *name == "yield" { "r#yield" } else { name }));
        
        // 参数列表
        let mut rust_params = Vec::new();
        for (param_type, param_name) in params {
            let rust_type = convert_c_type_to_rust(param_type);
            rust_params.push(format!("{}: {}", param_name, rust_type));
        }
        rust_code.push_str(&rust_params.join(", "));
        
        // 返回类型
        let rust_ret_type = convert_c_type_to_rust(ret_type);
        rust_code.push_str(&format!(") -> {} {{\n", rust_ret_type));
        
        // 函数体
        let arg_list = if params.is_empty() {
            "&[]".to_string()
        } else {
            let args: Vec<String> = params.iter().map(|(_, param_name)| {
                format!("{} as usize", param_name)
            }).collect();
            format!("&[{}]", args.join(", "))
        };
        
        rust_code.push_str(&format!("    syscall_raw(__NR_{}, {})\n", name.to_uppercase(), arg_list));
        rust_code.push_str("}\n\n");
    }
    
    // 生成do_函数的extern声明
    rust_code.push_str("// do_函数的extern声明（供内核调用，可以是C或Rust实现）\n");
    rust_code.push_str("extern \"C\" {\n");
    for (name, _, ret_type, params) in syscalls {
        rust_code.push_str(&format!("    fn do_{}(", name));
        let mut rust_params = Vec::new();
        for (param_type, param_name) in params {
            let rust_type = convert_c_type_to_rust(param_type);
            rust_params.push(format!("{}: {}", param_name, rust_type));
        }
        rust_code.push_str(&rust_params.join(", "));
        let rust_ret_type = convert_c_type_to_rust(ret_type);
        rust_code.push_str(&format!(") -> {};\n", rust_ret_type));
    }
    rust_code.push_str("}\n\n");
    
    // 添加syscall_raw辅助函数
    rust_code.push_str("// 辅助函数：执行原始系统调用\n");
    rust_code.push_str("fn syscall_raw<T>(nr: usize, args: &[usize]) -> T {\n");
    rust_code.push_str("    let mut ret: isize;\n");
    rust_code.push_str("    unsafe {\n");
    rust_code.push_str("        match args.len() {\n");
    rust_code.push_str("            0 => core::arch::asm!(\n");
    rust_code.push_str("                \"ecall\",\n");
    rust_code.push_str("                in(\"a7\") nr,\n");
    rust_code.push_str("                lateout(\"a0\") ret,\n");
    rust_code.push_str("                options(nostack)\n");
    rust_code.push_str("            ),\n");
    rust_code.push_str("            1 => core::arch::asm!(\n");
    rust_code.push_str("                \"ecall\",\n");
    rust_code.push_str("                in(\"a7\") nr,\n");
    rust_code.push_str("                in(\"a0\") args[0],\n");
    rust_code.push_str("                lateout(\"a0\") ret,\n");
    rust_code.push_str("                options(nostack)\n");
    rust_code.push_str("            ),\n");
    rust_code.push_str("            2 => core::arch::asm!(\n");
    rust_code.push_str("                \"ecall\",\n");
    rust_code.push_str("                in(\"a7\") nr,\n");
    rust_code.push_str("                in(\"a0\") args[0],\n");
    rust_code.push_str("                in(\"a1\") args[1],\n");
    rust_code.push_str("                lateout(\"a0\") ret,\n");
    rust_code.push_str("                options(nostack)\n");
    rust_code.push_str("            ),\n");
    rust_code.push_str("            3 => core::arch::asm!(\n");
    rust_code.push_str("                \"ecall\",\n");
    rust_code.push_str("                in(\"a7\") nr,\n");
    rust_code.push_str("                in(\"a0\") args[0],\n");
    rust_code.push_str("                in(\"a1\") args[1],\n");
    rust_code.push_str("                in(\"a2\") args[2],\n");
    rust_code.push_str("                lateout(\"a0\") ret,\n");
    rust_code.push_str("                options(nostack)\n");
    rust_code.push_str("            ),\n");
    rust_code.push_str("            _ => panic!(\"Too many arguments for syscall\"),\n");
    rust_code.push_str("        }\n");
    rust_code.push_str("        core::mem::transmute_copy(&ret)\n");
    rust_code.push_str("    }\n");
    rust_code.push_str("}\n\n");
    
    rust_code.push_str("// 添加新系统调用只需要在build.rs的syscalls数组中增加一行即可！\n");
    
    // 写入生成的Rust文件到build目录
    fs::write("src/generated_syscall_interface.rs", rust_code).unwrap();
    println!("Generated Rust syscall interface: src/generated_syscall_interface.rs");
}

fn convert_c_type_to_rust(c_type: &str) -> &str {
    match c_type {
        "void" => "()",
        "int" => "i32", 
        "long" => "isize",
        "size_t" => "usize",
        "const char *" => "*const u8",
        "char *" => "*mut u8",
        _ => "usize", // 默认类型
    }
}
