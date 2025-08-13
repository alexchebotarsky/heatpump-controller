#ifndef HEATPUMP_HPP
#define HEATPUMP_HPP

#include <string>

#include "Mode.hpp"
#include "esp_err.h"

class Heatpump {
 public:
  Heatpump(const char* default_mode, const int default_target_temperature);
  esp_err_t init();

  esp_err_t set_mode(const Mode mode);
  Mode get_mode();

  esp_err_t set_target_temperature(const int target_temperature);
  int get_target_temperature();

  esp_err_t set_fan_speed(const int fan_speed);
  int get_fan_speed();

  esp_err_t populate_from_json(const char* json);

  std::string to_binary_state();

 private:
  Mode mode;
  int target_temperature;
  int fan_speed;
};

#endif
