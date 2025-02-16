#ifndef RAPT_MATE_SERVER_HPP
#define RAPT_MATE_SERVER_HPP

#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_system.h"
#include "mdns.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "drivers/RaptPillBLE.hpp"
#include "common/core.hpp"
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include "esp_spiffs.h"

#define WIFI_SSID "Kasper - iPhone"
#define WIFI_PASS "kasper123"

class RaptMateServer {
public:
    RaptMateServer(RaptPillBLE* ble) : server(NULL), ble(ble) {}
    void init();
    RaptPillData get_data() { return rapt_pill_data; }
private:

    httpd_handle_t server;
    RaptPillBLE* ble;

    void init_wifi();
    void init_mdns();
    void init_http_server();

    // Static event handler for Wi‑Fi/IP events.
    static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data);
    // HTTP URI handlers.
    static esp_err_t index_get_handler(httpd_req_t *req);
    static esp_err_t static_file_get_handler(httpd_req_t *req);
    static esp_err_t settings_post_handler(httpd_req_t *req);

    static char* get_content_type(const char* filepath);

    static esp_err_t data_get_handler(httpd_req_t *req);
    RaptPillData rapt_pill_data;

};
#endif // RAPT_PILL_BLE_HPP