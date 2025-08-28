#include <stdio.h>

#include "Heatpump.hpp"
#include "IRTransmitter.hpp"
#include "LoopManager.hpp"
#include "MQTTManager.hpp"
#include "Mode.hpp"
#include "OperatingState.hpp"
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

constexpr const char* DEVICE_ID = CONFIG_DEVICE_ID;

WiFiManager wifi(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
MQTTManager mqtt(CONFIG_MQTT_BROKER_URL, CONFIG_DEVICE_ID, CONFIG_MQTT_QOS,
                 CONFIG_MQTT_RETENTION_POLICY);
LoopManager loop_manager(CONFIG_TEMPERATURE_CHECK_INTERVAL_MS);
TimeServer time_server;

Heatpump heatpump(CONFIG_DEFAULT_MODE, CONFIG_DEFAULT_TARGET_TEMPERATURE);

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

  err = heatpump.init();
  if (err != ESP_OK) {
    printf("Error initializing heatpump: %s\n", esp_err_to_name(err));
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

    // Ignore message that don't have deviceId or it doesn't match this device
    cJSON* device_id_item = cJSON_GetObjectItem(root, "deviceId");
    if (!cJSON_IsString(device_id_item) ||
        strcmp(device_id_item->valuestring, DEVICE_ID) != 0) {
      cJSON_Delete(root);
      return;
    }
    cJSON_Delete(root);

    esp_err_t err = heatpump.populate_from_json(message);
    if (err != ESP_OK) {
      printf("Error populating heatpump from JSON message '%s': %s\n", message,
             esp_err_to_name(err));
      return;
    }

    // Force run immediately to apply the changes
    loop_manager.force_run();

    // Transmit new state
    const char* signal = heatpump.to_binary_state();
    ir_transmitter.transmit_ir_signal(signal);

    Mode mode = heatpump.get_mode();
    int target_temperature = heatpump.get_target_temperature();
    printf("Set target state: mode=%s, target_temperature=%d\n",
           mode_to_str(mode), target_temperature);
  });

  // Transmit saved state on startup
  const char* signal = heatpump.to_binary_state();
  ir_transmitter.transmit_ir_signal(signal);

  while (true) {
    if (loop_manager.should_run()) {
      TemperatureReading reading = temperature_sensor.read();
      int target_temperature = heatpump.get_target_temperature();
      Mode mode = heatpump.get_mode();

      // Since we don't know exactly what the heatpump does right now, we just
      // estimate based on target and current temperatures.
      OperatingState operating_state = OperatingState::IDLE;
      if (reading.temperature > target_temperature &&
          (mode == Mode::AUTO || mode == Mode::COOL)) {
        operating_state = OperatingState::COOLING;
      } else if (reading.temperature < target_temperature &&
                 (mode == Mode::AUTO || mode == Mode::HEAT)) {
        operating_state = OperatingState::HEATING;
      } else {
        operating_state = OperatingState::IDLE;
      }

      char message[165];
      snprintf(message, sizeof(message),
               "{\"deviceId\":\"%s\",\"operatingState\":\"%s\","
               "\"currentTemperature\":%.1f,\"currentHumidity\":%.1f,"
               "\"timestamp\":\"%s\"}",
               DEVICE_ID, operating_state_to_str(operating_state),
               reading.temperature, reading.humidity, time_server.timestamp());
      mqtt.publish(MQTT_CURRENT_STATE_TOPIC, message);
    }

    // Minimal delay to avoid busy-waiting
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
