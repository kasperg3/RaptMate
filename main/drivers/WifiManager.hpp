#include <cstdio>
#include <cstring>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "web/RaptMateServer.hpp"
#include "drivers/RaptPillBLE.hpp"
class WiFiManager
{
public:
    WiFiManager() = default;

    void init()
    {
        ESP_LOGI("WiFiManager", "Initializing Wi‑Fi in APSTA mode");
        esp_netif_init();
        esp_event_loop_create_default();
        esp_netif_create_default_wifi_sta();
        esp_netif_create_default_wifi_ap();
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);
        esp_wifi_set_ps(WIFI_PS_NONE);
        registerEventHandlers();

        configureAP();
        configureSTA();

        esp_err_t err = esp_wifi_start();
        if (err != ESP_OK)
        {
            ESP_LOGE("WiFiManager", "Failed to start Wi-Fi: %s", esp_err_to_name(err));
            return;
        }
    }

    // Destructor to unregister the handler
    ~WiFiManager()
    {
        // TODO housekeeping, unregister event handlers, etc.
    }

private:
    void configureAP()
    {
        wifi_config_t wifi_config_ap = {
            .ap = {
                .ssid = "RaptMate",
                .password = "RaptMate",
                .ssid_len = strlen("RaptMate"),
                .authmode = WIFI_AUTH_WPA2_PSK,
                .max_connection = 4,
                .pmf_cfg = {
                    .capable = true,
                    .required = false}}};
        esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap);
    }

    void configureSTA()
    {
        wifi_config_t wifi_config_sta = {
            .sta = {
                .ssid = WIFI_SSID,
                .password = WIFI_PASS,
            }};
        ESP_LOGI("WiFiManager", "Configuring Wi-Fi SSID: %s", WIFI_SSID);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta);
    }

    void registerEventHandlers()
    {
        ESP_LOGI("WiFiManager", "Registering Wi-Fi event handlers");
        esp_event_handler_instance_t instance_got_ip;
        esp_err_t err = esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &WiFiManager::onGotIP,
                                                            NULL,
                                                            &instance_got_ip);

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_register(WIFI_EVENT,
                                            ESP_EVENT_ANY_ID,
                                            &WiFiManager::wifi_event_handler,
                                            NULL,
                                            &instance_any_id);
        if (err != ESP_OK)
        {
            ESP_LOGE("WiFiManager", "Failed to register event handler: %s", esp_err_to_name(err));
        }
    }

    static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
    {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
        {
            ESP_LOGI("WiFiManager", "Wi‑Fi started, connecting to AP...");
            esp_wifi_connect();
        }
        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            ESP_LOGW("WiFiManager", "Wi‑Fi disconnected, retrying...");
            esp_wifi_connect();
        }
    }

    static void onGotIP(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI("WiFiManager", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        auto *self = static_cast<WiFiManager *>(arg);
        initMDNS();
        initSNTP();
    }

    static void initMDNS()
    {
        ESP_LOGI("WiFiManager", "Initializing mDNS with hostname: raptmate.local");
        esp_err_t err = mdns_init();
        if (err)
        {
            ESP_LOGE("WiFiManager", "MDNS Init failed: %d", err);
            return;
        }
        mdns_hostname_set("raptmate");
        mdns_instance_name_set("RaptMate Device");
        ESP_LOGI("WiFiManager", "mDNS initialized: device is available as raptmate.local");
    }

    static void initSNTP()
    {
        ESP_LOGI("WiFiManager", "Initializing SNTP");
        sntp_init();
        sntp_setservername(0, "pool.ntp.org");
        sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
        sntp_set_sync_interval(3600000);

        for (int attempt = 0; attempt < 10; ++attempt)
        {
            time_t now = 0;
            struct tm timeinfo = {0};
            time(&now);
            localtime_r(&now, &timeinfo);

            if (timeinfo.tm_year >= (2016 - 1900))
            {
                ESP_LOGI("WiFiManager", "System time set successfully: %04d-%02d-%02d %02d:%02d:%02d",
                         timeinfo.tm_year + 1900,
                         timeinfo.tm_mon + 1,
                         timeinfo.tm_mday,
                         timeinfo.tm_hour,
                         timeinfo.tm_min,
                         timeinfo.tm_sec);
                break;
            }

            ESP_LOGI("WiFiManager", "Attempt %d: Waiting for system time to be set...", attempt + 1);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
};