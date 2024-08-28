#include "tui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

const int MAX_WIDTH = -1;
const int MAX_HEIGHT = -1;

const int MIN_WIDTH = -2;
const int MIN_HEIGHT = -2;

void _tui_clear_cells(TUI *tui) {
  const TERMINAL_CELL empty = {.c = ' ',
                               .color = COLOR_NO_COLOR,
                               .background_color = COLOR_NO_COLOR,
                               .on_click_callback = NULL};
  for (size_t i = 0; i < tui->cells_length; ++i) {
    tui->cells[i] = empty;
  }
}

void _tui_init_cells(TUI *tui) {
  tui->cells_length = tui_get_width(tui) * tui_get_height(tui);
  tui->cells = malloc(tui->cells_length * sizeof(TERMINAL_CELL));
  _tui_clear_cells(tui);
}

void _tui_delete_cells(TUI *tui) {
  tui->cells_length = 0;
  free(tui->cells);
  tui->cells = NULL;
}

TUI *tui_init() {
  setbuf(stdout, NULL);

  TUI *tui = malloc(sizeof(TUI));

  tui_get_cursor_pos(tui, &tui->init_cursor_x, &tui->init_cursor_y);

  // Save original serial communication configuration for stdin
  tcgetattr(STDIN_FILENO, &tui->original);

  // Put stdin in raw mode so keys get through directly without
  // requiring pressing enter.
  cfmakeraw(&tui->raw);
  tcsetattr(STDIN_FILENO, TCSANOW, &tui->raw);

  // Switch to the alternate buffer screen
  write(STDOUT_FILENO, "\e[?47h", 6);

  // Enable mouse tracking
  write(STDOUT_FILENO, "\e[?9h", 5);

  _tui_init_cells(tui);

  tui_refresh(tui);
  return tui;
}

void tui_delete(TUI *restrict tui) {
  // Revert the terminal back to its original state
  write(STDOUT_FILENO, "\e[?9l", 5);
  write(STDOUT_FILENO, "\e[?47l", 6);
  tcsetattr(STDIN_FILENO, TCSANOW, &tui->original);

  tui_move_to(tui->init_cursor_x, tui->init_cursor_y);

  _tui_delete_cells(tui);
  free(tui);
}

void tui_refresh(TUI *tui) {
  const int width = tui_get_width(tui);
  const int height = tui_get_height(tui);

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &tui->size);

  if (width != tui_get_width(tui) || height != tui_get_height(tui)) {
    _tui_delete_cells(tui);
    _tui_init_cells(tui);
  }
}

int tui_get_width(TUI *tui) { return tui->size.ws_col; }

int tui_get_height(TUI *tui) { return tui->size.ws_row; }

int tui_clear_screen() { return printf("\033[2J\r"); }

int tui_move_top(int n) { return printf("\033[%dA", n); }

int tui_move_up(int n) { return printf("\033[%dB", n); }

int tui_move_right(int n) { return printf("\033[%dC", n); }

int tui_move_left(int n) { return printf("\033[%dD", n); }

int tui_save_cursor() { return printf("\0337"); }

int tui_restore_cursor() { return printf("\0338"); }

void tui_get_cursor_pos(TUI *tui, int *x, int *y) {
  char buf[8];
  char cmd[] = "\033[6n";
  tcgetattr(0, &tui->raw);
  cfmakeraw(&tui->helper);
  tcsetattr(0, TCSANOW, &tui->helper);
  if (isatty(fileno(stdin))) {
    write(STDOUT_FILENO, cmd, sizeof(cmd));
    read(STDIN_FILENO, buf, sizeof(buf));

    sscanf(buf, "\033[%d;%dR", y, x);
    --*x;
    --*y;
  }
  tcsetattr(0, TCSANOW, &tui->raw);
}

int tui_move_to(int x, int y) { return printf("\033[%d;%dH", y + 1, x + 1); }

int tui_delete_before() { return printf("\b \b"); }

int tui_delete_under_cursor() { return printf(" \b"); }

int _tui_get_cell_index(TUI *tui, int x, int y) {
  const int width = tui_get_width(tui);
  return x + width * y;
}

void _tui_set_cell_char(TUI *tui, int x, int y, char c) {
  tui->cells[_tui_get_cell_index(tui, x, y)].c = c;
}

void _tui_set_cell_color(TUI *tui, int x, int y, COLOR color) {
  if (color == COLOR_NO_COLOR) {
    return;
  }
  tui->cells[_tui_get_cell_index(tui, x, y)].color = color;
}

