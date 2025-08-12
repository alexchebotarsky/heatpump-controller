#ifndef OPERATING_STATE_HPP
#define OPERATING_STATE_HPP

enum class OperatingState { IDLE, COOLING, HEATING };

const char* operating_state_to_str(OperatingState mode);
OperatingState str_to_operating_state(const char* str);

#endif
