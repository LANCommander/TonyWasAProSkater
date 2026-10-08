/* Psy-Q libc entry points. Named hle_* because the host C library has the same names. Game code
 * calls these natively (cdecl), so varargs arrive as ordinary host varargs. */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc_hle.h"
#include "pc_runtime.h"

int hle_sprintf(char *buf, const char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsprintf(buf, fmt, ap);
    va_end(ap);
    return n;
}

int hle_printf(const char *fmt, ...) {
    va_list ap;
    int n = 0;
    if (pc_trace) {
        va_start(ap, fmt);
        n = vfprintf(stdout, fmt, ap);
        va_end(ap);
        fflush(stdout);
    }
    return n;
}

int hle_puts(const char *s) {
    if (pc_trace)
        puts(s);
    return 0;
}

char *hle_strcpy(char *d, const char *s) { return strcpy(d, s); }
int hle_strlen(const char *s) { return (int)strlen(s); }
int hle_strcmp(const char *a, const char *b) { return strcmp(a, b); }
int hle_strncmp(const char *a, const char *b, int n) { return strncmp(a, b, n); }
void *hle_memcpy(void *d, const void *s, int n) { return memcpy(d, s, n); }
void *hle_memset(void *d, int c, int n) { return memset(d, c, n); }
void *hle_memmove(void *d, const void *s, int n) { return memmove(d, s, n); }
void *hle_memchr(const void *s, int c, int n) { return memchr(s, c, n); }
void hle_bzero(void *d, int n) { memset(d, 0, n); }

uint32_t hle_malloc(uint32_t n) { return PC_Malloc(n); }
void hle_free(uint32_t p) { PC_Free(p); }
void hle_InitHeap(uint32_t addr, uint32_t size) { PC_HeapInit(addr, size); }

/* __builtin_delete (LIBSN): operator delete without a class allocator */
void hle___builtin_delete(uint32_t p) { PC_Free(p); }
