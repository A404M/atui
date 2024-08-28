#ifndef A404M_UI_TUI
#define A404M_UI_TUI 1

#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>

extern const int MAX_WIDTH;
extern const int MAX_HEIGHT;

extern const int MIN_WIDTH;
extern const int MIN_HEIGHT;

typedef enum MOUSE_BUTTON {
  MOUSE_BUTTON_LEFT_CLICK = 32,
  MOUSE_BUTTON_MIDDLE_CLICK = 33,
  MOUSE_BUTTON_RIGHT_CLICK = 34,
  MOUSE_BUTTON_SCROLL_UP = 96,
  MOUSE_BUTTON_SCROLL_DOWN = 97,
} MOUSE_BUTTON;

typedef struct MOUSE_ACTION {
  MOUSE_BUTTON button;
  unsigned int x;
  unsigned int y;
} MOUSE_ACTION;

typedef void (*ON_CLICK_CALLBACK)(const MOUSE_ACTION *mouse_action);

#ifndef __cplusplus
typedef enum bool : uint8_t { false = 0, true = 1 } bool;
#endif

typedef enum COLOR {
  COLOR_NO_COLOR = -1,
  COLOR_RESET = 0,
  COLOR_RED = 1,
  COLOR_GREEN = 2,
  COLOR_YELLOW = 3,
  COLOR_BLUE = 4,
  COLOR_MAGENTA = 5,
  COLOR_CYAN = 6,
  COLOR_WHITE = 7
} COLOR;

typedef struct TERMINAL_CELL {
  char c;
  COLOR color;
  COLOR background_color;
  ON_CLICK_CALLBACK on_click_callback;
} TERMINAL_CELL;

typedef struct TUI {
  struct winsize size;
  struct termios original, raw, helper;
  int init_cursor_x, init_cursor_y;
  TERMINAL_CELL *cells;
  size_t cells_length;
} TUI;

typedef enum WIDGET_TYPE {
  WIDGET_TYPE_TEXT,
  WIDGET_TYPE_BUTTON,
  WIDGET_TYPE_COLUMN,
  WIDGET_TYPE_ROW,
  WIDGET_TYPE_BOX,
} WIDGET_TYPE;

typedef struct WIDGET {
  WIDGET_TYPE type;
  void *metadata;
} WIDGET;

typedef struct WIDGET_ARRAY {
  size_t size;
  WIDGET **widgets;
} WIDGET_ARRAY;

typedef struct TEXT_METADATA {
  char *text;
  COLOR color;
} TEXT_METADATA;

typedef struct BUTTON_METADATA {
  WIDGET *child;
  ON_CLICK_CALLBACK callback;
} BUTTON_METADATA;

typedef struct COLUMN_METADATA {
  WIDGET_ARRAY *children;
} COLUMN_METADATA;

typedef struct ROW_METADATA {
  WIDGET_ARRAY *children;
} ROW_METADATA;

typedef struct BOX_METADATA {
  int width;
  int height;
  WIDGET *child;
  COLOR color;
} BOX_METADATA;

typedef WIDGET *(*WIDGET_BUILDER)(TUI *tui);

extern TUI *tui_init();
extern void tui_delete(TUI *restrict tui);
extern void tui_refresh(TUI *tui);

extern int tui_get_width(TUI *tui);
extern int tui_get_height(TUI *tui);

extern void tui_get_cursor_pos(TUI *tui, int *x, int *y);
extern int tui_move_to(int x, int y);

extern int tui_clear_screen();

extern void tui_start_app(TUI *tui, WIDGET_BUILDER widget_builder, int fps);
extern void _tui_draw_widget_to_cells(TUI *tui, const WIDGET *widget,
                                      int width_begin, int width_end,
                                      int height_begin, int height_end,
                                      int *child_width, int *childHeight);

extern bool widget_eqauls(const WIDGET *restrict left, const WIDGET *restrict right);
extern bool widget_array_eqauls(const WIDGET_ARRAY *restrict left, const WIDGET_ARRAY * restrict right);

extern void tui_main_loop(TUI *tui, WIDGET_BUILDER widget_builder, int fps);

extern WIDGET *tui_new_widget(WIDGET_TYPE type, void *metadata);
extern void tui_delete_widget(WIDGET *restrict widget);

extern WIDGET *tui_make_text(char *restrict text, COLOR color);
extern TEXT_METADATA *_tui_make_text_metadata(char *restrict text, COLOR color);
extern void _tui_delete_text(WIDGET *restrict text);

extern WIDGET *tui_make_button(WIDGET *restrict child,
                               ON_CLICK_CALLBACK callback);
extern BUTTON_METADATA *_tui_make_button_metadata(WIDGET *restrict child,
                                                  ON_CLICK_CALLBACK callback);
extern void _tui_delete_button(WIDGET *restrict button);

extern WIDGET *tui_make_column(WIDGET_ARRAY *restrict children);
extern COLUMN_METADATA *
_tui_make_column_metadata(WIDGET_ARRAY *restrict children);
extern void _tui_delete_column(WIDGET *restrict column);

extern WIDGET *tui_make_row(WIDGET_ARRAY *restrict children);
extern ROW_METADATA *_tui_make_row_metadata(WIDGET_ARRAY *restrict children);
extern void _tui_delete_row(WIDGET *restrict row);

extern WIDGET *tui_make_box(int width, int height, WIDGET *restrict child,
                            COLOR color);
extern BOX_METADATA *_tui_make_box_metadata(WIDGET *restrict child, int width,
                                            int height, COLOR color);
extern void _tui_delete_box(WIDGET *restrict box);

extern WIDGET_ARRAY *tui_make_widget_array_raw(size_t size, ...);
extern void _tui_delete_widget_array(WIDGET_ARRAY *restrict widget_array);

#define tui_make_widget_array(...) tui_make_widget_array_raw(sizeof((WIDGET* []) {__VA_ARGS__}) / sizeof(WIDGET*), __VA_ARGS__)

#endif
