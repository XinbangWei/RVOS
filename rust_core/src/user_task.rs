use core::ffi::c_void;

/// A helper function to write a string slice to the console (fd=1).
fn puts(s: &str) {
    crate::write(1, s.as_ptr(), s.len());
}

/// Rust version of a user task.
/// This task demonstrates using syscalls from Rust.
#[no_mangle]
pub extern "C" fn rust_user_task2(_param: *mut c_void) {
    puts("Rust Task 2: Created!\n");

    for i in 0..5 {
        let mut buf = [0u8; 32];
        let s = "Rust Task 2: Running... iter \n";
        // A simple way to show iteration count without full formatting
        buf[..s.len()].copy_from_slice(s.as_bytes());
        buf[s.len() - 2] = b'0' + i as u8;
        puts(core::str::from_utf8(&buf[..s.len()]).unwrap_or(s));
        
        crate::r#yield(); // Yield the CPU
    }

    puts("Rust Task 2: Finished!\n");

    crate::exit(0); // Exit the task
}