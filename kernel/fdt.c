#include "kernel.h"
#include "fdt.h"
#include "string.h"
#include "arch/sbi.h"

/* Need malloc declaration */
extern void *malloc(size_t size);
extern void free(void *ptr);

/* Global boot information */
struct boot_info g_boot_info = {0};
struct fdt_cache g_fdt_cache = {0};

/* External symbols from start.S */
extern uint64_t boot_hartid_asm;
extern uint64_t dtb_addr_asm;

/* Forward declarations for static functions */
static void parse_cpu_info(void *fdt);
static void parse_uart_info(void *fdt);
static void parse_plic_info(void *fdt);
static void parse_clint_info(void *fdt);
static int fdt_find_node_by_compatible(void *fdt, const char *compatible);

/* Byte swap functions for big-endian FDT */
static uint32_t be32_to_cpu(uint32_t val) {
    return ((val & 0xff) << 24) | 
           ((val & 0xff00) << 8) | 
           ((val & 0xff0000) >> 8) | 
           ((val & 0xff000000) >> 24);
}

static uint64_t be64_to_cpu(uint64_t val) {
    return ((uint64_t)be32_to_cpu(val & 0xffffffff) << 32) | 
           be32_to_cpu(val >> 32);
}

/* Check FDT header validity */
int fdt_check_header(void *fdt) {
    struct fdt_header *header = (struct fdt_header *)fdt;
    
    if (be32_to_cpu(header->magic) != FDT_MAGIC) {
        return -1;
    }
    
    if (be32_to_cpu(header->version) < 16) {
        return -1;  /* Unsupported version */
    }
    
    return 0;
}

/* Read number from FDT cell */
uint64_t fdt_read_number(const void *cell, int size) {
    const uint32_t *p = (const uint32_t *)cell;
    uint64_t result = 0;
    
    for (int i = 0; i < size; i++) {
        result = (result << 32) | be32_to_cpu(p[i]);
    }
    
    return result;
}

/* Find a node by path */
int fdt_path_offset(void *fdt, const char *path) {
    if (!fdt || !path) return -1;
    
    struct fdt_header *header = (struct fdt_header *)fdt;
    uint32_t *struct_ptr = (uint32_t *)((char *)fdt + be32_to_cpu(header->off_dt_struct));
    int depth = 0;
    char current_path[256] = "/";
    int path_len = 1;
    
    while (1) {
        uint32_t token = be32_to_cpu(*struct_ptr++);
        
        switch (token) {
            case FDT_BEGIN_NODE: {
                char *node_name = (char *)struct_ptr;
                int name_len = strlen(node_name);
                
                if (depth > 0) {
                    if (path_len + name_len + 1 < sizeof(current_path)) {
                        strcat(current_path, node_name);
                        strcat(current_path, "/");
                        path_len += name_len + 1;
                    }
                }
                
                /* Check if this matches our target path */
                if (strncmp(current_path, path, strlen(path)) == 0) {
                    return (char *)struct_ptr - (char *)fdt;
                }
                
                depth++;
                /* Skip name (align to 4 bytes) */
                struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + name_len + 4) & ~3);
                break;
            }
            case FDT_END_NODE:
                depth--;
                if (depth >= 0) {
                    /* Remove last component from path */
                    char *last_slash = strrchr(current_path, '/');
                    if (last_slash && last_slash != current_path) {
                        *last_slash = '\0';
                        path_len = strlen(current_path);
                    }
                }
                break;
            case FDT_PROP: {
                uint32_t len = be32_to_cpu(*struct_ptr++);
                uint32_t nameoff = be32_to_cpu(*struct_ptr++);
                /* Skip property data (align to 4 bytes) */
                struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + len + 3) & ~3);
                break;
            }
            case FDT_NOP:
                break;
            case FDT_END:
                return -1;
            default:
                return -1;
        }
    }
}

