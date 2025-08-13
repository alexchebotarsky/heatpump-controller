#include "TimeServer.hpp"

#include <ctime>

#include "esp_netif_sntp.h"

constexpr const char* NTP_SERVER = "pool.ntp.org";

TimeServer::TimeServer() {}

esp_err_t TimeServer::init() {
  // Initialize SNTP
  esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVER);
  esp_err_t err = esp_netif_sntp_init(&config);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

char* TimeServer::timestamp() {
  static char timestamp[32];
  time_t now = time(nullptr);
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
  return timestamp;
}