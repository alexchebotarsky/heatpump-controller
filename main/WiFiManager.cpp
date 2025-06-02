#include "WiFiManager.hpp"

#include "esp_wifi.h"

WiFiManager::WiFiManager(const char* ssid, const char* password)
    : ssid(ssid), password(password) {}

esp_err_t WiFiManager::init() {
  // Initialize ESP network interface abstraction layer
  esp_err_t err = esp_netif_init();
  if (err != ESP_OK) {
    return err;
  }

  // Create the default ESP event loop.
  err = esp_event_loop_create_default();
  if (err != ESP_OK) {
    return err;
  }

  // Create the default network interface for Wi-Fi station mode.
  esp_netif_t* netif_ptr = esp_netif_create_default_wifi_sta();
  if (netif_ptr == nullptr) {
    return ESP_ERR_ESP_NETIF_INIT_FAILED;
  }

  // Initialize Wi-Fi driver
  wifi_init_config_t driver_config = WIFI_INIT_CONFIG_DEFAULT();
  err = esp_wifi_init(&driver_config);
  if (err != ESP_OK) {
    return err;
  }

  // Set up Wi-Fi configuration
  wifi_config_t wifi_config = {};
  snprintf((char*)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), ssid);
  snprintf((char*)wifi_config.sta.password, sizeof(wifi_config.sta.password),
           password);

  err = esp_wifi_set_mode(WIFI_MODE_STA);
  if (err != ESP_OK) {
    return err;
  }

  err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  if (err != ESP_OK) {
    return err;
  }

  // Register event handlers for Wi-Fi events
  err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                   &WiFiManager::wifi_event_handler, this);
  if (err != ESP_OK) {
    return err;
  }

  err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                   &WiFiManager::wifi_event_handler, this);
  if (err != ESP_OK) {
    return err;
  }

  // Start Wi-Fi driver
  err = esp_wifi_start();
  if (err != ESP_OK) {
    return err;
  }

  // Connect to Wi-Fi
  err = esp_wifi_connect();
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

void WiFiManager::on_connect(Callback callback) {
  callbacks_on_connect.push_back(callback);
}

void WiFiManager::on_disconnect(Callback callback) {
  callbacks_on_disconnect.push_back(callback);
}

void WiFiManager::wifi_event_handler(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data) {
  auto* self = static_cast<WiFiManager*>(arg);

  switch (event_id) {
    case IP_EVENT_STA_GOT_IP:
      self->handle_got_ip(static_cast<ip_event_got_ip_t*>(event_data));
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      self->handle_disconnected();
      break;
  }
}

void WiFiManager::handle_got_ip(ip_event_got_ip_t* event) {
  printf("Wi-Fi connected, IP address: %d.%d.%d.%d\n",
         IP2STR(&event->ip_info.ip));

  for (const auto& callback : callbacks_on_connect) {
    callback();
  }
}

void WiFiManager::handle_disconnected() {
  for (const auto& callback : callbacks_on_disconnect) {
    callback();
  }

  printf("Wi-Fi disconnected, reconnecting...\n");
  esp_wifi_connect();
}
