#include "OperatingState.hpp"

#include <cstring>

const char* operating_state_to_str(OperatingState state) {
  if (state == OperatingState::IDLE) return "IDLE";
  if (state == OperatingState::COOLING) return "COOLING";
  if (state == OperatingState::HEATING) return "HEATING";

  // Default
  return "IDLE";
}

OperatingState str_to_operating_state(const char* str) {
  if (strcmp(str, "IDLE") == 0) return OperatingState::IDLE;
  if (strcmp(str, "COOLING") == 0) return OperatingState::COOLING;
  if (strcmp(str, "HEATING") == 0) return OperatingState::HEATING;

  // Default
  return OperatingState::IDLE;
}
