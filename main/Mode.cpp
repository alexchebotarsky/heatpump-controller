#include "Mode.hpp"

#include <cstring>

const char* mode_to_str(Mode mode) {
  if (mode == Mode::OFF) return "OFF";
  if (mode == Mode::COOL) return "COOL";
  if (mode == Mode::HEAT) return "HEAT";
  if (mode == Mode::AUTO) return "AUTO";

  // Default
  return "OFF";
}

Mode str_to_mode(const char* str) {
  if (strcmp(str, "OFF") == 0) return Mode::OFF;
  if (strcmp(str, "COOL") == 0) return Mode::COOL;
  if (strcmp(str, "HEAT") == 0) return Mode::HEAT;
  if (strcmp(str, "AUTO") == 0) return Mode::AUTO;

  // Default
  return Mode::OFF;
}
