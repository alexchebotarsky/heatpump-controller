#ifndef IR_TRANSMITTER_HPP
#define IR_TRANSMITTER_HPP

#include <cstdint>

#include "driver/gpio.h"
#include "driver/ledc.h"

class IRTransmitter {
 public:
  IRTransmitter(const int gpio_pin, const int pwm_channel, const int pwm_timer);
  esp_err_t init();

  esp_err_t handle_ir_transmission(const char* signal);

 private:
  const gpio_num_t gpio;
  const ledc_channel_t pwm_channel;
  const ledc_timer_t pwm_timer;

  esp_err_t send_signal(const char* signal);
  esp_err_t send_pulse(uint32_t duration_us);
  esp_err_t send_space(uint32_t duration_us);
};

#endif
