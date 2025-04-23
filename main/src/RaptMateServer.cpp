#include "web/RaptMateServer.hpp"

static const char *SERVER_TAG = "RaptMateServer";

void RaptMateServer::init()
{
    // We start the HTTP server immediately. In this example,
    // mDNS is (re)initialized once an IP is acquired.
    init_http_server();

    // Initialize SPIFFS for react app
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/web",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = false,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(SERVER_TAG, "Failed to mount or format filesystem");
    }
    else
    {
        ESP_LOGI(SERVER_TAG, "SPIFFS mounted");
    }
}

RaptMateServer::~RaptMateServer()
{
    ESP_LOGI(SERVER_TAG, "Deinitializing Wi-Fi and SNTP");

    // Deinitialize SNTP
    esp_netif_sntp_deinit();

    // Stop Wi-Fi
    esp_wifi_stop();
    esp_wifi_deinit();
}

void RaptMateServer::init_http_server()
{
    ESP_LOGI(SERVER_TAG, "Starting HTTP Server");
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Register the root URI handler that serves the HTML page.

        httpd_uri_t index_uri = {};
        index_uri.uri = "/*";
        index_uri.method = HTTP_GET;
        index_uri.handler = RaptMateServer::index_get_handler;
        index_uri.user_ctx = this->ble;
        httpd_register_uri_handler(server, &index_uri);

        ESP_LOGI(SERVER_TAG, "HTTP Server started");
    }
    else
    {
        ESP_LOGE(SERVER_TAG, "Error starting HTTP Server!");
    }
}

char *RaptMateServer::get_content_type(const char *filepath)
{
    const char *ext = strrchr(filepath, '.');
    if (!ext)
    {
        return "text/plain";
    }
    if (strcmp(ext, ".html") == 0)
    {
        return "text/html";
    }
    else if (strcmp(ext, ".css") == 0)
    {
        return "text/css";
    }
    else if (strcmp(ext, ".js") == 0)
    {
        return "application/javascript";
    }
    else if (strcmp(ext, ".png") == 0)
    {
        return "image/png";
    }
    else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
    {
        return "image/jpeg";
    }
    return "text/plain";
}

esp_err_t RaptMateServer::static_file_get_handler(httpd_req_t *req)
{
    char filepath[600];
    if (strcmp(req->uri, "/") == 0)
    {
        snprintf(filepath, sizeof(filepath), "/web/index.html");
    }
    else
    {
        snprintf(filepath, sizeof(filepath), "/web%s", req->uri);
    }

    ESP_LOGI(SERVER_TAG, "File path: %s", filepath);
    // Open the file for reading
    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        ESP_LOGE(SERVER_TAG, "Failed to open file: %s", filepath);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    // Send file content in chunks
    char buffer[1024];
    size_t read_bytes;
    httpd_resp_set_type(req, get_content_type(filepath));
    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
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

    if (total_len >= sizeof(content))
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Content too long");
        return ESP_FAIL;
    }

    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, content + cur_len, total_len - cur_len);
        if (received <= 0)
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive post data");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    content[total_len] = '\0';

    cJSON *json = cJSON_Parse(content);
    if (json == NULL)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    // TODO create the real settings
    // Parse and apply settings from JSON
    cJSON *setting1 = cJSON_GetObjectItem(json, "setting1");
    cJSON *setting2 = cJSON_GetObjectItem(json, "setting2");

    if (cJSON_IsString(setting1) && cJSON_IsNumber(setting2))
    {
        // Apply settings
        ESP_LOGI(SERVER_TAG, "Setting1: %s", setting1->valuestring);
        ESP_LOGI(SERVER_TAG, "Setting2: %d", setting2->valueint);
        // TODO Add code to apply settings here
    }
    else
    {
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
    if (strcmp(req->uri, "/data") == 0)
    {
        return data_get_handler(req);
    }
    else if (strcmp(req->uri, "/settings") == 0)
    {
        return settings_post_handler(req);
    }
    else
    {
        return static_file_get_handler(req);
    }
}

std::string RaptMateServer::formatRaptPillData(const RaptPillData &data)
{
    char json_response[512]; // Buffer for JSON response
    char timestamp_str[64];
    // time_t raw_time = static_cast<time_t>(data.timestamp);
    // struct tm time_info;
    // localtime_r(&raw_time, &time_info);
    // strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", &time_info);

    snprintf(json_response, sizeof(json_response),
             "{\"timestamp\": \"%lld\", \"gravity_velocity\": %f, \"temperature_celsius\": %f, \"specific_gravity\": %f, \"accel_x\": %f, \"accel_y\": %f, \"accel_z\": %f, \"battery\": %f}",
             data.timestamp, data.gravity_velocity, data.temperature_celsius, data.specific_gravity,
             data.accel_x, data.accel_y, data.accel_z, data.battery);

    return std::string(json_response);
}
esp_err_t RaptMateServer::data_get_handler(httpd_req_t *req)
{
    RaptPillBLE *ble = static_cast<RaptPillBLE *>(req->user_ctx);
    RaptPillData data = ble->getData();
    
    auto json_response = RaptMateServer::formatRaptPillData(data);
    // Send JSON response
    ESP_LOGI(SERVER_TAG, "Response: %s", json_response.c_str());
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response.c_str(), json_response.length());
    return ESP_OK;
}
