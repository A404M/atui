#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "ui/tui.h"

bool is_clicked = false;

void on_button_click(const MOUSE_ACTION *mouse_action) {
  is_clicked = !is_clicked;
}

WIDGET *ui_build(TUI *tui) {
  if (is_clicked) {
    char frame[20+3+1];
    const uint64_t fps = 1000000000/tui->last_frame;
    sprintf(frame, "%ldfps", fps);
    return tui_make_box(
        -1, -1,
        tui_make_column(tui_make_widget_array(
            tui_make_box(0, 12, NULL, COLOR_NO_COLOR),
            tui_make_row(tui_make_widget_array(
                tui_make_box(50, 0, NULL, COLOR_NO_COLOR),
                tui_make_box(
                    20, 3,
                    tui_make_column(tui_make_widget_array(
                        tui_make_text(frame, COLOR_BLUE),
                        tui_make_button(tui_make_text("       Back", COLOR_RED),
                                        on_button_click))),
                    COLOR_WHITE))))),
        COLOR_MAGENTA);
  } else {
    return tui_make_box(
        -1, -1,
        tui_make_column(tui_make_widget_array(
            tui_make_box(0, 12, NULL, COLOR_NO_COLOR),
            tui_make_row(tui_make_widget_array(
                tui_make_box(50, 0, NULL, COLOR_NO_COLOR),
                tui_make_button(
                    tui_make_box(MIN_WIDTH, MIN_HEIGHT,
                                 tui_make_text("\nClick here\n", COLOR_BLUE),
                                 COLOR_WHITE),
                    on_button_click))))),
        COLOR_MAGENTA);
  }
}

int main() {
  TUI *tui = tui_init();

  tui_start_app(tui, ui_build, 144);

  tui_delete(tui);

  return 0;
}
