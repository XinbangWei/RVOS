#ifndef __RUST_INTERFACE_H__
#define __RUST_INTERFACE_H__

#include <kernel/types.h>

// Forward declaration of the DeviceInfo struct
struct DeviceInfo;

// Functions provided by the Rust crate
void init_fdt_and_devices_rust(uint64_t fdt_phys_addr);
const struct DeviceInfo *get_device_info(void);

// Definition of the DeviceInfo struct, matching the Rust layout
struct DeviceInfo {
    uint64_t uart_base;
    uint64_t plic_base;
    uint64_t clint_base;
};

#endif // __RUST_INTERFACE_H__
