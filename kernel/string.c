#include "kernel/types.h"
#include <stddef.h>  /* For size_t */

#ifndef NULL
#define NULL ((void*)0)
#endif

/* External malloc/free declarations */
extern void *malloc(size_t size);
extern void free(void *ptr);

/* External printf for error reporting */
extern int printf(const char *format, ...);

/* Error handling policy for malloc failures:
 * - For string building functions (strcpy, strcat, etc.): Use fallback direct copy
 * - For functions that must allocate (strdup): Return NULL on failure
 * - Always ensure null termination even in fallback cases
 * - Print warning messages when fallback is used (helps debugging)
 */

/* Standard string functions with malloc-based safety for bare-metal environment */
/* Standard strlen */
int strlen(const char *s) {
    if (!s) return 0;
    
    int len = 0;
    while (*s++) len++;
    return len;
}

/* Standard strcmp */
int strcmp(const char *s1, const char *s2) {
    if (!s1 || !s2) return (s1 == s2) ? 0 : (s1 ? 1 : -1);
    
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

/* Standard strncmp */
int strncmp(const char *s1, const char *s2, size_t n) {
    if (!s1 || !s2 || n == 0) return 0;
    
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *s1 - *s2;
}

/* malloc-based strcpy for safety */
char *strcpy(char *dest, const char *src) {
    if (!dest || !src) return dest;
    
    size_t src_len = strlen(src);
    
    /* Use temporary buffer to build result safely */
    char *temp = malloc(src_len + 1);
    if (!temp) {
        /* Fallback: direct copy but ensure null termination */
        printf("WARNING: malloc failed in strcpy, using fallback (potential buffer overflow risk)\n");
        char *d = dest;
        while (*src) *d++ = *src++;
        *d = '\0';
        return dest;
    }
    
    /* Build in safe temporary buffer */
    char *t = temp;
    while (*src) *t++ = *src++;
    *t = '\0';
    
    /* Copy back to destination */
    char *d = dest;
    t = temp;
    while (*t) *d++ = *t++;
    *d = '\0';
    
    free(temp);
    return dest;
}

/* malloc-based strcat for safety */
char *strcat(char *dest, const char *src) {
    if (!dest || !src) return dest;
    
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    size_t total_len = dest_len + src_len;
    
    /* Use temporary buffer to build result safely */
    char *temp = malloc(total_len + 1);
    if (!temp) {
        /* Fallback: find end and append directly */
        printf("WARNING: malloc failed in strcat, using fallback (potential buffer overflow risk)\n");
        char *d = dest;
        while (*d) d++;
        while (*src) *d++ = *src++;
        *d = '\0';
        return dest;
    }
    
    /* Build complete string in safe temporary buffer */
    char *t = temp;
    char *d = dest;
    
    /* Copy existing dest content */
    while (*d) *t++ = *d++;
    
    /* Append src content */
    while (*src) *t++ = *src++;
    *t = '\0';
    
    /* Copy back to destination */
    d = dest;
    t = temp;
    while (*t) *d++ = *t++;
    *d = '\0';
    
    free(temp);
    return dest;
}

/* malloc-based strncat for safety */
char *strncat(char *dest, const char *src, size_t n) {
    if (!dest || !src || n == 0) return dest;
    
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    size_t copy_len = (n < src_len) ? n : src_len;
    size_t total_len = dest_len + copy_len;
    
    /* Use temporary buffer to build result safely */
    char *temp = malloc(total_len + 1);
    if (!temp) {
        /* Fallback: find end and append directly with bounds check */
        printf("WARNING: malloc failed in strncat, using fallback (potential buffer overflow risk)\n");
        char *d = dest;
        while (*d) d++;  /* Find end of dest */
        
        size_t count = 0;
        while (count < n && *src) {
            *d++ = *src++;
            count++;
        }
        *d = '\0';  /* Always null terminate */
        return dest;
    }
    
    /* Build complete string in safe temporary buffer */
    char *t = temp;
    char *d = dest;
    
    /* Copy existing dest content */
    while (*d) *t++ = *d++;
    
    /* Append up to n characters from src */
    size_t count = 0;
    while (count < n && *src) {
        *t++ = *src++;
        count++;
    }
    *t = '\0';
    
    /* Copy back to destination */
    d = dest;
    t = temp;
    while (*t) *d++ = *t++;
    *d = '\0';
    
    free(temp);
    return dest;
}

/* malloc-based strncpy for safety */
char *strncpy(char *dest, const char *src, size_t n) {
    if (!dest || !src) return dest;
    
    /* Use temporary buffer for safety */
    char *temp = malloc(n + 1);  /* Extra byte for safety */
    if (!temp) {
        /* Fallback: standard strncpy behavior */
        printf("WARNING: malloc failed in strncpy, using fallback (potential buffer overflow risk)\n");
        size_t i;
        for (i = 0; i < n && src[i] != '\0'; i++) {
            dest[i] = src[i];
        }
        /* Pad with null bytes if src is shorter than n */
        for (; i < n; i++) {
            dest[i] = '\0';
        }
        return dest;
    }
    
    /* Build in safe temporary buffer */
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        temp[i] = src[i];
    }
    /* Pad with null bytes if src is shorter than n */
    for (; i < n; i++) {
        temp[i] = '\0';
    }
    temp[n] = '\0';  /* Extra safety null terminator */
    
    /* Copy back to destination (only n bytes as per standard) */
    for (i = 0; i < n; i++) {
        dest[i] = temp[i];
    }
    
    free(temp);
    return dest;
}

