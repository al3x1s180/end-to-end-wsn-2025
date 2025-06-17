#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

static inline double temperature_int2double(int value) {
  return value / 100.0;
}

static inline double humidity_int2double(int value) {
  return value / 100.0;
}

static inline void float2str(double value, char *buf) {
  sprintf(buf, "%.2f", value);
}

#endif
