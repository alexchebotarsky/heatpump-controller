#ifndef MQTT_MANAGER_HPP
#define MQTT_MANAGER_HPP

#include <cstdint>
#include <vector>

#include "esp_event.h"
#include "mqtt_client.h"

typedef void (*Handler)(const char* message, const int length);

struct Subscription {
  const char* topic;
  Handler handler;
};

class MQTTManager {
 public:
  MQTTManager(const char* broker_uri, const char* clientId, const int qos,
              const int retention_policy);
  esp_err_t init();

  esp_err_t start();
  esp_err_t stop();

  void publish(const char* topic, const char* payload);
  void subscribe(const char* topic, Handler handler);

 private:
  const char* broker_uri;
  const char* client_id;
  const int qos;
  const int retention_policy;
  bool is_connected;
  esp_mqtt_client_handle_t client;
  std::vector<Subscription> subscriptions;

  static void mqtt_event_handler(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data);

  void handle_connected();
  void handle_disconnected();
  void handle_message(esp_mqtt_event_handle_t event);
};

#endif
