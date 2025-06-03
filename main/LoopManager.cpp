#include "LoopManager.hpp"

LoopManager::LoopManager(uint32_t interval_ms)
    : interval(pdMS_TO_TICKS(interval_ms)),
      last_run(0),
      force_run_flag(false) {}

bool LoopManager::should_run() {
  TickType_t now = xTaskGetTickCount();
  if (now - last_run >= interval || force_run_flag) {
    last_run = now;
    force_run_flag = false;
    return true;
  }

  return false;
}

void LoopManager::force_run() { force_run_flag = true; }
