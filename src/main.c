#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "ui/color.h"
#include "ui/tui.h"

constexpr COLOR BACKGROUND_COLOR = color_hex(0xFF2B2A33);
constexpr COLOR COLOR_FPS_BACK = color_hex(0xFFCCCCCC);
constexpr COLOR COLOR_FPS_TEXT = color_hex(0xFF000000);

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

// WIDGET *search_header() {}

WIDGET *fps_counter(uint64_t last_frame) {
  char fps_text[3 + 1 + 20 + 1];
  sprintf(fps_text, "fps %ld",
          (last_frame == 0) ? 0 : (NANO_TO_SECOND / last_frame));
  return tui_make_padding(
      tui_make_box(MIN_WIDTH, 1, tui_make_text(fps_text, COLOR_FPS_TEXT),
                   COLOR_FPS_BACK),
      0, 0, 2, 2);
}

WIDGET *ui_build(TUI *tui) {
  return tui_make_box(
      MAX_WIDTH, MAX_HEIGHT,
      tui_make_column(tui_make_widget_array(
          fps_counter(tui->last_frame), search_box(),
          tui_make_row(tui_make_widget_array(tui_make_center(tui_make_box(
              MIN_WIDTH, MIN_HEIGHT,
              tui_make_padding(tui_make_text("Hi here", color_init(0xFF0000FF)),
                               2, 2, 2, 2),
              color_init(0xFFFFFFFF))))))),
      BACKGROUND_COLOR);
}

int main() {
  TUI *tui = tui_init();

  tui_start_app(tui, ui_build, 144);

  tui_delete(tui);

  return 0;
}
