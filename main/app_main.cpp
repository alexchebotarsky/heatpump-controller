#include <stdio.h>

#include "IRTransmitter.hpp"
#include "LoopManager.hpp"
#include "MQTTManager.hpp"
#include "TemperatureSensor.hpp"
#include "WiFiManager.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

// MQTT topics
constexpr const char* MQTT_TEMPERATURE_SENSOR_TOPIC =
    CONFIG_MQTT_TEMPERATURE_SENSOR_TOPIC;
constexpr const char* MQTT_IR_TRANSMITTER_TOPIC =
    CONFIG_MQTT_IR_TRANSMITTER_TOPIC;

WiFiManager wifi(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
MQTTManager mqtt(CONFIG_MQTT_BROKER_URL, CONFIG_MQTT_CLIENT_ID, CONFIG_MQTT_QOS,
                 CONFIG_MQTT_RETENTION_POLICY);
LoopManager loop_manager(CONFIG_TEMPERATURE_PUBLISH_INTERVAL_MS);

TemperatureSensor temperature_sensor(CONFIG_TEMPERATURE_SENSOR_GPIO);
IRTransmitter ir_transmitter(CONFIG_IR_TRANSMITTER_GPIO,
                             CONFIG_IR_TRANSMITTER_PWM_CHANNEL,
                             CONFIG_IR_TRANSMITTER_PWM_TIMER);

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

  err = temperature_sensor.init();
  if (err != ESP_OK) {
    printf("Error initializing temperature sensor: %s\n", esp_err_to_name(err));
    esp_restart();
  }

  err = ir_transmitter.init();
  if (err != ESP_OK) {
    printf("Error initializing IR transmitter: %s\n", esp_err_to_name(err));
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

  mqtt.subscribe(MQTT_IR_TRANSMITTER_TOPIC, [](const char* json_str) {
    esp_err_t err = ir_transmitter.handle_ir_transmission(json_str);
    if (err != ESP_OK) {
      printf("Error handling IR transmission: %s\n", esp_err_to_name(err));
      return;
    }
  });

  while (true) {
    if (loop_manager.should_run()) {
      TemperatureReading reading = temperature_sensor.read();

      char message[72];
      snprintf(message, sizeof(message),
               "{\"temperature\":%.1f,\"humidity\":%.1f}", reading.temperature,
               reading.humidity);
      mqtt.publish(MQTT_TEMPERATURE_SENSOR_TOPIC, message);
    }

    // Minimal delay to avoid busy-waiting
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
