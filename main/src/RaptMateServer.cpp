#include "web/RaptMateServer.hpp"
#include "cJSON.h"

static const char* SERVER_TAG = "RaptMateServer";


void RaptMateServer::init() {

    
    init_wifi();
    // We start the HTTP server immediately. In this example,
    // mDNS is (re)initialized once an IP is acquired.
    init_http_server();

    // Initialize SPIFFS for react app
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = false,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(SERVER_TAG, "Failed to mount or format filesystem");
    }else{
        ESP_LOGI(SERVER_TAG, "SPIFFS mounted");

    }
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
    config.uri_match_fn =httpd_uri_match_wildcard;
    if (httpd_start(&server, &config) == ESP_OK) {
        // Register the root URI handler that serves the HTML page.
        
        httpd_uri_t index_uri = {};
        index_uri.uri = "/*";
        index_uri.method = HTTP_GET;
        index_uri.handler = RaptMateServer::index_get_handler;
        index_uri.user_ctx = this->ble;
        httpd_register_uri_handler(server, &index_uri);

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

char* RaptMateServer::get_content_type(const char* filepath) {
    const char* ext = strrchr(filepath, '.');
    if (!ext) {
        return "text/plain";
    }
    if (strcmp(ext, ".html") == 0) {
        return "text/html";
    } else if (strcmp(ext, ".css") == 0) {
        return "text/css";
    } else if (strcmp(ext, ".js") == 0) {
        return "application/javascript";
    } else if (strcmp(ext, ".png") == 0) {
        return "image/png";
    } else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) {
        return "image/jpeg";
    }
    return "text/plain";
}

esp_err_t RaptMateServer::static_file_get_handler(httpd_req_t *req){
    char filepath[600];
    if (strcmp(req->uri, "/") == 0) {
        snprintf(filepath, sizeof(filepath), "/spiffs/index.html");
    } else {
        snprintf(filepath, sizeof(filepath), "/spiffs%s", req->uri);
    }
    
    ESP_LOGI(SERVER_TAG, "File path: %s", filepath);
    // Open the file for reading
    FILE *file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGE(SERVER_TAG, "Failed to open file: %s", filepath);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    // Send file content in chunks
    char buffer[1024];
    size_t read_bytes;
    httpd_resp_set_type(req, get_content_type(filepath));
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        httpd_resp_send_chunk(req, buffer, read_bytes);
    }
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0); // End response
    return ESP_OK;
}

esp_err_t RaptMateServer::settings_post_handler(httpd_req_t *req)
{
    char content[256];
    int total_len = req->content_len;
    int cur_len = 0;
    int received = 0;

    if (total_len >= sizeof(content)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long");
        return ESP_FAIL;
    }

    while (cur_len < total_len) {
        received = httpd_req_recv(req, content + cur_len, total_len - cur_len);
        if (received <= 0) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive post data");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    content[total_len] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    // TODO create the real settings 
    // Parse and apply settings from JSON
    cJSON *setting1 = cJSON_GetObjectItem(json, "setting1");
    cJSON *setting2 = cJSON_GetObjectItem(json, "setting2");

    if (cJSON_IsString(setting1) && cJSON_IsNumber(setting2)) {
        // Apply settings
        ESP_LOGI(SERVER_TAG, "Setting1: %s", setting1->valuestring);
        ESP_LOGI(SERVER_TAG, "Setting2: %d", setting2->valueint);
        // Add code to apply settings here
    } else {
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid settings format");
        return ESP_FAIL;
    }

    cJSON_Delete(json);
    httpd_resp_sendstr(req, "Settings updated successfully");
    return ESP_OK;
}

esp_err_t RaptMateServer::index_get_handler(httpd_req_t *req)
{
    // Map URI to SPIFFS file path. For root, serve index.html.
    if (strcmp(req->uri, "/data") == 0){
        return data_get_handler(req);
    } else if(strcmp(req->uri, "/settings") == 0) {
        return settings_post_handler(req);
    } else {
        return static_file_get_handler(req);
    }
}

esp_err_t RaptMateServer::data_get_handler(httpd_req_t *req) {
    RaptPillBLE* ble = static_cast<RaptPillBLE*>(req->user_ctx);
    RaptPillData data = ble->getData();
    char json_response[256];
    snprintf(json_response, sizeof(json_response), "{\"gravity_velocity\": %f, \"temperature_celsius\": %f, \"specific_gravity\": %f, \"accel_x\": %f, \"accel_y\": %f, \"accel_z\": %f, \"battery\": %f}", data.gravity_velocity, data.temperature_celsius, data.specific_gravity, data.accel_x, data.accel_y, data.accel_z, data.battery);
    ESP_LOGI(SERVER_TAG, "Data: %s", json_response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, strlen(json_response));
    return ESP_OK;
}

