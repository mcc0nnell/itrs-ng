#include <stddef.h>

int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    for (size_t i = 0; i < n; ++i) d[i] = s[i];
    return dst;
}

void *memset(void *dst, int c, size_t n) {
    unsigned char *d = dst;
    for (size_t i = 0; i < n; ++i) d[i] = (unsigned char)c;
    return dst;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n] != '\0') ++n;
    return n;
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        unsigned char ac = (unsigned char)a[i];
        unsigned char bc = (unsigned char)b[i];
        if (ac != bc) return ac - bc;
        if (ac == 0) return 0;
    }
    return 0;
}

char *strchr(const char *s, int c) {
    char needle = (char)c;
    for (;;) {
        if (*s == needle) return (char *)s;
        if (*s == '\0') return NULL;
        ++s;
    }
}

static void byte_swap(unsigned char *a, unsigned char *b, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        unsigned char t = a[i];
        a[i] = b[i];
        b[i] = t;
    }
}

void qsort(void *base, size_t count, size_t size, int (*compare)(const void *, const void *)) {
    unsigned char *bytes = base;
    if (!base || !compare || size == 0 || count < 2) return;

    for (size_t i = 1; i < count; ++i) {
        size_t j = i;
        while (j > 0) {
            unsigned char *left = bytes + (j - 1) * size;
            unsigned char *right = bytes + j * size;
            if (compare(left, right) <= 0) break;
            byte_swap(left, right, size);
            --j;
        }
    }
}
