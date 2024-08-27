#include "tui.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

void _tui_init_callbacks(TUI *tui) {
  const int width = tui_get_width(tui);
  const int height = tui_get_height(tui);

  tui->on_click_callbacks = malloc(width * sizeof(ON_CLICK_CALLBACK *));
  for (int i = 0; i < width; ++i) {
    tui->on_click_callbacks[i] = malloc(height * sizeof(ON_CLICK_CALLBACK));

    for (int j = 0; j < height; ++j) {
      tui->on_click_callbacks[i][j] = NULL;
    }
  }
}

void _tui_delete_callbacks(TUI *tui, int width) {
  for (int i = 0; i < width; ++i) {
    free(tui->on_click_callbacks[i]);
  }

  free(tui->on_click_callbacks);
}

TUI *tui_init() {
  setbuf(stdout, NULL);

  TUI *tui = malloc(sizeof(TUI));
  tui->buffer = malloc(0);

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

  _tui_init_callbacks(tui);

  tui_refresh(tui);
  return tui;
}

void tui_delete(TUI *tui) {
  const int width = tui_get_width(tui);

  // Revert the terminal back to its original state
  write(STDOUT_FILENO, "\e[?9l", 5);
  write(STDOUT_FILENO, "\e[?47l", 6);
  tcsetattr(STDIN_FILENO, TCSANOW, &tui->original);

  tui_move_to(tui->init_cursor_x, tui->init_cursor_y);

  _tui_delete_callbacks(tui, width);
  free(tui);
}

void tui_refresh(TUI *tui) {
  const int width = tui_get_width(tui);
  const int height = tui_get_height(tui);

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &tui->size);

  if (width == tui_get_width(tui) && height == tui_get_height(tui)) {
    return;
  }
  _tui_delete_callbacks(tui, width);
  _tui_init_callbacks(tui);
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
    --x;
    --y;
  }
  tcsetattr(0, TCSANOW, &tui->raw);
}

int tui_move_to(int x, int y) { return printf("\033[%d;%dH", y + 1, x + 1); }

int tui_delete_before() { return printf("\b \b"); }

int tui_delete_under_cursor() { return printf(" \b"); }

void tui_set_on_click_callback(TUI *tui, int x, int y,
                               ON_CLICK_CALLBACK callback) {
  tui->on_click_callbacks[x][y] = callback;
}

