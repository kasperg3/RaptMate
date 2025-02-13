#include "web/RaptMateServer.hpp"

static const char* SERVER_TAG = "RaptMateServer";


void RaptMateServer::init() {
    init_wifi();
    // We start the HTTP server immediately. In this example,
    // mDNS is (re)initialized once an IP is acquired.
    init_http_server();
}

void RaptMateServer::init_wifi() {
    ESP_LOGI(SERVER_TAG, "Initializing Wi‑Fi in STA mode");
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_ps(WIFI_PS_NONE);

    // Register event handlers for Wi‑Fi and IP events.
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &RaptMateServer::wifi_event_handler,
                                        this,
                                        &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &RaptMateServer::wifi_event_handler,
                                        this,
                                        &instance_got_ip);

    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(SERVER_TAG, "Wi‑Fi initialized. Connecting to SSID: %s", WIFI_SSID);
    esp_wifi_connect();
}

void RaptMateServer::init_mdns() {
    ESP_LOGI(SERVER_TAG, "Initializing mDNS with hostname: raptmate.local");
    esp_err_t err = mdns_init();
    if (err) {
        ESP_LOGE(SERVER_TAG, "MDNS Init failed: %d", err);
        return;
    }
    // The hostname "raptmate" makes the device reachable as raptmate.local.
    mdns_hostname_set("raptmate");
    mdns_instance_name_set("RaptMate Device");
    ESP_LOGI(SERVER_TAG, "mDNS initialized: device is available as raptmate.local");
}

void RaptMateServer::init_http_server() {
    ESP_LOGI(SERVER_TAG, "Starting HTTP Server");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        // Register the root URI handler that serves the HTML page.
        httpd_uri_t index_uri = {};
        index_uri.uri = "/";
        index_uri.method = HTTP_GET;
        index_uri.handler = RaptMateServer::index_get_handler;
        index_uri.user_ctx = this;
        httpd_register_uri_handler(server, &index_uri);

        // Register the /data URI handler that returns JSON data.
        httpd_uri_t data_uri = {};
        data_uri.uri = "/data";
        data_uri.method = HTTP_GET;
        data_uri.handler = RaptMateServer::data_get_handler;
        data_uri.user_ctx = this->ble;
        httpd_register_uri_handler(server, &data_uri);

        ESP_LOGI(SERVER_TAG, "HTTP Server started");
    } else {
        ESP_LOGE(SERVER_TAG, "Error starting HTTP Server!");
    }
}

void RaptMateServer::wifi_event_handler(void* arg, esp_event_base_t event_base,
                                          int32_t event_id, void* event_data) {
    RaptMateServer* self = static_cast<RaptMateServer*>(arg);
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(SERVER_TAG, "Wi‑Fi disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(SERVER_TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        // Initialize mDNS now that an IP address is available.
        self->init_mdns();
    }
}

esp_err_t RaptMateServer::index_get_handler(httpd_req_t *req) {
    const char* html_content =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "  <meta charset='utf-8'>"
        "  <title>RaptMate BLE Data</title>"
        "  <style>"
        "    body { font-family: Arial, sans-serif; margin: 0; padding: 0; background-color: #f4f4f9; color: #333; }"
        "    h1 { background-color: #4CAF50; color: white; padding: 20px; text-align: center; margin: 0; }"
        "    #data { padding: 20px; text-align: center; font-size: 1.5em; }"
        "    .data-item { background-color: #fff; border-radius: 8px; box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1); margin: 10px auto; padding: 20px; max-width: 600px; }"
        "  </style>"
        "</head>"
        "<body>"
        "  <h1>RaptMate BLE Data</h1>"
        "  <div id='data'>Loading...</div>"
        "  <script>"
        "    function fetchData() {"
        "      fetch('/data')"
        "        .then(response => response.json())"
        "        .then(data => {"
        "          document.getElementById('data').innerHTML = "
        "            '<div class=\"data-item\">Gravity Velocity: ' + data.gravity_velocity + '</div>' +"
        "            '<div class=\"data-item\">Temperature (Celsius): ' + data.temperature_celsius + '</div>' +"
        "            '<div class=\"data-item\">Specific Gravity: ' + data.specific_gravity + '</div>' +"
        "            '<div class=\"data-item\">Acceleration X: ' + data.accel_x + '</div>' +"
        "            '<div class=\"data-item\">Acceleration Y: ' + data.accel_y + '</div>' +"
        "            '<div class=\"data-item\">Acceleration Z: ' + data.accel_z + '</div>' +"
        "            '<div class=\"data-item\">Battery: ' + data.battery + '</div>';"
        "        })"
        "        .catch(err => console.error('Error:', err));"
        "    }"
        "    setInterval(fetchData, 1000);"
        "    fetchData();"
        "  </script>"
        "</body>"
        "</html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_content, strlen(html_content));
    return ESP_OK;
}

esp_err_t RaptMateServer::data_get_handler(httpd_req_t *req) {
    RaptPillBLE* ble = static_cast<RaptPillBLE*>(req->user_ctx);
    RaptPillData data = ble->getData();
    char json_response[256];
    snprintf(json_response, sizeof(json_response), "{\"gravity_velocity\": %f, \"temperature_celsius\": %f, \"specific_gravity\": %f, \"accel_x\": %f, \"accel_y\": %f, \"accel_z\": %f, \"battery\": %f}", data.gravity_velocity, data.temperature_celsius, data.specific_gravity, data.accel_x, data.accel_y, data.accel_z, data.battery);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, strlen(json_response));
    return ESP_OK;
}