/* Get property from a node */
void *fdt_get_property(void *fdt, int nodeoffset, const char *name, int *lenp) {
    if (!fdt || nodeoffset < 0 || !name || !lenp) {
        *lenp = 0;
        return NULL;
    }
    
    struct fdt_header *header = (struct fdt_header *)fdt;
    char *strings_start = (char *)fdt + be32_to_cpu(header->off_dt_strings);
    uint32_t *struct_ptr = (uint32_t *)((char *)fdt + nodeoffset);
    
    /* Skip to first property */
    while (1) {
        uint32_t token = be32_to_cpu(*struct_ptr++);
        
        if (token == FDT_BEGIN_NODE) {
            char *node_name = (char *)struct_ptr;
            int name_len = strlen(node_name);
            struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + name_len + 4) & ~3);
            continue;
        } else if (token == FDT_PROP) {
            uint32_t len = be32_to_cpu(*struct_ptr++);
            uint32_t nameoff = be32_to_cpu(*struct_ptr++);
            char *prop_name = strings_start + nameoff;
            
            if (strcmp(prop_name, name) == 0) {
                *lenp = len;
                return struct_ptr;
            }
            
            /* Skip property data */
            struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + len + 3) & ~3);
        } else {
            break;
        }
    }
    
    *lenp = 0;
    return NULL;
}

/* Parse memory information from device tree */
static void parse_memory_info(void *fdt) {
    g_boot_info.memory_start = 0x80200000;  /* Our kernel start */
    g_boot_info.memory_size = 128 * 1024 * 1024;  /* 128MB default */
    
    if (!fdt) {
        printf("No FDT, using default memory config\n");
        return;
    }
    
    /* Try to find memory node */
    struct fdt_header *header = (struct fdt_header *)fdt;
    char *struct_start = (char *)fdt + be32_to_cpu(header->off_dt_struct);
    char *strings_start = (char *)fdt + be32_to_cpu(header->off_dt_strings);
    uint32_t *struct_ptr = (uint32_t *)struct_start;
    
    while (1) {
        uint32_t token = be32_to_cpu(*struct_ptr++);
        
        if (token == FDT_BEGIN_NODE) {
            char *node_name = (char *)struct_ptr;
            int name_len = strlen(node_name);
            
            /* Check if this is memory node */
            if (strncmp(node_name, "memory", 6) == 0) {
                printf("Found memory node: %s\n", node_name);
                
                /* Skip node name */
                struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + name_len + 4) & ~3);
                
                /* Look for reg property */
                while (1) {
                    uint32_t prop_token = be32_to_cpu(*struct_ptr++);
                    if (prop_token == FDT_PROP) {
                        uint32_t len = be32_to_cpu(*struct_ptr++);
                        uint32_t nameoff = be32_to_cpu(*struct_ptr++);
                        char *prop_name = strings_start + nameoff;
                        
                        if (strcmp(prop_name, "reg") == 0 && len >= 8) {
                            /* reg property contains address and size */
                            uint64_t addr = fdt_read_number(struct_ptr, 2);
                            uint64_t size = fdt_read_number((char*)struct_ptr + 8, 2);
                            
                            g_boot_info.memory_start = addr;
                            g_boot_info.memory_size = size;
                            printf("Memory from DT: start=0x%lx, size=0x%lx\n", addr, size);
                            return;
                        }
                        
                        /* Skip property data */
                        struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + len + 3) & ~3);
                    } else if (prop_token == FDT_END_NODE) {
                        break;
                    } else if (prop_token == FDT_END) {
                        goto end_parse;
                    }
                }
            } else {
                /* Skip node name */
                struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + name_len + 4) & ~3);
            }
        } else if (token == FDT_END_NODE) {
            continue;
        } else if (token == FDT_PROP) {
            uint32_t len = be32_to_cpu(*struct_ptr++);
            struct_ptr++; /* skip nameoff */
            /* Skip property data */
            struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + len + 3) & ~3);
        } else if (token == FDT_END) {
            break;
        }
    }
    
end_parse:
    printf("Using memory config: start=0x%lx, size=0x%lx\n", 
           g_boot_info.memory_start, g_boot_info.memory_size);
}

