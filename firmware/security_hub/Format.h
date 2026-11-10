#pragma once
#include <Arduino.h>
#include <stdarg.h>

// printf-style helper so config strings can carry placeholders.
inline String format(const char* fmt, ...) {
  char buffer[160];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  return String(buffer);
}
