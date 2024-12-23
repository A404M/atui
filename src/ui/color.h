#pragma once

#include <stdint.h>

typedef struct COLOR {
  uint8_t b, g, r, a;
} COLOR;

#define color_hex(value) { \
  (uint8_t)(((uint32_t)value >> 8*0)&0xFF), \
  (uint8_t)(((uint32_t)value >> 8*1)&0xFF), \
  (uint8_t)(((uint32_t)value >> 8*2)&0xFF), \
  (uint8_t)(((uint32_t)value >> 8*3)&0xFF), \
}

constexpr COLOR COLOR_NO_COLOR = {
    .a = 0,
    .r = 0,
    .g = 0,
    .b = 0,
};


extern COLOR color_init(uint32_t value);

extern bool color_equals(COLOR a,COLOR b);
extern bool color_not_equals(COLOR a,COLOR b);
