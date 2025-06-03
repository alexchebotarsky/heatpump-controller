#include "IRTransmitter.hpp"

#include <cstring>

#include "cJSON.h"
#include "esp_rom_sys.h"

constexpr const char* SIGNAL_KEY = "signal";

// PWM configuration
constexpr const ledc_mode_t PWM_SPEED_MODE = LEDC_LOW_SPEED_MODE;
constexpr const uint32_t PWM_FREQUENCY = 38000;                       // 38kHz
constexpr const ledc_timer_bit_t PWM_RESOLUTION = LEDC_TIMER_10_BIT;  // 2^10
constexpr const uint32_t PWM_DUTY_OFF = 0;                            // 0%
constexpr const uint32_t PWM_DUTY_ON = (1 << PWM_RESOLUTION) / 2;     // 50%

constexpr uint32_t HEADER_PULSE = 4400;
constexpr uint32_t HEADER_SPACE = 4350;
constexpr uint32_t ZERO_PULSE = 560;
constexpr uint32_t ZERO_SPACE = 520;
constexpr uint32_t ONE_PULSE = 560;
constexpr uint32_t ONE_SPACE = 1600;
constexpr uint32_t END_PULSE = 560;
constexpr uint32_t END_SPACE = 7450;

IRTransmitter::IRTransmitter(const int gpio_pin, const int pwm_channel,
                             const int pwm_timer)
    : gpio(static_cast<gpio_num_t>(gpio_pin)),
      pwm_channel(static_cast<ledc_channel_t>(pwm_channel)),
      pwm_timer(static_cast<ledc_timer_t>(pwm_timer)) {}

esp_err_t IRTransmitter::init() {
  gpio_config_t config = {};
  config.mode = GPIO_MODE_OUTPUT;
  config.pin_bit_mask = 1ULL << gpio;

  esp_err_t err = gpio_config(&config);
  if (err != ESP_OK) {
    return err;
  }

  ledc_timer_config_t ledc_timer = {};
  ledc_timer.timer_num = pwm_timer;
  ledc_timer.speed_mode = PWM_SPEED_MODE;
  ledc_timer.freq_hz = PWM_FREQUENCY;
  ledc_timer.duty_resolution = PWM_RESOLUTION;

  err = ledc_timer_config(&ledc_timer);
  if (err != ESP_OK) {
    return err;
  }

  ledc_channel_config_t ledc_channel = {};
  ledc_channel.gpio_num = gpio;
  ledc_channel.channel = pwm_channel;
  ledc_channel.timer_sel = pwm_timer;
  ledc_channel.speed_mode = PWM_SPEED_MODE;
  ledc_channel.duty = 0;  // OFF by default

  err = ledc_channel_config(&ledc_channel);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

esp_err_t IRTransmitter::handle_ir_transmission(const char* json_signal) {
  cJSON* root = cJSON_Parse(json_signal);
  if (!root) {
    return ESP_ERR_INVALID_ARG;
  }

  cJSON* signal_item = cJSON_GetObjectItem(root, SIGNAL_KEY);
  if (!cJSON_IsString(signal_item)) {
    cJSON_Delete(root);
    return ESP_ERR_INVALID_ARG;
  }
  const char* signal = signal_item->valuestring;

  // Send the IR signal twice to ensure reception
  esp_err_t err;
  for (size_t i = 0; i < 2; i++) {
    err = send_signal(signal);
    if (err != ESP_OK) {
      cJSON_Delete(root);
      return err;
    }
  }

  printf("Signal transmitted: %s\n", signal);

  cJSON_Delete(root);
  return ESP_OK;
}

esp_err_t IRTransmitter::send_signal(const char* signal) {
  esp_err_t err;

  // Send header
  if (err = send_pulse(HEADER_PULSE), err != ESP_OK) return err;
  if (err = send_space(HEADER_SPACE), err != ESP_OK) return err;

  // Iterate through each bit in the signal string
  for (size_t i = 0; i < strlen(signal); i++) {
    if (signal[i] == '1') {
      if (err = send_pulse(ONE_PULSE), err != ESP_OK) return err;
      if (err = send_space(ONE_SPACE), err != ESP_OK) return err;
    } else if (signal[i] == '0') {
      if (err = send_pulse(ZERO_PULSE), err != ESP_OK) return err;
      if (err = send_space(ZERO_SPACE), err != ESP_OK) return err;
    } else {
      return ESP_ERR_INVALID_ARG;
    }
  }

  // Send end pulse
  if (err = send_pulse(END_PULSE), err != ESP_OK) return err;
  if (err = send_space(END_SPACE), err != ESP_OK) return err;

  return ESP_OK;
}

esp_err_t IRTransmitter::send_pulse(uint32_t duration_us) {
  esp_err_t err = ledc_set_duty(PWM_SPEED_MODE, pwm_channel, PWM_DUTY_ON);
  if (err != ESP_OK) {
    return err;
  }
  err = ledc_update_duty(PWM_SPEED_MODE, pwm_channel);
  if (err != ESP_OK) {
    return err;
  }

  esp_rom_delay_us(duration_us);

  err = ledc_set_duty(PWM_SPEED_MODE, pwm_channel, PWM_DUTY_OFF);
  if (err != ESP_OK) {
    return err;
  }
  err = ledc_update_duty(PWM_SPEED_MODE, pwm_channel);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

esp_err_t IRTransmitter::send_space(uint32_t duration_us) {
  esp_err_t err = ledc_set_duty(PWM_SPEED_MODE, pwm_channel, PWM_DUTY_OFF);
  if (err != ESP_OK) {
    return err;
  }
  err = ledc_update_duty(PWM_SPEED_MODE, pwm_channel);
  if (err != ESP_OK) {
    return err;
  }

  esp_rom_delay_us(duration_us);

  return ESP_OK;
}
