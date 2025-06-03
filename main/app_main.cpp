#include <stdio.h>

#include "MQTTManager.hpp"
#include "WiFiManager.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

constexpr const int TEMPERATURE_PUBLISH_INTERVAL_MS =
    CONFIG_TEMPERATURE_PUBLISH_INTERVAL_MS;

// MQTT topics
constexpr const char* MQTT_TEMPERATURE_SENSOR_TOPIC =
    CONFIG_MQTT_TEMPERATURE_SENSOR_TOPIC;
constexpr const char* MQTT_IR_TRANSMITTER_TOPIC =
    CONFIG_MQTT_IR_TRANSMITTER_TOPIC;

WiFiManager wifi(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
MQTTManager mqtt(CONFIG_MQTT_BROKER_URL, CONFIG_MQTT_CLIENT_ID, CONFIG_MQTT_QOS,
                 CONFIG_MQTT_RETENTION_POLICY);

extern "C" void app_main(void) {
  esp_err_t err = nvs_flash_init();
  if (err != ESP_OK) {
    printf("Error initializing NVS: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  err = wifi.init();
  if (err != ESP_OK) {
    printf("Error initializing WiFi manager: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  err = mqtt.init();
  if (err != ESP_OK) {
    printf("Error initializing MQTT client: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  wifi.on_connect([]() {
    esp_err_t err = mqtt.start();
    if (err != ESP_OK) {
      printf("Error starting MQTT client: %s\n", esp_err_to_name(err));
      esp_restart();
    }
  });

  wifi.on_disconnect([]() {
    esp_err_t err = mqtt.stop();
    if (err != ESP_OK) {
      printf("Error stopping MQTT client: %s\n", esp_err_to_name(err));
      esp_restart();
    }
  });

  mqtt.subscribe(MQTT_IR_TRANSMITTER_TOPIC, [](const char* message) {
    printf("Transmitting IR signal: %s\n", message);
  });

  TickType_t last_check = 0;
  while (true) {
    TickType_t now = xTaskGetTickCount();

    if (now - last_check >= pdMS_TO_TICKS(TEMPERATURE_PUBLISH_INTERVAL_MS)) {
      last_check = now;

      float current_temperature = 20.0;
      float current_humidity = 50.0;

      char message[80];
      snprintf(message, sizeof(message),
               "{\"currentTemperature\":%.2f,\"currentHumidity\":\"%.2f\"}",
               current_temperature, current_humidity);
      mqtt.publish(MQTT_TEMPERATURE_SENSOR_TOPIC, message);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
