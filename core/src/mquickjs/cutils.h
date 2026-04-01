/*
 * C utilities
 *
 * Copyright (c) 2017 Fabrice Bellard
 * Copyright (c) 2018 Charlie Gordon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef CUTILS_H
#define CUTILS_H

#include <stdlib.h>
#include <inttypes.h>

#ifdef __GNUC__
#define force_inline __attribute__((always_inline)) inline
#define no_inline __attribute__((noinline))
#define __maybe_unused __attribute__((unused))
#define __js_printf_like(a, b) __attribute__((format(printf, a, b)))
#else
#define force_inline inline
#define no_inline
#define __maybe_unused
#define __js_printf_like(a, b)
#endif

#define countof(x) (sizeof(x) / sizeof((x)[0]))

typedef int BOOL;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

static inline int max_int(int a, int b)
{
    if (a > b) return a; else return b;
}

static inline int min_int(int a, int b)
{
    if (a < b) return a; else return b;
}

static inline uint32_t max_uint32(uint32_t a, uint32_t b)
{
    if (a > b) return a; else return b;
}

static inline uint32_t min_uint32(uint32_t a, uint32_t b)
{
    if (a < b) return a; else return b;
}

static inline size_t max_size_t(size_t a, size_t b)
{
    if (a > b) return a; else return b;
}

static inline size_t min_size_t(size_t a, size_t b)
{
    if (a < b) return a; else return b;
}

static inline int64_t max_int64(int64_t a, int64_t b)
{
    if (a > b) return a; else return b;
}

static inline int64_t min_int64(int64_t a, int64_t b)
{
    if (a < b) return a; else return b;
}

/* XXX: should use Clang/GCC builtins */
static inline int clz32(uint32_t a)
{
    int r;
    if (a == 0) return 32;
    r = 0;
    if (a <= 0x0000FFFF) { r += 16; a <<= 16; }
    if (a <= 0x00FFFFFF) { r += 8; a <<= 8; }
    if (a <= 0x0FFFFFFF) { r += 4; a <<= 4; }
    if (a <= 0x3FFFFFFF) { r += 2; a <<= 2; }
    if (a <= 0x7FFFFFFF) { r += 1; a <<= 1; }
    return r;
}

static inline void put_u16(uint8_t *ptr, uint16_t v)
{
    ptr[0] = v;
    ptr[1] = v >> 8;
}

static inline uint16_t get_u16(const uint8_t *ptr)
{
    return ptr[0] | (ptr[1] << 8);
}

static inline void put_u32(uint8_t *ptr, uint32_t v)
{
    ptr[0] = v;
    ptr[1] = v >> 8;
    ptr[2] = v >> 16;
    ptr[3] = v >> 24;
}

static inline uint32_t get_u32(const uint8_t *ptr)
{
    return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}

static inline void put_u64(uint8_t *ptr, uint64_t v)
{
    put_u32(ptr, v);
    put_u32(ptr + 4, v >> 32);
}

static inline uint64_t get_u64(const uint8_t *ptr)
{
    return get_u32(ptr) | ((uint64_t)get_u32(ptr + 4) << 32);
}

static inline void put_be32(uint8_t *ptr, uint32_t v)
{
    ptr[0] = v >> 24;
    ptr[1] = v >> 16;
    ptr[2] = v >> 8;
    ptr[3] = v;
}

static inline uint32_t get_be32(const uint8_t *ptr)
{
    return (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
}

static inline int get_i8(const uint8_t *ptr)
{
    return (int8_t)*ptr;
}

static inline int get_i16(const uint8_t *ptr)
{
    return (int16_t)get_u16(ptr);
}

static inline int get_i32(const uint8_t *ptr)
{
    return (int32_t)get_u32(ptr);
}

#define UTF8_CHAR_LEN_MAX 6

static inline int unicode_to_utf8(uint8_t *buf, unsigned int c)
{
    if (c <= 0x7f) {
        buf[0] = c;
        return 1;
    } else if (c <= 0x7ff) {
        buf[0] = 0xc0 | (c >> 6);
        buf[1] = 0x80 | (c & 0x3f);
        return 2;
    } else if (c <= 0xffff) {
        buf[0] = 0xe0 | (c >> 12);
        buf[1] = 0x80 | ((c >> 6) & 0x3f);
        buf[2] = 0x80 | (c & 0x3f);
        return 3;
    } else if (c <= 0x10ffff) {
        buf[0] = 0xf0 | (c >> 18);
        buf[1] = 0x80 | ((c >> 12) & 0x3f);
        buf[2] = 0x80 | ((c >> 6) & 0x3f);
        buf[3] = 0x80 | (c & 0x3f);
        return 4;
    } else {
        return 0;
    }
}

static inline int unicode_from_utf8(const uint8_t *p, int max_len, size_t *p_len)
{
    int c, b, i, len;

    c = *p++;
    if (c < 0x80) {
        *p_len = 1;
        return c;
    } else if (c < 0xc0) {
        *p_len = 1;
        return -1;
    } else if (c < 0xe0) {
        len = 2;
        c &= 0x1f;
    } else if (c < 0xf0) {
        len = 3;
        c &= 0x0f;
    } else if (c < 0xf8) {
        len = 4;
        c &= 0x07;
    } else {
        *p_len = 1;
        return -1;
    }
    if (len > max_len) {
        *p_len = 1;
        return -1;
    }
    for(i = 1; i < len; i++) {
        b = *p++;
        if ((b & 0xc0) != 0x80) {
            *p_len = i;
            return -1;
        }
        c = (c << 6) | (b & 0x3f);
    }
    *p_len = len;
    return c;
}

static inline int utf8_get(const uint8_t *p, size_t *p_len)
{
    return unicode_from_utf8(p, UTF8_CHAR_LEN_MAX, p_len);
}

static inline int __utf8_get(const uint8_t *p, size_t *p_len)
{
    return unicode_from_utf8(p, UTF8_CHAR_LEN_MAX, p_len);
}

static inline int from_hex(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else
        return -1;
}

union float64_cast {
    double d;
    uint64_t u64;
};

static inline uint64_t float64_as_uint64(double d)
{
    union float64_cast u;
    u.d = d;
    return u.u64;
}

static inline double uint64_as_float64(uint64_t u64)
{
    union float64_cast u;
    u.u64 = u64;
    return u.d;
}

#endif /* CUTILS_H */