/* Parse CPU information from device tree */
static void parse_cpu_info(void *fdt) {
    g_boot_info.cpu_count = 1;
    g_boot_info.timebase_freq = 10000000;  /* 10MHz default */
    
    if (!fdt) {
        printf("No FDT, using default CPU config\n");
        return;
    }
    
    /* Try to find cpus node */
    struct fdt_header *header = (struct fdt_header *)fdt;
    char *struct_start = (char *)fdt + be32_to_cpu(header->off_dt_struct);
    char *strings_start = (char *)fdt + be32_to_cpu(header->off_dt_strings);
    uint32_t *struct_ptr = (uint32_t *)struct_start;
    
    int cpu_count = 0;
    
    while (1) {
        uint32_t token = be32_to_cpu(*struct_ptr++);
        
        if (token == FDT_BEGIN_NODE) {
            char *node_name = (char *)struct_ptr;
            int name_len = strlen(node_name);
            
            /* Check if this is cpus node */
            if (strcmp(node_name, "cpus") == 0) {
                printf("Found cpus node\n");
                
                /* Skip node name */
                struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + name_len + 4) & ~3);
                
                /* Look for timebase-frequency property */
                while (1) {
                    uint32_t prop_token = be32_to_cpu(*struct_ptr++);
                    if (prop_token == FDT_PROP) {
                        uint32_t len = be32_to_cpu(*struct_ptr++);
                        uint32_t nameoff = be32_to_cpu(*struct_ptr++);
                        char *prop_name = strings_start + nameoff;
                        
                        if (strcmp(prop_name, "timebase-frequency") == 0 && len == 4) {
                            g_boot_info.timebase_freq = be32_to_cpu(*struct_ptr);
                            printf("Timebase frequency: %ld\n", g_boot_info.timebase_freq);
                        }
                        
                        /* Skip property data */
                        struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + len + 3) & ~3);
                    } else if (prop_token == FDT_BEGIN_NODE) {
                        /* Count CPU nodes */
                        char *cpu_name = (char *)struct_ptr;
                        if (strncmp(cpu_name, "cpu", 3) == 0) {
                            cpu_count++;
                        }
                        int cpu_name_len = strlen(cpu_name);
                        struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + cpu_name_len + 4) & ~3);
                    } else if (prop_token == FDT_END_NODE) {
                        continue;
                    } else if (prop_token == FDT_END) {
                        goto end_cpu_parse;
                    }
                }
            } else if (strncmp(node_name, "cpu", 3) == 0) {
                cpu_count++;
                /* Skip node name */
                struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + name_len + 4) & ~3);
            } else {
                /* Skip node name */
                struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + name_len + 4) & ~3);
            }
        } else if (token == FDT_END_NODE) {
            continue;
        } else if (token == FDT_PROP) {
            uint32_t len = be32_to_cpu(*struct_ptr++);
            struct_ptr++; /* skip nameoff */
            /* Skip property data */
            struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + len + 3) & ~3);
        } else if (token == FDT_END) {
            break;
        }
    }
    
end_cpu_parse:
    if (cpu_count > 0) {
        g_boot_info.cpu_count = cpu_count;
    }
    printf("CPU config: count=%d, timebase=%ld\n",
           g_boot_info.cpu_count, g_boot_info.timebase_freq);
}

/* Initialize boot information */
void boot_info_init(void) {
    /* Get hart ID and DTB address from assembly */
    g_boot_info.hartid = boot_hartid_asm;
    g_boot_info.dtb_addr = dtb_addr_asm;
    
    printf("Boot info: hartid=%ld, dtb=0x%lx\n", 
           g_boot_info.hartid, g_boot_info.dtb_addr);
    
    /* Check if we have a valid device tree */
    if (g_boot_info.dtb_addr && fdt_check_header((void *)g_boot_info.dtb_addr) == 0) {
        printf("Valid device tree found\n");
        parse_memory_info((void *)g_boot_info.dtb_addr);
        parse_cpu_info((void *)g_boot_info.dtb_addr);
    } else {
        printf("No valid device tree, using defaults\n");
        parse_memory_info(NULL);
        parse_cpu_info(NULL);
    }
    
    /* Initialize device cache */
    fdt_cache_init();
}

/* Initialize FDT cache - parse once, use many times */
void fdt_cache_init(void) {
    void *fdt = (void *)g_boot_info.dtb_addr;
    
    if (!fdt || fdt_check_header(fdt) != 0) {
        printf("No valid FDT, using default device config\n");
        /* Set default values */
        strcpy(g_fdt_cache.uart.name, "uart0");
        g_fdt_cache.uart.base_addr = 0x10000000;
        g_fdt_cache.uart.irq = 10;
        
        strcpy(g_fdt_cache.plic.name, "plic");
        g_fdt_cache.plic.base_addr = 0x0c000000;
        
        strcpy(g_fdt_cache.clint.name, "clint");
        g_fdt_cache.clint.base_addr = 0x02000000;
        return;
    }
    
    printf("Parsing device tree and caching device info...\n");
    
    /* Parse and cache device information */
    parse_uart_info(fdt);
    parse_plic_info(fdt);
    parse_clint_info(fdt);
    
    printf("Device tree cache initialized\n");
}

