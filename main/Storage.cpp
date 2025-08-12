#include "Storage.hpp"

#include <cstdint>

#include "cJSON.h"
#include "nvs_flash.h"

constexpr const char* NVS_NAMESPACE = "heatpump";

constexpr const char* DEVICE_ID_JSON_KEY = "deviceId";

constexpr const char* MODE_NVS_KEY = "mode";
constexpr const char* MODE_JSON_KEY = "mode";

constexpr const char* TARGET_TEMPERATURE_NVS_KEY = "target_temp";
constexpr const char* TARGET_TEMPERATURE_JSON_KEY = "targetTemperature";

Storage::Storage(const char* default_mode, const int default_target_temperature)
    : mode(str_to_mode(default_mode)),
      target_temperature(default_target_temperature) {}

esp_err_t Storage::init() {
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

esp_err_t Storage::set_mode(const Mode mode) {
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

Mode Storage::get_mode() { return mode; }

esp_err_t Storage::set_target_temperature(const int target_temperature) {
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

int Storage::get_target_temperature() { return target_temperature; }

esp_err_t Storage::populate_from_json(const char* json_str) {
  cJSON* root = cJSON_Parse(json_str);
  if (!root) {
    return ESP_ERR_INVALID_ARG;
  }

  cJSON* mode_item = cJSON_GetObjectItem(root, MODE_JSON_KEY);
  if (!cJSON_IsString(mode_item)) {
    cJSON_Delete(root);
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t err = set_mode(str_to_mode(mode_item->valuestring));
  if (err != ESP_OK) {
    cJSON_Delete(root);
    return err;
  }

  cJSON* target_temp_item =
      cJSON_GetObjectItem(root, TARGET_TEMPERATURE_JSON_KEY);
  if (!cJSON_IsNumber(target_temp_item)) {
    cJSON_Delete(root);
    return ESP_ERR_INVALID_ARG;
  }

  err = set_target_temperature(target_temp_item->valueint);
  if (err != ESP_OK) {
    cJSON_Delete(root);
    return err;
  }

  cJSON_Delete(root);
  return ESP_OK;
}