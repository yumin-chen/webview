/*
 * Double to ASCII conversion
 *
 * Copyright (c) 2017 Fabrice Bellard
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
#ifndef DTOA_H
#define DTOA_H

#include <stdint.h>

#define JS_DTOA_FORMAT_FREE 0
#define JS_DTOA_FORMAT_FRAC 1
#define JS_DTOA_FORMAT_FIXED 2

#define JS_DTOA_EXP_ENABLED (1 << 3)
#define JS_DTOA_EXP_DISABLED (1 << 4)
#define JS_DTOA_MINUS_ZERO (1 << 5)

typedef struct {
    double d;
} JSDTOATempMem;

typedef struct {
    double d;
} JSATODTempMem;

int js_dtoa(char *buf, double d, int radix, int n_digits, int flags, JSDTOATempMem *mem);
int js_dtoa_max_len(double d, int radix, int n_digits, int flags);
double js_atod(const char *str, const char **pp, int radix, int flags, JSATODTempMem *mem);

#define JS_ATOD_ACCEPT_BIN_OCT (1 << 0)
#define JS_ATOD_ACCEPT_UNDERSCORES (1 << 1)
#define JS_ATOD_INT_ONLY (1 << 2)

#endif /* DTOA_H */
