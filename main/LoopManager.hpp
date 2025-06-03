#ifndef LOOP_MANAGER_HPP
#define LOOP_MANAGER_HPP

#include <cstdint>

#include "freertos/FreeRTOS.h"

class LoopManager {
 public:
  LoopManager(uint32_t interval_ms);
  bool should_run();
  void force_run();

 private:
  TickType_t interval;
  TickType_t last_run;
  bool force_run_flag;
};

#endif
