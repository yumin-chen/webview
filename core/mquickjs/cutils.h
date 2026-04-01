#ifndef CUTILS_H
#define CUTILS_H

#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#define force_inline __attribute__((always_inline)) inline
#define no_inline __attribute__((noinline))
#define __maybe_unused __attribute__((unused))
#define __js_printf_like(f, a) __attribute__((format(printf, f, a)))

#define countof(x) (sizeof(x) / sizeof((x)[0]))

typedef int BOOL;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static inline int max_int(int a, int b) { return a > b ? a : b; }
static inline int min_int(int a, int b) { return a < b ? a : b; }
static inline uint32_t max_uint32(uint32_t a, uint32_t b) { return a > b ? a : b; }
static inline uint32_t min_uint32(uint32_t a, uint32_t b) { return a < b ? a : b; }
static inline size_t max_size_t(size_t a, size_t b) { return a > b ? a : b; }
static inline size_t min_size_t(size_t a, size_t b) { return a < b ? a : b; }

static inline uint16_t get_u16(const uint8_t *ptr) {
    return ptr[0] | (ptr[1] << 8);
}

static inline void put_u16(uint8_t *ptr, uint16_t v) {
    ptr[0] = v & 0xff;
    ptr[1] = (v >> 8) & 0xff;
}

static inline uint32_t get_u32(const uint8_t *ptr) {
    return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}

static inline void put_u32(uint8_t *ptr, uint32_t v) {
    ptr[0] = v & 0xff;
    ptr[1] = (v >> 8) & 0xff;
    ptr[2] = (v >> 16) & 0xff;
    ptr[3] = (v >> 24) & 0xff;
}

static inline uint32_t get_be32(const uint8_t *ptr) {
    return (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
}

static inline void put_be32(uint8_t *ptr, uint32_t v) {
    ptr[0] = (v >> 24) & 0xff;
    ptr[1] = (v >> 16) & 0xff;
    ptr[2] = (v >> 8) & 0xff;
    ptr[3] = v & 0xff;
}

static inline uint64_t get_u64(const uint8_t *ptr) {
    return get_u32(ptr) | ((uint64_t)get_u32(ptr + 4) << 32);
}

static inline void put_u64(uint8_t *ptr, uint64_t v) {
    put_u32(ptr, v & 0xffffffff);
    put_u32(ptr + 4, v >> 32);
}

static inline int8_t get_i8(const uint8_t *ptr) { return (int8_t)*ptr; }
static inline int16_t get_i16(const uint8_t *ptr) { return (int16_t)get_u16(ptr); }
static inline int32_t get_i32(const uint8_t *ptr) { return (int32_t)get_u32(ptr); }

static inline int from_hex(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static inline int clz32(uint32_t a) {
    if (a == 0) return 32;
    return __builtin_clz(a);
}

static inline uint64_t float64_as_uint64(double d) {
    union { double d; uint64_t u; } u;
    u.d = d;
    return u.u;
}

static inline double uint64_as_float64(uint64_t u) {
    union { double d; uint64_t u; } un;
    un.u = u;
    return un.d;
}

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

// Minimal replacement for missing dtoa functions if needed
size_t i32toa(char *buf, int32_t v);
size_t i64toa(char *buf, int64_t v);
size_t u32toa(char *buf, uint32_t v);
size_t u64toa(char *buf, uint64_t v);
size_t u64toa_radix(char *buf, uint64_t v, int radix);

#endif