/* Parse UART information */
static void parse_uart_info(void *fdt) {
    int uart_node = fdt_find_node_by_compatible(fdt, "ns16550a");
    if (uart_node < 0) {
        uart_node = fdt_find_node_by_compatible(fdt, "sifive,uart0");
    }
    
    if (uart_node >= 0) {
        int len;
        uint32_t *reg = (uint32_t *)fdt_get_property(fdt, uart_node, "reg", &len);
        if (reg && len >= 4) {
            /* Handle both 32-bit and 64-bit address formats */
            if (len >= 8) {
                /* 64-bit address format: <addr_high addr_low size_high size_low> */
                g_fdt_cache.uart.base_addr = ((uint64_t)be32_to_cpu(reg[0]) << 32) | be32_to_cpu(reg[1]);
                g_fdt_cache.uart.size = ((uint64_t)be32_to_cpu(reg[2]) << 32) | be32_to_cpu(reg[3]);
            } else {
                /* 32-bit address format: <addr size> */
                g_fdt_cache.uart.base_addr = be32_to_cpu(reg[0]);
                g_fdt_cache.uart.size = be32_to_cpu(reg[1]);
            }
        } else {
            /* Fallback to default values */
            g_fdt_cache.uart.base_addr = 0x10000000;
            g_fdt_cache.uart.size = 0x1000;
        }
        
        /* Try to get interrupt information */
        uint32_t *interrupts = (uint32_t *)fdt_get_property(fdt, uart_node, "interrupts", &len);
        if (interrupts && len >= 4) {
            g_fdt_cache.uart.irq = be32_to_cpu(interrupts[0]);
        } else {
            g_fdt_cache.uart.irq = 10;  /* Default UART IRQ */
        }
        
        strcpy(g_fdt_cache.uart.name, "uart0");
        strcpy(g_fdt_cache.uart.compatible, "ns16550a");
    } else {
        /* No UART found, use defaults */
        strcpy(g_fdt_cache.uart.name, "uart0");
        g_fdt_cache.uart.base_addr = 0x10000000;
        g_fdt_cache.uart.size = 0x1000;
        g_fdt_cache.uart.irq = 10;
        strcpy(g_fdt_cache.uart.compatible, "ns16550a");
    }
    
    printf("UART: base=0x%lx, size=0x%lx, irq=%d\n",
           g_fdt_cache.uart.base_addr, g_fdt_cache.uart.size, g_fdt_cache.uart.irq);
}

/* Parse PLIC information */
static void parse_plic_info(void *fdt) {
    int plic_node = fdt_find_node_by_compatible(fdt, "riscv,plic0");
    if (plic_node < 0) {
        plic_node = fdt_find_node_by_compatible(fdt, "sifive,plic-1.0.0");
    }
    
    if (plic_node >= 0) {
        int len;
        uint32_t *reg = (uint32_t *)fdt_get_property(fdt, plic_node, "reg", &len);
        if (reg && len >= 4) {
            /* Handle both 32-bit and 64-bit address formats */
            if (len >= 8) {
                /* 64-bit address format */
                g_fdt_cache.plic.base_addr = ((uint64_t)be32_to_cpu(reg[0]) << 32) | be32_to_cpu(reg[1]);
                g_fdt_cache.plic.size = ((uint64_t)be32_to_cpu(reg[2]) << 32) | be32_to_cpu(reg[3]);
            } else {
                /* 32-bit address format */
                g_fdt_cache.plic.base_addr = be32_to_cpu(reg[0]);
                g_fdt_cache.plic.size = be32_to_cpu(reg[1]);
            }
        } else {
            /* Fallback to default values */
            g_fdt_cache.plic.base_addr = 0x0c000000;
            g_fdt_cache.plic.size = 0x4000000;
        }
        
        strcpy(g_fdt_cache.plic.name, "plic");
        strcpy(g_fdt_cache.plic.compatible, "riscv,plic0");
    } else {
        /* No PLIC found, use defaults */
        strcpy(g_fdt_cache.plic.name, "plic");
        g_fdt_cache.plic.base_addr = 0x0c000000;
        g_fdt_cache.plic.size = 0x4000000;
        strcpy(g_fdt_cache.plic.compatible, "riscv,plic0");
    }
    
    printf("PLIC: base=0x%lx, size=0x%lx\n",
           g_fdt_cache.plic.base_addr, g_fdt_cache.plic.size);
}