/* Standard strchr */
char *strchr(const char *s, int c) {
    if (!s) return NULL;
    
    while (*s) {
        if (*s == c) return (char *)s;
        s++;
    }
    /* Check if we're looking for null terminator */
    if (c == '\0') return (char *)s;
    return NULL;
}

/* Standard strrchr */
char *strrchr(const char *s, int c) {
    if (!s) return NULL;
    
    const char *last = NULL;
    while (*s) {
        if (*s == c) last = s;
        s++;
    }
    /* Check if we're looking for null terminator */
    if (c == '\0') return (char *)s;
    return (char *)last;
}

/* Standard strstr */
char *strstr(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    if (*needle == '\0') return (char *)haystack;  /* Empty needle */
    
    size_t needle_len = strlen(needle);
    
    while (*haystack) {
        if (strncmp(haystack, needle, needle_len) == 0) {
            return (char *)haystack;
        }
        haystack++;
    }
    return NULL;
}

/* Standard strdup */
char *strdup(const char *s) {
    if (!s) return NULL;
    
    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (!copy) return NULL;
    
    strcpy(copy, s);
    return copy;
}

/* Safer strcat with destination buffer size checking */
char *strcat_s(char *dest, size_t dest_size, const char *src) {
    if (!dest || !src || dest_size == 0) return dest;
    
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    size_t total_len = dest_len + src_len;
    
    /* Check if destination buffer is large enough */
    if (total_len >= dest_size) {
        printf("ERROR: strcat_s buffer overflow risk - need %lu bytes, have %lu\n", 
               total_len + 1, dest_size);
        return dest;  /* Don't modify dest if not enough space */
    }
    
    /* Use temporary buffer to build result safely */
    char *temp = malloc(total_len + 1);
    if (!temp) {
        /* Even fallback should respect buffer size */
        printf("WARNING: malloc failed in strcat_s, using fallback with size check\n");
        char *d = dest + dest_len;  /* Find end of dest */
        size_t space_left = dest_size - dest_len - 1;  /* -1 for null terminator */
        
        while (space_left > 0 && *src) {
            *d++ = *src++;
            space_left--;
        }
        *d = '\0';
        return dest;
    }
    
    /* Build complete string in safe temporary buffer */
    char *t = temp;
    char *d = dest;
    
    /* Copy existing dest content */
    while (*d) *t++ = *d++;
    
    /* Append src content */
    while (*src) *t++ = *src++;
    *t = '\0';
    
    /* Copy back to destination */
    d = dest;
    t = temp;
    while (*t) *d++ = *t++;
    *d = '\0';
    
    free(temp);
    return dest;
}