void _tui_set_cell_background_color(TUI *tui, int x, int y,
                                    COLOR background_color) {
  if (background_color == COLOR_NO_COLOR) {
    return;
  }
  tui->cells[_tui_get_cell_index(tui, x, y)].background_color =
      background_color;
}

void _tui_set_cell_on_click_callback(TUI *tui, int x, int y,
                                     ON_CLICK_CALLBACK on_click_callback) {
  tui->cells[_tui_get_cell_index(tui, x, y)].on_click_callback =
      on_click_callback;
}

void tui_handle_mouse_action(TUI *tui, const MOUSE_ACTION *mouse_action) {
  const ON_CLICK_CALLBACK callback =
      tui->cells[_tui_get_cell_index(tui, mouse_action->x, mouse_action->y)]
          .on_click_callback;
  if (callback != NULL) {
    callback(mouse_action);
  }
}

int tui_change_terminal_text_color(COLOR color) {
  if (color == COLOR_NO_COLOR) {
    return 0;
  } else if (color == COLOR_RESET) {
    return printf("\033[%dm", COLOR_RESET);
  }
  return printf("\033[%dm", color + 30);
}

int tui_change_terminal_background_color(COLOR color) {
  if (color == COLOR_NO_COLOR) {
    return 0;
  } else if (color == COLOR_RESET) {
    return printf("\033[%dm", COLOR_RESET);
  }
  return printf("\033[%dm", color + 40);
}

bool handle_input(TUI *tui) {
  unsigned char buff[6];
  read(STDIN_FILENO, &buff, 1);
  if (buff[0] == 3) { // User pressd Ctr+C
    return true;
  } else if (buff[0] == '\x1B') { // [ESCAPE]
    // TODO: fix for inputting actual <ESC>
    read(STDIN_FILENO, &buff, 5);
    switch (buff[1]) {
    case 77: {
      const MOUSE_ACTION mouse_action = {
          .button = buff[2],
          .x = buff[3] - 32 - 1, // starts at 0
          .y = buff[4] - 32 - 1, // starts at 0
      };
      tui_handle_mouse_action(tui, &mouse_action);
      /*printf("button:%u\n\rx:%u\n\ry:%u\n\n\r", mouse_action.button,*/
      /*       mouse_action.x, mouse_action.y);*/
    } break;
    }
  } else {
    switch (buff[0]) {
    case 'h':
      tui_move_left(1);
      break;
    case 'j':
      tui_move_up(1);
      break;
    case 'k':
      tui_move_top(1);
      break;
    case 'l':
      tui_move_right(1);
      break;
    case 'q':
      return true;
    case '\r': { // <ENTER>
      int x, y;
      tui_get_cursor_pos(tui, &x, &y);
      const MOUSE_ACTION mouse_action = {
          .button = MOUSE_BUTTON_LEFT_CLICK,
          .x = x,
          .y = y,
      };
      tui_handle_mouse_action(tui, &mouse_action);
    } break;
    case '\b':
    case 127: // back space
      tui_delete_before();
      break;
    default:
      /*printf("unknown:%c,%d\n\r", buff[0], buff[0]);*/
      /*sleep(1);*/
      break;
    }
  }
  return false;
}

void tui_start_app(TUI *tui, WIDGET_BUILDER widget_builder, int fps) {
  tui_main_loop(tui, widget_builder, fps);
}

