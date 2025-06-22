/// A C-compatible struct to hold base addresses of essential devices.
/// This struct is populated by Rust and read by C code.
#[repr(C)]
pub struct DeviceInfo {
    pub uart_base: usize,
    pub plic_base: usize,
    pub clint_base: usize,
}

// External C functions for accessing FDT cache data
extern "C" {
    fn c_get_uart_base() -> u64;
    fn c_get_plic_base() -> u64;
    fn c_get_clint_base() -> u64;
}

/// A static, mutable instance of DeviceInfo that stores the hardware information
/// discovered from the FDT.
/// It is `unsafe` to access because it's a mutable static, but access is
/// controlled via safe interfaces.
static mut DEVICE_INFO: DeviceInfo = DeviceInfo {
    uart_base: 0x10000000, // Default for QEMU virt machine
    plic_base: 0x0c000000, // Default for QEMU virt machine
    clint_base: 0x02000000, // Default for QEMU virt machine
};

/// Returns an immutable reference to the global DEVICE_INFO struct.
/// This is the safe way for C code to access device information.
/// Marked `#[no_mangle]` and `extern "C"` to be callable from C.
#[no_mangle]
pub extern "C" fn get_device_info() -> *const DeviceInfo {
    unsafe { &raw const DEVICE_INFO }
}

/// Initializes hardware information by parsing the Flattened Device Tree (FDT).
///
/// # Arguments
/// * `fdt_phys_addr`: The physical memory address of the .dtb file passed by the bootloader.
///
/// This function retrieves device addresses from the C FDT cache that has already
/// been populated by the existing C FDT parsing code. This provides the best of
/// both worlds: proven C FDT parsing with Rust safety guarantees.
#[no_mangle]
pub extern "C" fn init_fdt_and_devices_rust(_fdt_phys_addr: usize) {
    // The C code has already parsed the FDT and populated g_fdt_cache.
    // We just need to copy the data from the C cache to our Rust structure.
    unsafe {
        DEVICE_INFO.uart_base = c_get_uart_base() as usize;
        DEVICE_INFO.plic_base = c_get_plic_base() as usize;
        DEVICE_INFO.clint_base = c_get_clint_base() as usize;
    }
}