void tui_handle_mouse_action(TUI *tui, MOUSE_ACTION mouse_action) {
  const ON_CLICK_CALLBACK callback =
      tui->on_click_callbacks[mouse_action.x][mouse_action.y];
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

int handle_input(TUI *tui) {
  unsigned char buff[6];
  read(STDIN_FILENO, &buff, 1);
  if (buff[0] == 3) { // User pressd Ctr+C
    return 1;
  } else if (buff[0] == '\x1B') { // [ESCAPE]
    // TODO: fix for inputting actual <ESC>
    read(STDIN_FILENO, &buff, 5);
    switch (buff[1]) {
    case 77: {
      MOUSE_ACTION mouse_action = {
          .button = buff[2],
          .x = buff[3] - 32 - 1, // starts at 0
          .y = buff[4] - 32 - 1, // starts at 0
      };
      tui_handle_mouse_action(tui, mouse_action);
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
    case 'c':
      tui_clear_screen();
      break;
    case 's':
      tui_save_cursor();
      break;
    case 'r':
      tui_restore_cursor();
      break;
    case '\b':
    case 127: // back space
      tui_delete_before();
      break;
    default:
      /*printf("unknown:%c,%d\n\r", buff[0], buff[0]);*/
      break;
    }
  }
  return 0;
}

void tui_start_app(TUI *tui, WIDGET_BUILDER widget_builder) {
  tui_main_loop(tui, widget_builder);
}

void _tui_draw_widget(TUI *tui, const WIDGET *widget, int width_begin,
                      int width_end, int height_begin, int height_end,
                      COLOR parent_color, int *child_width, int *child_height) {
  switch (widget->type) {
  case WIDGET_TYPE_TEXT: {
    const TEXT_METADATA *metadata = widget->metadata;
    const int widthDiff = width_end - width_begin;
    const int textLen = strlen(metadata->text);
    int height = height_begin;
    for (; height < height_end; ++height) {
      tui_move_to(width_begin, height);
      const int begin = (height - height_begin) * widthDiff;
      int end = begin + widthDiff;
      int shouldExit = 0;
      if (end > textLen) {
        end = textLen;
        shouldExit = 1;
      }

      tui_change_terminal_text_color(metadata->color);
      tui_change_terminal_background_color(parent_color);
      write(STDOUT_FILENO, metadata->text + begin, end - begin);
      tui_change_terminal_background_color(COLOR_RESET);
      tui_change_terminal_text_color(COLOR_RESET);
      if (shouldExit) {
        break;
      }
    }
    *child_height = height + 1;
    *child_width =
        (textLen > widthDiff ? width_end : textLen + width_begin) + 1;
  } break;
  case WIDGET_TYPE_BUTTON: {
    const BUTTON_METADATA *metadata = widget->metadata;
    tui_move_to(width_begin, height_begin);
    if (metadata->child != NULL) {
      _tui_draw_widget(tui, metadata->child, width_begin, width_end,
                       height_begin, height_end, parent_color, child_width,
                       child_height);
      /*printf("%d,%d\n\r",*child_width,*child_height);*/
      /*sleep(1);*/
      for (int i = width_begin; i < *child_width; ++i) {
        for (int j = height_begin; j < *child_height; ++j) {
          tui_set_on_click_callback(tui, i, j, metadata->callback);
        }
      }
    }
  } break;
  case WIDGET_TYPE_COLUMN: {
    const COLUMN_METADATA *metadata = widget->metadata;
    *child_width = width_begin;
    *child_height = height_begin;
    for (int i = 0; i < metadata->children->size; ++i) {
      const WIDGET *child = metadata->children->widgets[i];
      int width_temp;
      _tui_draw_widget(tui, child, width_begin, width_end, *child_height,
                       height_end, parent_color, &width_temp, child_height);
      if (width_temp > *child_width) {
        *child_width = width_temp;
      }
    }
  } break;
  case WIDGET_TYPE_ROW: {
    const ROW_METADATA *metadata = widget->metadata;
    *child_width = width_begin;
    *child_height = height_begin;
    for (int i = 0; i < metadata->children->size; ++i) {
      const WIDGET *child = metadata->children->widgets[i];
      int height_temp;
      _tui_draw_widget(tui, child, *child_width, width_end, height_begin,
                       height_end, parent_color, child_width, &height_temp);
      if (height_temp > *child_height) {
        *child_height = height_temp;
      }
    }
  } break;
  case WIDGET_TYPE_BOX: {
    const BOX_METADATA *metadata = widget->metadata;

    if (metadata->width != -1) {
      width_end = metadata->width + width_begin >= width_end
                      ? width_end
                      : metadata->width + width_begin;
    }
    if (metadata->height != -1) {
      height_end = metadata->height + height_begin >= height_end
                       ? height_end
                       : metadata->height + height_begin;
    }

    for (int y = height_begin; y < height_end; ++y) {
      tui_move_to(width_begin, y);
      for (int x = width_begin; x < width_end; ++x) {
        tui_change_terminal_background_color(metadata->color);
        write(STDOUT_FILENO, " ", 1);
        tui_change_terminal_background_color(COLOR_RESET);
      }
    }

    if (metadata->child != NULL) {
      int t0, t1;
      _tui_draw_widget(tui, metadata->child, width_begin, width_end,
                       height_begin, height_end, metadata->color, &t0, &t1);
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

void tui_main_loop(TUI *tui, WIDGET_BUILDER widget_builder) {
  while (1) {
    clock_t start = clock();
    tui_refresh(tui);
    tui_save_cursor();
    tui_clear_screen();
    WIDGET *root_widget = widget_builder(tui);

    int width, height;
    _tui_draw_widget(tui, root_widget, 0, tui_get_width(tui), 0,
                     tui_get_height(tui), COLOR_NO_COLOR, &width, &height);

    tui_delete_widget(root_widget);
    tui_move_to(30, 30);
    printf("time:%ld",clock()-start);
    tui_restore_cursor();
    if (handle_input(tui)) {
      return;
    }
  }
}

WIDGET *tui_new_widget(WIDGET_TYPE type, void *metadata) {
  WIDGET *widget = malloc(sizeof(WIDGET));
  widget->type = type;
  widget->metadata = metadata;
  return widget;
}

void tui_delete_widget(WIDGET *widget) {
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

WIDGET *tui_make_text(char *text, COLOR color) {
  return tui_new_widget(WIDGET_TYPE_TEXT, _tui_make_text_metadata(text, color));
}

TEXT_METADATA *_tui_make_text_metadata(char *text, COLOR color) {
  TEXT_METADATA *metadata = malloc(sizeof(TEXT_METADATA));
  metadata->text = malloc(strlen(text) + 1);
  strcpy(metadata->text, text);
  metadata->color = color;
  return metadata;
}

void _tui_delete_text(WIDGET *text) {
  free(((TEXT_METADATA *)text->metadata)->text);
  free(text->metadata);
}

WIDGET *tui_make_button(WIDGET *child, ON_CLICK_CALLBACK callback) {
  return tui_new_widget(WIDGET_TYPE_BUTTON,
                        _tui_make_button_metadata(child, callback));
}

BUTTON_METADATA *_tui_make_button_metadata(WIDGET *child,
                                           ON_CLICK_CALLBACK callback) {
  BUTTON_METADATA *metadata = malloc(sizeof(BUTTON_METADATA));
  metadata->child = child;
  metadata->callback = callback;
  return metadata;
}

void _tui_delete_button(WIDGET *button) {
  tui_delete_widget(((BUTTON_METADATA *)button->metadata)->child);
  free(button->metadata);
}

WIDGET *tui_make_column(WIDGET_ARRAY *children) {
  return tui_new_widget(WIDGET_TYPE_COLUMN,
                        _tui_make_column_metadata(children));
}

COLUMN_METADATA *_tui_make_column_metadata(WIDGET_ARRAY *children) {
  COLUMN_METADATA *metadata = malloc(sizeof(COLUMN_METADATA));
  metadata->children = children;
  return metadata;
}

void _tui_delete_column(WIDGET *column) {
  COLUMN_METADATA *metadata = column->metadata;
  for (size_t i = 0; i < metadata->children->size; ++i) {
    tui_delete_widget(metadata->children->widgets[i]);
  }
  free(metadata->children);
  free(column->metadata);
}

WIDGET *tui_make_row(WIDGET_ARRAY *children) {
  return tui_new_widget(WIDGET_TYPE_ROW,
                        _tui_make_row_metadata(children));
}

ROW_METADATA *_tui_make_row_metadata(WIDGET_ARRAY *children) {
  ROW_METADATA *metadata = malloc(sizeof(ROW_METADATA));
  metadata->children = children;
  return metadata;
}

void _tui_delete_row(WIDGET *row) {
  ROW_METADATA *metadata = row->metadata;
  for (size_t i = 0; i < metadata->children->size; ++i) {
    tui_delete_widget(metadata->children->widgets[i]);
  }
  free(metadata->children);
  free(row->metadata);
}

WIDGET *tui_make_box(int width, int height, WIDGET *child, COLOR color) {
  return tui_new_widget(WIDGET_TYPE_BOX,
                        _tui_make_box_metadata(child, width, height, color));
}

BOX_METADATA *_tui_make_box_metadata(WIDGET *child, int width, int height,
                                     COLOR color) {
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

WIDGET_ARRAY *tui_make_widget_array(int size,...){
  va_list arg_pointer;
  va_start(arg_pointer, size);

  WIDGET **widgets = malloc(size*sizeof(WIDGET**));

  for(int i = 0;i < size;++i){
    widgets[i] = va_arg(arg_pointer,WIDGET*);
  }
  va_end(arg_pointer);

  WIDGET_ARRAY *widget_array = malloc(sizeof(WIDGET_ARRAY));
  widget_array->widgets = widgets;
  widget_array->size = size;

  return widget_array;
}
