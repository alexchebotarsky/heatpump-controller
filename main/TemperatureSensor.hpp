#ifndef TEMPERATURE_SENSOR_HPP
#define TEMPERATURE_SENSOR_HPP

#include "driver/gpio.h"

struct TemperatureReading {
  float temperature;
  float humidity;
};

class TemperatureSensor {
 public:
  TemperatureSensor(const int gpio_pin);
  esp_err_t init();

  TemperatureReading read();

 private:
  const gpio_num_t gpio;
};

#endif
