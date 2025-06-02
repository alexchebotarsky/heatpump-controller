#include "MQTTManager.hpp"

#include <string>

MQTTManager::MQTTManager(const char* broker_uri, const char* client_id,
                         const int qos, const int retention_policy)
    : broker_uri(broker_uri),
      client_id(client_id),
      qos(qos),
      retention_policy(retention_policy),
      is_connected(false),
      client(nullptr) {}

esp_err_t MQTTManager::init() {
  esp_mqtt_client_config_t cfg = {};
  cfg.broker.address.uri = broker_uri;
  cfg.credentials.client_id = client_id;

  client = esp_mqtt_client_init(&cfg);

  // Register event handlers for MQTT events
  esp_err_t err = esp_mqtt_client_register_event(
      client, MQTT_EVENT_CONNECTED, &MQTTManager::mqtt_event_handler, this);
  if (err != ESP_OK) {
    return err;
  }

  err = esp_mqtt_client_register_event(client, MQTT_EVENT_DISCONNECTED,
                                       &MQTTManager::mqtt_event_handler, this);
  if (err != ESP_OK) {
    return err;
  }

  err = esp_mqtt_client_register_event(client, MQTT_EVENT_DATA,
                                       &MQTTManager::mqtt_event_handler, this);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

esp_err_t MQTTManager::start() {
  if (client == nullptr) {
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t err = esp_mqtt_client_start(client);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

esp_err_t MQTTManager::stop() {
  if (client == nullptr) {
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t err = esp_mqtt_client_stop(client);
  if (err != ESP_OK) {
    return err;
  }

  return ESP_OK;
}

void MQTTManager::publish(const char* topic, const char* message) {
  // We do not retain messages, so ignore if the client is not connected
  if (!is_connected) {
    return;
  }

  int msg_id =
      esp_mqtt_client_publish(client, topic, message, 0, qos, retention_policy);

  if (msg_id < 0) {
    printf("Error publishing message to topic %s\n", topic);
    return;
  }
}

void MQTTManager::subscribe(const char* topic, Handler handler) {
  Subscription subscription = {topic, handler};
  subscriptions.push_back(subscription);
  printf("Subscribed to topic %s\n", topic);
}

void MQTTManager::mqtt_event_handler(void* arg, esp_event_base_t base,
                                     int32_t event_id, void* data) {
  auto* self = static_cast<MQTTManager*>(arg);

  switch (event_id) {
    case MQTT_EVENT_CONNECTED:
      self->handle_connected();
      break;
    case MQTT_EVENT_DISCONNECTED:
      self->handle_disconnected();
      break;
    case MQTT_EVENT_DATA:
      self->handle_message(static_cast<esp_mqtt_event_handle_t>(data));
      break;
  }
}

void MQTTManager::handle_connected() {
  printf("MQTT client %s connected\n", client_id);
  is_connected = true;

  for (const auto& subscription : subscriptions) {
    esp_mqtt_client_subscribe(client, subscription.topic, qos);
  }
}

void MQTTManager::handle_disconnected() {
  printf("MQTT client %s disconnected\n", client_id);
  is_connected = false;
}

void MQTTManager::handle_message(esp_mqtt_event_handle_t event) {
  std::string topic(event->topic, event->topic_len);
  std::string message(event->data, event->data_len);

  for (const auto& subscription : subscriptions) {
    if (strcmp(subscription.topic, topic.c_str()) == 0) {
      subscription.handler(message.c_str(), event->data_len);
      return;
    }
  }

  printf("Error: no handler for topic: %s\n", topic.c_str());
}