void _tui_draw_widget_to_cells(TUI *tui, const WIDGET *widget, int width_begin,
                               int width_end, int height_begin, int height_end,
                               int *child_width, int *child_height) {
  switch (widget->type) {
  case WIDGET_TYPE_TEXT: {
    const TEXT_METADATA *metadata = widget->metadata;
    const int width_diff = width_end - width_begin;
    const size_t text_len = strlen(metadata->text);
    size_t inserted_index = 0;
    int height = height_begin;
    for (; height < height_end; ++height) {
      for (int j = 0; j < width_diff; ++j) {
        const int x = width_begin + j;
        const int y = height;
        _tui_set_cell_color(tui, x, y, metadata->color);
        if (inserted_index < text_len) {
          const char c = metadata->text[inserted_index];
          inserted_index += 1;
          if (c == '\n') {
            break;
          } else {
            _tui_set_cell_char(tui, x, y, c);
          }
          if (inserted_index == text_len) {
            goto END_OF_TEXT;
          }
        }
      }
    }
  END_OF_TEXT:
    *child_height = height + 1;
    *child_width =
        (text_len > width_diff ? width_end : text_len + width_begin) + 1;
  } break;
  case WIDGET_TYPE_BUTTON: {
    const BUTTON_METADATA *metadata = widget->metadata;
    if (metadata->child != NULL) {
      _tui_draw_widget_to_cells(tui, metadata->child, width_begin, width_end,
                                height_begin, height_end, child_width,
                                child_height);
      for (int i = width_begin; i < *child_width; ++i) {
        for (int j = height_begin; j < *child_height; ++j) {
          _tui_set_cell_on_click_callback(tui, i, j, metadata->callback);
        }
      }
    }
  } break;
  case WIDGET_TYPE_COLUMN: {
    const COLUMN_METADATA *metadata = widget->metadata;
    *child_width = width_begin;
    *child_height = height_begin;
    for (size_t i = 0; i < metadata->children->size; ++i) {
      const WIDGET *child = metadata->children->widgets[i];
      int width_temp;
      _tui_draw_widget_to_cells(tui, child, width_begin, width_end,
                                *child_height, height_end, &width_temp,
                                child_height);
      if (width_temp > *child_width) {
        *child_width = width_temp;
      }
    }
  } break;
  case WIDGET_TYPE_ROW: {
    const ROW_METADATA *metadata = widget->metadata;
    *child_width = width_begin;
    *child_height = height_begin;
    for (size_t i = 0; i < metadata->children->size; ++i) {
      const WIDGET *child = metadata->children->widgets[i];
      int height_temp;
      _tui_draw_widget_to_cells(tui, child, *child_width, width_end,
                                height_begin, height_end, child_width,
                                &height_temp);
      if (height_temp > *child_height) {
        *child_height = height_temp;
      }
    }
  } break;
  case WIDGET_TYPE_BOX: {
    const BOX_METADATA *metadata = widget->metadata;

    if (metadata->width != MAX_WIDTH) {
      width_end = metadata->width + width_begin >= width_end
                      ? width_end
                      : metadata->width + width_begin;
    }
    if (metadata->height != MAX_HEIGHT) {
      height_end = metadata->height + height_begin >= height_end
                       ? height_end
                       : metadata->height + height_begin;
    }

    for (int y = height_begin; y < height_end; ++y) {
      for (int x = width_begin; x < width_end; ++x) {
        _tui_set_cell_background_color(tui, x, y, metadata->color);
      }
    }

    if (metadata->child != NULL) {
      int t0, t1;
      _tui_draw_widget_to_cells(tui, metadata->child, width_begin, width_end,
                                height_begin, height_end, &t0, &t1);
    }
    *child_width = width_end;
    *child_height = height_end;
  } break;
  default:
    fprintf(stderr, "widget type '%d' went wrong in _tui_draw_widget",
            widget->type);
    exit(1);
  }
}

int _tui_move_to_in_str(char *str, int x, int y) {
  return sprintf(str, "\033[%d;%dH", y + 1, x + 1);
}

int _tui_get_background_color_ascii(COLOR color) {
  if (color == COLOR_NO_COLOR) {
    return 0;
  } else if (color == COLOR_RESET) {
    return printf("\033[%dm", COLOR_RESET);
  }
  return printf("\033[%dm", color + 40);
}

void _tui_draw_cells_to_terminal(TUI *tui) {
  const size_t size_of_cell = 5 + 5 + sizeof(char) + 5;
  const size_t size = tui->cells_length * size_of_cell;
  char str[(size + 2) * sizeof(char) + 1];

  _tui_move_to_in_str(str, 0, 0);

  char cell_str[5];

  COLOR last_color = COLOR_NO_COLOR;
  COLOR last_background_color = COLOR_NO_COLOR;

  for (size_t i = 0; i < tui->cells_length; ++i) {
    const TERMINAL_CELL cell = tui->cells[i];

    if (last_color != cell.color ||
        last_background_color != cell.background_color) {
      sprintf(cell_str, "\033[%dm", COLOR_RESET);
      strcat(str, cell_str);
      last_color = cell.color;
      last_background_color = cell.background_color;
      if (cell.color == COLOR_RESET || cell.color == COLOR_NO_COLOR) {
        sprintf(cell_str, "\033[%dm", COLOR_RESET);
      } else {
        sprintf(cell_str, "\033[%dm", cell.color + 30);
      }
      strcat(str, cell_str);

      if (cell.background_color == COLOR_RESET ||
          cell.background_color == COLOR_NO_COLOR) {
        sprintf(cell_str, "\033[%dm", COLOR_RESET);
      } else {
        sprintf(cell_str, "\033[%dm", cell.background_color + 40);
      }
      strcat(str, cell_str);
    }
    strncat(str, &cell.c, 1);
  }
  const int len = strlen(str);

  write(STDOUT_FILENO, str, len);
}

