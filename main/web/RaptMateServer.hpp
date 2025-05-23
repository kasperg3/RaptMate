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
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "time.h"
#include "cJSON.h"
#include "drivers/WifiManager.hpp"

class RaptMateServer {
public:
    RaptMateServer(RaptPillBLE* ble, WiFiManager* wm) : server(NULL), ble(ble), wm(wm) {}
    void init();
    ~RaptMateServer();

    RaptPillData get_data() { return rapt_pill_data; }
private:

    httpd_handle_t server;
    RaptPillBLE* ble;
    WiFiManager* wm;
    void init_http_server();

    // HTTP URI handlers.
    static esp_err_t index_get_handler(httpd_req_t *req);
    static esp_err_t static_file_get_handler(httpd_req_t *req);
    static esp_err_t settings_post_handler(httpd_req_t *req);
    static std::string formatRaptPillData(const RaptPillData &data);
    static char* get_content_type(const char* filepath);

    static esp_err_t data_get_handler(httpd_req_t *req);
    RaptPillData rapt_pill_data;

};
#endif // RAPT_PILL_BLE_HPP