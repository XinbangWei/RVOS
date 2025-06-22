use core::slice;

#[no_mangle]
pub extern "C" fn memcpy(dest: *mut u8, src: *const u8, n: usize) -> *mut u8 {
    unsafe {
        core::ptr::copy_nonoverlapping(src, dest, n);
    }
    dest
}

#[no_mangle]
pub extern "C" fn memset(s: *mut u8, c: i32, n: usize) -> *mut u8 {
    unsafe {
        core::ptr::write_bytes(s, c as u8, n);
    }
    s
}

#[no_mangle]
pub extern "C" fn memcmp(s1: *const u8, s2: *const u8, n: usize) -> i32 {
    unsafe {
        let s1_slice = slice::from_raw_parts(s1, n);
        let s2_slice = slice::from_raw_parts(s2, n);
        for i in 0..n {
            let b1 = s1_slice[i];
            let b2 = s2_slice[i];
            if b1 != b2 {
                return (b1 as i32) - (b2 as i32);
            }
        }
    }
    0
}

#[no_mangle]
pub extern "C" fn strlen(s: *const i8) -> usize {
    let mut len = 0;
    while unsafe { *s.add(len) } != 0 {
        len += 1;
    }
    len
}

#[no_mangle]
pub extern "C" fn strcmp(s1: *const i8, s2: *const i8) -> i32 {
    unsafe {
        let mut i = 0;
        loop {
            let c1 = *s1.add(i) as u8;
            let c2 = *s2.add(i) as u8;

            if c1 != c2 {
                return (c1 as i32) - (c2 as i32);
            }

            if c1 == 0 {
                return 0;
            }

            i += 1;
        }
    }
}

#[no_mangle]
pub extern "C" fn strncmp(s1: *const i8, s2: *const i8, n: usize) -> i32 {
    unsafe {
        for i in 0..n {
            let c1 = *s1.add(i) as u8;
            let c2 = *s2.add(i) as u8;

            if c1 != c2 {
                return (c1 as i32) - (c2 as i32);
            }

            if c1 == 0 {
                return 0;
            }
        }
    }
    0
}

#[no_mangle]
pub extern "C" fn strcpy(dest: *mut i8, src: *const i8) -> *mut i8 {
    unsafe {
        let mut i = 0;
        loop {
            let c = *src.add(i);
            *dest.add(i) = c;
            if c == 0 {
                break;
            }
            i += 1;
        }
    }
    dest
}

#[no_mangle]
pub extern "C" fn strcat(dest: *mut i8, src: *const i8) -> *mut i8 {
    unsafe {
        let dest_len = strlen(dest);
        strcpy(dest.add(dest_len), src);
    }
    dest
}

#[no_mangle]
pub extern "C" fn strrchr(s: *const i8, c: i32) -> *mut i8 {
    unsafe {
        let len = strlen(s);
        let ch = c as u8;

        for i in (0..=len).rev() {
            if *s.add(i) as u8 == ch {
                return s.add(i) as *mut i8;
            }
        }
    }
    core::ptr::null_mut()
}

#[no_mangle]
pub extern "C" fn strstr(haystack: *const i8, needle: *const i8) -> *mut i8 {
    unsafe {
        let haystack_len = strlen(haystack);
        let needle_len = strlen(needle);

        if needle_len == 0 {
            return haystack as *mut i8;
        }

        if needle_len > haystack_len {
            return core::ptr::null_mut();
        }

        for i in 0..=(haystack_len - needle_len) {
            if strncmp(haystack.add(i), needle, needle_len) == 0 {
                return haystack.add(i) as *mut i8;
            }
        }
    }
    core::ptr::null_mut()
}