int kbhit() {
  struct timeval tv = {0L, 0L};
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(0, &fds);
  return select(1, &fds, NULL, NULL, &tv) > 0;
}

bool widget_array_eqauls(const WIDGET_ARRAY *restrict left, const WIDGET_ARRAY *restrict right) {
  if (left->size != right->size) {
    return false;
  }
  for (size_t i = 0; i < left->size; ++i) {
    if (!widget_eqauls(left->widgets[i], right->widgets[i])) {
      return false;
    }
  }
  return true;
}

bool widget_eqauls(const WIDGET *restrict left, const WIDGET *restrict right) {
  if (left == NULL || right == NULL) {
    return left == NULL && right == NULL;
  }
  if (left->type != right->type) {
    return false;
  }

  switch (left->type) {
  case WIDGET_TYPE_TEXT: {
    const TEXT_METADATA *left_data = left->metadata;
    const TEXT_METADATA *right_data = right->metadata;
    return left_data->color == right_data->color &&
           strcmp(left_data->text, right_data->text) == 0;
  } break;
  case WIDGET_TYPE_BUTTON: {
    const BUTTON_METADATA *left_data = left->metadata;
    const BUTTON_METADATA *right_data = right->metadata;
    return left_data->callback == right_data->callback &&
           widget_eqauls(left_data->child, right_data->child);
  } break;
  case WIDGET_TYPE_COLUMN: {
    const COLUMN_METADATA *left_data = left->metadata;
    const COLUMN_METADATA *right_data = right->metadata;
    return widget_array_eqauls(left_data->children, right_data->children);
  } break;
  case WIDGET_TYPE_ROW: {
    const ROW_METADATA *left_data = left->metadata;
    const ROW_METADATA *right_data = right->metadata;
    return widget_array_eqauls(left_data->children, right_data->children);
  } break;
  case WIDGET_TYPE_BOX: {
    const BOX_METADATA *left_data = left->metadata;
    const BOX_METADATA *right_data = right->metadata;
    return left_data->width == right_data->width &&
           left_data->height == right_data->height &&
           left_data->color == right_data->color &&
           widget_eqauls(left_data->child, right_data->child);
  } break;
  default:
    fprintf(stderr, "Type error '%d' in tui_delete_widget\n", left->type);
    exit(1);
  }

  return true;
}

const int NANO_TO_SECOND = 1000000000;

void nano_sleep(long int nano_seconds) {
  struct timespec remaining,
      request = {nano_seconds / NANO_TO_SECOND, nano_seconds % NANO_TO_SECOND};
  nanosleep(&request, &remaining);
}

long int nano_time() {
  struct timespec t = {0, 0}, tend = {0, 0};
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * NANO_TO_SECOND + t.tv_nsec;
}

void tui_main_loop(TUI *tui, WIDGET_BUILDER widget_builder, int fps) {
  const long int frame_nano = NANO_TO_SECOND / fps;
  while (1) {
    const long int start = nano_time();
    tui_save_cursor();
    tui_refresh(tui);
    WIDGET *root_widget = widget_builder(tui);
    _tui_clear_cells(tui);

    int width, height;
    _tui_draw_widget_to_cells(tui, root_widget, 0, tui_get_width(tui), 0,
                              tui_get_height(tui), &width, &height);

    _tui_draw_cells_to_terminal(tui);
    tui_delete_widget(root_widget);
    /*tui_move_to(0, 0);*/
    /*printf("%ld\t%ld", last_frame_time, frame_nano);*/
    tui_restore_cursor();
    if (fps != -1) {
      const long int diff = nano_time() - start;
      nano_sleep(frame_nano-diff);
    }
    if (kbhit()) {
      if (handle_input(tui)) {
        return;
      }
    }
  }
}

WIDGET *tui_new_widget(WIDGET_TYPE type, void *metadata) {
  WIDGET *widget = malloc(sizeof(WIDGET));
  widget->type = type;
  widget->metadata = metadata;
  return widget;
}

void tui_delete_widget(WIDGET *restrict widget) {
  if (widget == NULL) {
    return;
  }
  switch (widget->type) {
  case WIDGET_TYPE_TEXT:
    _tui_delete_text(widget);
    break;
  case WIDGET_TYPE_BUTTON:
    _tui_delete_button(widget);
    break;
  case WIDGET_TYPE_COLUMN:
    _tui_delete_column(widget);
    break;
  case WIDGET_TYPE_ROW:
    _tui_delete_row(widget);
    break;
  case WIDGET_TYPE_BOX:
    _tui_delete_box(widget);
    break;
  default:
    fprintf(stderr, "Type error '%d' in tui_delete_widget\n", widget->type);
    exit(1);
  }
  free(widget);
}

