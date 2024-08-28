#include "ui/tui.h"
#include <stdio.h>
#include <unistd.h>

void on_button_click(MOUSE_ACTION mouse_action) {
  printf("hello");
  sleep(1);
}

WIDGET *ui_build(TUI *tui) {
  return tui_make_box(
      -1, -1,
      tui_make_row(tui_make_widget_array(
          3,
          tui_make_box(
              8, 2,
              tui_make_button(tui_make_text("Hello, World!", COLOR_BLUE),
                              on_button_click),
              COLOR_YELLOW),
          tui_make_box(
              8, 2,
              tui_make_button(tui_make_text("Hello, World!", COLOR_RED),
                              on_button_click),
              COLOR_BLUE),
          tui_make_text("Hello, World!", COLOR_RED), 1)),
      COLOR_MAGENTA);
}

int main() {
  TUI *tui = tui_init();

  tui_start_app(tui, ui_build);

  tui_delete(tui);

  return 0;
}
