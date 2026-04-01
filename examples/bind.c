#include "alloy/api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

typedef struct {
  alloy_component_t win;
  alloy_signal_t count_signal;
  int count;
} context_t;

void on_click(alloy_component_t handle, alloy_event_type_t ev, void *ud) {
  context_t *ctx = (context_t *)ud;
  ctx->count++;
  alloy_signal_set_int(ctx->count_signal, ctx->count);
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine,
                   int nCmdShow) {
#else
int main(void) {
#endif
  // Dual-engine Architecture: Secure host process with reactive signals
  alloy_component_t win = alloy_create_window("Alloy bind.c (Dual Engine)", 800, 600);

  context_t ctx;
  ctx.win = win;
  ctx.count = 0;
  ctx.count_signal = alloy_signal_create_int(0);

  alloy_component_t vstack = alloy_create_vstack(win);

  alloy_component_t lbl = alloy_create_label(vstack);
  alloy_bind_property(lbl, ALLOY_PROP_TEXT, ctx.count_signal);

  alloy_component_t btn = alloy_create_button(vstack);
  alloy_set_text(btn, "Increment");
  alloy_set_event_callback(btn, ALLOY_EVENT_CLICK, on_click, &ctx);

  alloy_add_child(win, vstack);

  printf("Alloy bind.c example started (Secure C host).\n");

  alloy_run(win);

  alloy_signal_destroy(ctx.count_signal);
  alloy_destroy(win);
  return 0;
}
