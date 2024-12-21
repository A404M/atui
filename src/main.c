#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "ui/color.h"
#include "ui/tui.h"

WIDGET *search_box() {
  return tui_make_padding(
      tui_make_box(MAX_WIDTH, 1,
                   tui_make_center(tui_make_row(tui_make_widget_array(
                       tui_make_box(MAX_WIDTH, 1, NULL, color_init(0xFF42414D)),
                       tui_make_box(10, 1,
                                    tui_make_center(tui_make_text(
                                        "Search", color_init(0xFF000000))),
                                    color_init(0xFFC4C3C9))))),
                   COLOR_NO_COLOR),
      1, 1, 10, 10);
}

WIDGET *search_header() {}

WIDGET *ui_build(TUI *tui) {
  return tui_make_box(
      MAX_WIDTH, MAX_HEIGHT,
      tui_make_column(tui_make_widget_array(
          search_box(),
          tui_make_row(tui_make_widget_array(tui_make_center(
              tui_make_box(MIN_WIDTH, 3,
                           tui_make_center(tui_make_text(
                               "Hi here", color_init(0xFF0000FF))),
                           color_init(0xFFFFFFFF))))))),
      color_init(0xFF2B2A33));
}

int main() {
  TUI *tui = tui_init();

  tui_start_app(tui, ui_build, 144);

  tui_delete(tui);

  return 0;
}
