#ifndef __TYPES_H__
#define __TYPES_H__

#include <stdint.h>  // 使用标准的类型定义
#include <stddef.h>  // 包含size_t等

// 使用标准类型，避免重复定义
// typedef unsigned char uint8_t;     // 已在stdint.h中定义
// typedef unsigned short uint16_t;   // 已在stdint.h中定义
// typedef unsigned int  uint32_t;    // 已在stdint.h中定义
// typedef unsigned long long uint64_t; // 已在stdint.h中定义
// typedef uint64_t uintptr_t;        // 已在stdint.h中定义

/*
 * RISCV64: register is 64bits width
 */ 
typedef uint64_t reg_t;

#endif /* __TYPES_H__ */
