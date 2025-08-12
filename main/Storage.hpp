#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "Mode.hpp"
#include "esp_err.h"

class Storage {
 public:
  Storage(const char* default_mode, const int default_target_temperature);
  esp_err_t init();

  esp_err_t set_mode(const Mode mode);
  Mode get_mode();

  esp_err_t set_target_temperature(const int target_temperature);
  int get_target_temperature();

  esp_err_t populate_from_json(const char* json);

 private:
  Mode mode;
  int target_temperature;
};

#endif
