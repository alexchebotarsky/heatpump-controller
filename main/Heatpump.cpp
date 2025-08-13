#include "Heatpump.hpp"

#include <bitset>
#include <cstdint>

#include "cJSON.h"
#include "nvs_flash.h"

constexpr const char* NVS_NAMESPACE = "heatpump";

constexpr const char* BINARY_HEADER =
    "1111001000001101000000111111110000000001";

constexpr const char* MODE_NVS_KEY = "mode";
constexpr const char* MODE_JSON_KEY = "mode";

constexpr const char* TARGET_TEMPERATURE_NVS_KEY = "target_temp";
constexpr const char* TARGET_TEMPERATURE_JSON_KEY = "targetTemperature";

constexpr const char* FAN_SPEED_NVS_KEY = "fan_speed";
constexpr const char* FAN_SPEED_JSON_KEY = "fanSpeed";

Heatpump::Heatpump(const char* default_mode,
                   const int default_target_temperature)
    : mode(str_to_mode(default_mode)),
      target_temperature(default_target_temperature),
      fan_speed(0) {}

esp_err_t Heatpump::init() {
  nvs_handle_t nvs_storage;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_storage);
  if (err != ESP_OK) {
    return err;
  }

  static char mode[16];
  size_t mode_size = sizeof(mode);
  if (nvs_get_str(nvs_storage, MODE_NVS_KEY, mode, &mode_size) == ESP_OK) {
    this->mode = str_to_mode(mode);
  }

  int32_t target_temperature;
  if (nvs_get_i32(nvs_storage, TARGET_TEMPERATURE_NVS_KEY,
                  &target_temperature) == ESP_OK) {
    this->target_temperature = target_temperature;
  }

  nvs_close(nvs_storage);
  return ESP_OK;
}

esp_err_t Heatpump::set_mode(const Mode mode) {
  nvs_handle_t nvs_storage;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_storage);
  if (err != ESP_OK) {
    return err;
  }

  err = nvs_set_str(nvs_storage, MODE_NVS_KEY, mode_to_str(mode));
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  err = nvs_commit(nvs_storage);
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  this->mode = mode;

  nvs_close(nvs_storage);
  return ESP_OK;
}

Mode Heatpump::get_mode() { return mode; }

esp_err_t Heatpump::set_target_temperature(const int target_temperature) {
  if (target_temperature < 17 || target_temperature > 30) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t nvs_storage;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_storage);
  if (err != ESP_OK) {
    return err;
  }

  err =
      nvs_set_i32(nvs_storage, TARGET_TEMPERATURE_NVS_KEY, target_temperature);
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  err = nvs_commit(nvs_storage);
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  this->target_temperature = target_temperature;

  nvs_close(nvs_storage);
  return ESP_OK;
}

int Heatpump::get_target_temperature() { return target_temperature; }

esp_err_t Heatpump::set_fan_speed(const int fan_speed) {
  if (fan_speed < 0 || fan_speed > 100) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t nvs_storage;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_storage);
  if (err != ESP_OK) {
    return err;
  }

  err = nvs_set_i32(nvs_storage, FAN_SPEED_NVS_KEY, fan_speed);
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  err = nvs_commit(nvs_storage);
  if (err != ESP_OK) {
    nvs_close(nvs_storage);
    return err;
  }

  this->fan_speed = fan_speed;

  nvs_close(nvs_storage);
  return ESP_OK;
}

int Heatpump::get_fan_speed() { return fan_speed; }

esp_err_t Heatpump::populate_from_json(const char* json_str) {
  cJSON* root = cJSON_Parse(json_str);
  if (!root) {
    return ESP_ERR_INVALID_ARG;
  }

  cJSON* mode_item = cJSON_GetObjectItem(root, MODE_JSON_KEY);
  if (cJSON_IsString(mode_item)) {
    esp_err_t err = set_mode(str_to_mode(mode_item->valuestring));
    if (err != ESP_OK) {
      cJSON_Delete(root);
      return err;
    }
  }

  cJSON* target_temp_item =
      cJSON_GetObjectItem(root, TARGET_TEMPERATURE_JSON_KEY);
  if (cJSON_IsNumber(target_temp_item)) {
    esp_err_t err = set_target_temperature(target_temp_item->valueint);
    if (err != ESP_OK) {
      cJSON_Delete(root);
      return err;
    }
  }

  cJSON* fan_speed_item = cJSON_GetObjectItem(root, FAN_SPEED_JSON_KEY);
  if (cJSON_IsNumber(fan_speed_item)) {
    esp_err_t err = set_fan_speed(fan_speed_item->valueint);
    if (err != ESP_OK) {
      cJSON_Delete(root);
      return err;
    }
  }

  cJSON_Delete(root);
  return ESP_OK;
}

std::string Heatpump::to_binary_state() {
  // Temperature range conversion: [17-30] to [0-13]
  int temp = target_temperature - 17;

  int fan = 0;
  if (fan_speed > 0) {
    fan = (fan_speed / 20) * 2 + 2;
  } else {
    fan = 0;  // AUTO
  }

  // Power is inverted: 0=ON, 1=OFF
  int p = (mode == Mode::OFF) ? 1 : 0;

  int mo = 0;
  switch (mode) {
    case Mode::AUTO:
      mo = 0;
      break;
    case Mode::COOL:
      mo = 1;
      break;
    case Mode::HEAT:
      mo = 3;
      break;
    case Mode::OFF:
      // For some reason when power is off, the mode is always HEAT
      mo = 3;
      break;
  }

  // Calculate checksums
  int chsm = (temp + fan) % 16;
  int cs = mo ^ 1;

  std::string result = BINARY_HEADER;
  result += std::bitset<4>(temp).to_string();
  result += "0000";
  result += std::bitset<4>(fan).to_string();
  result += "0";
  result += std::to_string(p);
  result += std::bitset<2>(mo).to_string();
  result += "00000000";
  result += std::bitset<4>(chsm).to_string();
  result += "0";
  result += std::to_string(p);
  result += std::bitset<2>(cs).to_string();

  return result;
}