WIDGET *tui_make_text(char *restrict text, COLOR color) {
  return tui_new_widget(WIDGET_TYPE_TEXT, _tui_make_text_metadata(text, color));
}

TEXT_METADATA *_tui_make_text_metadata(char *restrict text, COLOR color) {
  TEXT_METADATA *metadata = malloc(sizeof(TEXT_METADATA));
  metadata->text = malloc(strlen(text) + 1);
  strcpy(metadata->text, text);
  metadata->color = color;
  return metadata;
}

void _tui_delete_text(WIDGET *restrict text) {
  free(((TEXT_METADATA *)text->metadata)->text);
  free(text->metadata);
}

WIDGET *tui_make_button(WIDGET *restrict child, ON_CLICK_CALLBACK callback) {
  return tui_new_widget(WIDGET_TYPE_BUTTON,
                        _tui_make_button_metadata(child, callback));
}

BUTTON_METADATA *_tui_make_button_metadata(WIDGET *restrict child,
                                           ON_CLICK_CALLBACK callback) {
  BUTTON_METADATA *metadata = malloc(sizeof(BUTTON_METADATA));
  metadata->child = child;
  metadata->callback = callback;
  return metadata;
}

void _tui_delete_button(WIDGET *restrict button) {
  tui_delete_widget(((BUTTON_METADATA *)button->metadata)->child);
  free(button->metadata);
}

WIDGET *tui_make_column(WIDGET_ARRAY *restrict children) {
  return tui_new_widget(WIDGET_TYPE_COLUMN,
                        _tui_make_column_metadata(children));
}

COLUMN_METADATA *_tui_make_column_metadata(WIDGET_ARRAY *restrict children) {
  COLUMN_METADATA *metadata = malloc(sizeof(COLUMN_METADATA));
  metadata->children = children;
  return metadata;
}

void _tui_delete_column(WIDGET *restrict column) {
  COLUMN_METADATA *metadata = column->metadata;
  _tui_delete_widget_array(metadata->children);
  free(column->metadata);
}

WIDGET *tui_make_row(WIDGET_ARRAY *restrict children) {
  return tui_new_widget(WIDGET_TYPE_ROW, _tui_make_row_metadata(children));
}

ROW_METADATA *_tui_make_row_metadata(WIDGET_ARRAY *restrict children) {
  ROW_METADATA *metadata = malloc(sizeof(ROW_METADATA));
  metadata->children = children;
  return metadata;
}

void _tui_delete_row(WIDGET *restrict row) {
  ROW_METADATA *metadata = row->metadata;
  _tui_delete_widget_array(metadata->children);
  free(row->metadata);
}

WIDGET *tui_make_box(int width, int height, WIDGET *restrict child,
                     COLOR color) {
  return tui_new_widget(WIDGET_TYPE_BOX,
                        _tui_make_box_metadata(child, width, height, color));
}

BOX_METADATA *_tui_make_box_metadata(WIDGET *restrict child, int width,
                                     int height, COLOR color) {
  BOX_METADATA *metadata = malloc(sizeof(BOX_METADATA));
  metadata->width = width;
  metadata->height = height;
  metadata->child = child;
  metadata->color = color;
  return metadata;
}

void _tui_delete_box(WIDGET *box) {
  tui_delete_widget(((BOX_METADATA *)box->metadata)->child);
  free(box->metadata);
}

WIDGET_ARRAY *tui_make_widget_array_raw(size_t size, ...) {
  va_list arg_pointer;
  va_start(arg_pointer, size);

  WIDGET **widgets = malloc(size * sizeof(WIDGET **));

  for (size_t i = 0; i < size; ++i) {
    widgets[i] = va_arg(arg_pointer, WIDGET *);
  }
  va_end(arg_pointer);

  WIDGET_ARRAY *widget_array = malloc(sizeof(WIDGET_ARRAY));
  widget_array->widgets = widgets;
  widget_array->size = size;

  return widget_array;
}

void _tui_delete_widget_array(WIDGET_ARRAY *restrict widget_array) {
  for (size_t i = 0; i < widget_array->size; ++i) {
    tui_delete_widget(widget_array->widgets[i]);
  }
  free(widget_array->widgets);
  free(widget_array);
}
