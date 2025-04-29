#ifndef WIFI_MANAGER_HPP
#define WIFI_MANAGER_HPP

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
#include "esp_mac.h"
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
        wifi_init_softap();
    }

    void wifi_init_softap()
    {
        esp_netif_init();
        esp_event_loop_create_default();
        esp_netif_create_default_wifi_ap();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // always start with this

        esp_wifi_init(&cfg);
        esp_wifi_set_ps(WIFI_PS_NONE);
        registerEventHandlers();
        // TODO read the config file and determine whether start in AP or APSTA mode.
        esp_wifi_set_mode(WIFI_MODE_AP);
        configureAP();
        // configureSTA("", "");
        
        esp_err_t err = esp_wifi_start();
        if (err != ESP_OK)
        {
            ESP_LOGE("WiFiManager", "Failed to start Wi-Fi: %s", esp_err_to_name(err));
            return;
        }
        // Wait until the wifi is started 
        // esp_wifi_scan_start();
        esp_wifi_connect();
    }

    // Destructor to unregister the handler
    ~WiFiManager()
    {
        // TODO housekeeping, unregister event handlers, etc.
    }

    void setCredentials(const char *ssid, const char *password = nullptr)
    {
        // Stop Wi-Fi if it's already running
        esp_wifi_stop();

        // Change mode to APSTA
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

        // Reapply the original AP configuration
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));
        configureSTA(ssid, password);
        esp_err_t err = esp_wifi_start();
        if (err != ESP_OK)
        {
            ESP_LOGE("WiFiManager", "Failed to start Wi-Fi: %s", esp_err_to_name(err));
            return;
        }
        // Wait for a brief moment to ensure the Wi-Fi is ready
        vTaskDelay(pdMS_TO_TICKS(100));
        // Connect to the new network
        err = esp_wifi_connect();
        if (err != ESP_OK)
        {
            ESP_LOGE("WiFiManager", "Failed to initiate connection: %s", esp_err_to_name(err));
        }
        else
        {
            ESP_LOGI("WiFiManager", "STA configuration updated, connecting to: %s", ssid);
        }
    }

private:
    bool m_isConnected = false;

    wifi_config_t wifi_config_ap = {};

    void configureAP()
    {
        wifi_config_ap = {
            .ap = {
                .ssid = "RaptMate",
                .ssid_len = strlen("RaptMate"),
                .channel = 6,
                .authmode = WIFI_AUTH_OPEN,
                .max_connection = 4,
                .beacon_interval = 500,
                .pmf_cfg = {
                    .capable = true,
                    .required = false,
                },
            }};

        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));
    }

    /**
     * @brief Configure STA WiFi credentials and connect
     * @param ssid Network SSID (1-32 characters)
     * @param password Network password (8-64 characters, empty for open networks)
     * @return true if configuration was successful, false otherwise
     */
    bool configureSTA(const char *ssid, const char *password = nullptr)
    {   


        // Validate SSID
        if (ssid == nullptr || strlen(ssid) == 0 || strlen(ssid) > 32)
        {
            ESP_LOGE("WiFiManager", "Invalid SSID length (must be 1-32 chars)");
            return false;
        }

        // Validate password if provided
        if (password != nullptr && strlen(password) > 0)
        {
            if (strlen(password) < 8)
            {
                ESP_LOGE("WiFiManager", "Password too short (min 8 chars)");
                return false;
            }
            if (strlen(password) > 64)
            {
                ESP_LOGE("WiFiManager", "Password too long (max 64 chars)");
                return false;
            }
        }

        // Prepare new configuration
        wifi_config_t wifi_config = {};
        strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), ssid, sizeof(wifi_config.sta.ssid) - 1);
        wifi_config.sta.ssid[sizeof(wifi_config.sta.ssid) - 1] = '\0'; // Ensure null-termination

        if (password != nullptr && strlen(password) > 0)
        {
            strncpy(reinterpret_cast<char *>(wifi_config.sta.password), password, sizeof(wifi_config.sta.password) - 1);
            wifi_config.sta.password[sizeof(wifi_config.sta.password) - 1] = '\0'; // Ensure null-termination
        }
        else
        {
            wifi_config.sta.password[0] = '\0'; // Empty password for open networks
        }

        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.pmf_cfg.capable = true;
        wifi_config.sta.pmf_cfg.required = false;
        // Apply configuration
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

        return true;
    }

    void registerEventHandlers()
    {
        ESP_LOGI("WiFiManager", "Registering Wi-Fi event handlers");
        esp_event_handler_instance_t instance_got_ip;
        esp_err_t err = esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &WiFiManager::onGotIP,
                                                            this,
                                                            &instance_got_ip);

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_register(WIFI_EVENT,
                                            ESP_EVENT_ANY_ID,
                                            &WiFiManager::wifi_event_handler,
                                            this,
                                            &instance_any_id);
        if (err != ESP_OK)
        {
            ESP_LOGE("WiFiManager", "Failed to register event handler: %s", esp_err_to_name(err));
        }
    }

    static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
    {
        WiFiManager *self = static_cast<WiFiManager *>(arg);

        if (event_base == WIFI_EVENT)
        {
            switch (event_id)
            {
            case WIFI_EVENT_STA_CONNECTED:
                self->m_isConnected = true;
                ESP_LOGI("WiFiManager", "Connected to AP");
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                self->m_isConnected = false;
                ESP_LOGW("WiFiManager", "Disconnected from AP");
                // Auto-reconnect
                static int reconnect_attempts = 0;
                if (reconnect_attempts < 30)
                {
                    ESP_LOGI("WiFiManager", "Reconnecting to AP (attempt %d)", reconnect_attempts + 1);
                    esp_wifi_connect();
                    reconnect_attempts++;
                }
                else
                {
                    ESP_LOGE("WiFiManager", "Max reconnect attempts reached. Stopping reconnection.");
                    reconnect_attempts = 0; // Reset for future use
                }
                break;

            case WIFI_EVENT_STA_START:
                ESP_LOGI("WiFiManager", "STA started");
                break;
            }
        }
    }

    static void onGotIP(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
    {
        WiFiManager *self = static_cast<WiFiManager *>(arg);
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI("WiFiManager", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        self->initMDNS();
        self->initSNTP();
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

#endif // WIFI_MANAGER_HPP