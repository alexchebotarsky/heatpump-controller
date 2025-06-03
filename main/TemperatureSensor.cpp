#include "TemperatureSensor.hpp"

#include <cstdint>

#include "dht.h"

constexpr char MAX_SENSORS = 1;

TemperatureSensor::TemperatureSensor(const int gpio_pin)
    : gpio(static_cast<gpio_num_t>(gpio_pin)) {}

esp_err_t TemperatureSensor::init() {
  gpio_config_t config = {};
  config.mode = GPIO_MODE_INPUT;
  config.pin_bit_mask = 1ULL << gpio;

  esp_err_t err = gpio_config(&config);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

TemperatureReading TemperatureSensor::read() {
  int16_t temperature;
  int16_t humidity;

  esp_err_t err = dht_read_data(DHT_TYPE_AM2301, gpio, &humidity, &temperature);
  if (err != ESP_OK) {
    printf("Error getting temperature reading: %s\n", esp_err_to_name(err));
    return TemperatureReading{0, 0};
  }

  return TemperatureReading{static_cast<float>(temperature) / 10,
                            static_cast<float>(humidity) / 10};
}