/* Parse CLINT information */
static void parse_clint_info(void *fdt) {
    int clint_node = fdt_find_node_by_compatible(fdt, "riscv,clint0");
    if (clint_node < 0) {
        clint_node = fdt_find_node_by_compatible(fdt, "sifive,clint0");
    }
    
    if (clint_node >= 0) {
        int len;
        uint32_t *reg = (uint32_t *)fdt_get_property(fdt, clint_node, "reg", &len);
        if (reg && len >= 4) {
            /* Handle both 32-bit and 64-bit address formats */
            if (len >= 8) {
                /* 64-bit address format */
                g_fdt_cache.clint.base_addr = ((uint64_t)be32_to_cpu(reg[0]) << 32) | be32_to_cpu(reg[1]);
                g_fdt_cache.clint.size = ((uint64_t)be32_to_cpu(reg[2]) << 32) | be32_to_cpu(reg[3]);
            } else {
                /* 32-bit address format */
                g_fdt_cache.clint.base_addr = be32_to_cpu(reg[0]);
                g_fdt_cache.clint.size = be32_to_cpu(reg[1]);
            }
        } else {
            /* Fallback to default values */
            g_fdt_cache.clint.base_addr = 0x02000000;
            g_fdt_cache.clint.size = 0x10000;
        }
        
        strcpy(g_fdt_cache.clint.name, "clint");
        strcpy(g_fdt_cache.clint.compatible, "riscv,clint0");
    } else {
        /* No CLINT found, use defaults */
        strcpy(g_fdt_cache.clint.name, "clint");
        g_fdt_cache.clint.base_addr = 0x02000000;
        g_fdt_cache.clint.size = 0x10000;
        strcpy(g_fdt_cache.clint.compatible, "riscv,clint0");
    }
    
    printf("CLINT: base=0x%lx, size=0x%lx\n",
           g_fdt_cache.clint.base_addr, g_fdt_cache.clint.size);
}

/* Device query functions - fast access to cached data */
uint64_t fdt_get_uart_base(void) {
    return g_fdt_cache.uart.base_addr;
}

uint32_t fdt_get_uart_irq(void) {
    return g_fdt_cache.uart.irq;
}

uint64_t fdt_get_plic_base(void) {
    return g_fdt_cache.plic.base_addr;
}

uint64_t fdt_get_clint_base(void) {
    return g_fdt_cache.clint.base_addr;
}

int fdt_get_cpu_count(void) {
    return g_boot_info.cpu_count;
}

uint64_t fdt_get_timebase_freq(void) {
    return g_boot_info.timebase_freq;
}

/* Find node by compatible string */
static int fdt_find_node_by_compatible(void *fdt, const char *compatible) {
    if (!fdt || !compatible) return -1;
    
    struct fdt_header *header = (struct fdt_header *)fdt;
    char *struct_start = (char *)fdt + be32_to_cpu(header->off_dt_struct);
    char *strings_start = (char *)fdt + be32_to_cpu(header->off_dt_strings);
    uint32_t *struct_ptr = (uint32_t *)struct_start;
    int node_offset = 0;
    
    while (1) {
        uint32_t token = be32_to_cpu(*struct_ptr++);
        
        if (token == FDT_BEGIN_NODE) {
            node_offset = (char *)struct_ptr - 4 - (char *)fdt;  /* Node offset */
            char *node_name = (char *)struct_ptr;
            int name_len = strlen(node_name);
            struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + name_len + 4) & ~3);
            
        } else if (token == FDT_PROP) {
            uint32_t len = be32_to_cpu(*struct_ptr++);
            uint32_t nameoff = be32_to_cpu(*struct_ptr++);
            char *prop_name = strings_start + nameoff;
            
            /* Check if this is compatible property */
            if (strcmp(prop_name, "compatible") == 0) {
                char *prop_value = (char *)struct_ptr;
                if (strstr(prop_value, compatible)) {
                    return node_offset;  /* Found matching node */
                }
            }
            
            /* Skip property data */
            struct_ptr = (uint32_t *)(((uintptr_t)struct_ptr + len + 3) & ~3);
            
        } else if (token == FDT_END_NODE) {
            continue;
        } else if (token == FDT_NOP) {
            continue;
        } else if (token == FDT_END) {
            break;
        }
    }
    
    return -1;  /* Not found */
}

/* Rust interface functions - provide access to cached FDT data */
uint64_t c_get_uart_base(void) {
    return g_fdt_cache.uart.base_addr;
}

uint64_t c_get_plic_base(void) {
    return g_fdt_cache.plic.base_addr;
}

uint64_t c_get_clint_base(void) {
    return g_fdt_cache.clint.base_addr;
}
