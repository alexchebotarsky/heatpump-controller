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

  char mode[8];
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

void int_to_bin(char* dest, uint32_t value, int bits) {
  for (int i = bits - 1; i >= 0; --i) {
    *dest++ = ((value >> i) & 1) ? '1' : '0';
  }
  *dest = '\0';
}

const char* Heatpump::to_binary_state() {
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

  // Convert to binary
  char temp_bin[5];
  int_to_bin(temp_bin, temp, 4);

  char fan_bin[5];
  int_to_bin(fan_bin, fan, 4);

  char p_bin[2];
  int_to_bin(p_bin, p, 1);

  char mo_bin[3];
  int_to_bin(mo_bin, mo, 2);

  char chsm_bin[5];
  int_to_bin(chsm_bin, chsm, 4);

  char cs_bin[3];
  int_to_bin(cs_bin, cs, 2);

  // Combine all binary strings into the final state
  static char state[73];
  snprintf(state, sizeof(state), "%s%s0000%s0%s%s00000000%s0%s%s",
           BINARY_HEADER, temp_bin, fan_bin, p_bin, mo_bin, chsm_bin, p_bin,
           cs_bin);

  return state;
}
