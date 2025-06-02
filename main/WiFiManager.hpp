#ifndef WIFI_MANAGER_HPP
#define WIFI_MANAGER_HPP

#include <vector>

#include "esp_event.h"
#include "esp_netif.h"

typedef void (*Callback)();

class WiFiManager {
 public:
  WiFiManager(const char* ssid, const char* password);
  esp_err_t init();

  void on_connect(Callback callback);
  void on_disconnect(Callback callback);

 private:
  const char* ssid;
  const char* password;
  std::vector<Callback> callbacks_on_connect;
  std::vector<Callback> callbacks_on_disconnect;

  static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data);

  void handle_got_ip(ip_event_got_ip_t* event);
  void handle_disconnected();
};

#endif
