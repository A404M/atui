#include "color.h"

COLOR color_init(uint32_t value) { return *(COLOR*)&value; }

bool color_equals(COLOR a, COLOR b) {
  return a.a == b.a && a.r == b.r && a.g == b.g && a.b == b.b;
}

bool color_not_equals(COLOR a, COLOR b) {
  return a.a != b.a || a.r != b.r || a.g != b.g || a.b != b.b;
}
