#ifndef DTOA_H
#define DTOA_H

#include "cutils.h"

#define JS_DTOA_FORMAT_FREE  0
#define JS_DTOA_FORMAT_FIXED 1
#define JS_DTOA_FORMAT_FRAC  2

#define JS_DTOA_EXP_ENABLED (1 << 3)
#define JS_DTOA_EXP_DISABLED (1 << 4)
#define JS_DTOA_MINUS_ZERO (1 << 5)

typedef struct JSDTOATempMem JSDTOATempMem;
typedef struct JSATODTempMem JSATODTempMem;

struct JSDTOATempMem {
    uint8_t buf[2048]; // Oversized for safety
};

struct JSATODTempMem {
    uint8_t buf[2048]; // Oversized for safety
};

int js_dtoa(char *buf, double d, int radix, int n_digits, int flags, JSDTOATempMem *mem);
int js_dtoa_max_len(double d, int radix, int n_digits, int flags);
double js_atod(const char *str, const char **pp, int radix, int flags, JSATODTempMem *mem);

#define JS_ATOD_ACCEPT_BIN_OCT (1 << 0)
#define JS_ATOD_ACCEPT_UNDERSCORES (1 << 1)
#define JS_ATOD_INT_ONLY (1 << 2)

#endif
