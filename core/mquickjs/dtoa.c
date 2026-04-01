#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "dtoa.h"
#include "cutils.h"

size_t i32toa(char *buf, int32_t v) {
    return sprintf(buf, "%d", v);
}
size_t i64toa(char *buf, int64_t v) {
    return sprintf(buf, "%" PRId64, v);
}
size_t u32toa(char *buf, uint32_t v) {
    return sprintf(buf, "%u", v);
}
size_t u64toa(char *buf, uint64_t v) {
    return sprintf(buf, "%" PRIu64, v);
}
size_t u64toa_radix(char *buf, uint64_t v, int radix) {
    if (radix == 16) return sprintf(buf, "%" PRIx64, v);
    return sprintf(buf, "%" PRIu64, v);
}

// Dummy dtoa/atod for now
int js_dtoa(char *buf, double d, int radix, int n_digits, int flags, JSDTOATempMem *mem) {
    return sprintf(buf, "%g", d);
}
int js_dtoa_max_len(double d, int radix, int n_digits, int flags) {
    return 128;
}
double js_atod(const char *str, const char **pp, int radix, int flags, JSATODTempMem *mem) {
    return strtod(str, (char**)pp);
}
