#ifndef __STRING_H__
#define __STRING_H__

#include "kernel/types.h"

/* NULL definition */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* For extra safety, you could define a const null pointer */
#define CONST_NULL ((const void*)0)

/* Use size_t from kernel/types.h or system headers */
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
/* size_t should be defined in kernel/types.h or included from stddef.h */
#endif

/* Debug version that might help catch errors */
#ifdef DEBUG_NULL_ACCESS
#define DEBUG_NULL ((volatile void*)0)  /* volatile访问会更明显 */
#endif

/* Standard C library string functions */
int strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);
char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);
char *strstr(const char *haystack, const char *needle);
char *strdup(const char *s);

#endif /* __STRING_H__ */
