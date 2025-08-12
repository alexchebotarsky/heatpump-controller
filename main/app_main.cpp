#include <stdio.h>

#include "IRTransmitter.hpp"
#include "LoopManager.hpp"
#include "MQTTManager.hpp"
#include "Mode.hpp"
#include "Storage.hpp"
#include "TemperatureSensor.hpp"
#include "TimeServer.hpp"
#include "WiFiManager.hpp"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

// MQTT topics
constexpr const char* MQTT_CURRENT_STATE_TOPIC =
    CONFIG_MQTT_CURRENT_STATE_TOPIC;
constexpr const char* MQTT_TARGET_STATE_TOPIC = CONFIG_MQTT_TARGET_STATE_TOPIC;

WiFiManager wifi(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
MQTTManager mqtt(CONFIG_MQTT_BROKER_URL, CONFIG_MQTT_CLIENT_ID, CONFIG_MQTT_QOS,
                 CONFIG_MQTT_RETENTION_POLICY);
Storage storage(CONFIG_DEFAULT_MODE, CONFIG_DEFAULT_TARGET_TEMPERATURE);
LoopManager loop_manager(CONFIG_TEMPERATURE_CHECK_INTERVAL_MS);
TimeServer time_server;

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

  err = storage.init();
  if (err != ESP_OK) {
    printf("Error initializing storage: %s\n", esp_err_to_name(err));
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
    esp_err_t err = time_server.init();
    if (err != ESP_OK) {
      printf("Error initializing time server: %s\n", esp_err_to_name(err));
      esp_restart();
    }
  });

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

  mqtt.subscribe(MQTT_TARGET_STATE_TOPIC, [](const char* message) {
    // Ignore invalid messages
    cJSON* root = cJSON_Parse(message);
    if (!root) {
      return;
    }

    esp_err_t err = storage.populate_from_json(message);
    if (err != ESP_OK) {
      printf("Error populating storage from JSON message '%s': %s\n", message,
             esp_err_to_name(err));
      return;
    }

    Mode mode = storage.get_mode();
    int target_temperature = storage.get_target_temperature();
    printf("Set target state: mode=%s, target_temperature=%d\n",
           mode_to_str(mode), target_temperature);
  });

  while (true) {
    if (loop_manager.should_run()) {
      TemperatureReading reading = temperature_sensor.read();

      char message[256];
      snprintf(message, sizeof(message),
               "{\"currentTemperature\":%.1f,\"currentHumidity\":%.1f,"
               "\"timestamp\":\"%s\"}",
               reading.temperature, reading.humidity, time_server.timestamp());
      mqtt.publish(MQTT_CURRENT_STATE_TOPIC, message);
    }

    // Minimal delay to avoid busy-waiting
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
