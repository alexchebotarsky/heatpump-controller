#ifndef TIME_SERVER_HPP
#define TIME_SERVER_HPP

#include "esp_err.h"

class TimeServer {
 public:
  TimeServer();
  esp_err_t init();
  char* timestamp();
};

#endif
