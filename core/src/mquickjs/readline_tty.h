#ifndef READLINE_TTY_H
#define READLINE_TTY_H

#include "mquickjs.h"

typedef struct ReadlineState {
    int term_width;
    uint8_t *term_cmd_buf;
    uint8_t *term_kill_buf;
    int term_cmd_buf_size;
    char *term_history;
    int term_history_buf_size;
    int (*get_color)(int *plen, const char *buf, int pos, int buf_len);
} ReadlineState;

int readline_tty_init(void);
const char *readline_tty(ReadlineState *s, const char *prompt, BOOL is_password);
int readline_is_interrupted(void);

#endif
