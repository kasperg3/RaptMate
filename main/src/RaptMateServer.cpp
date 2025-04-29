#include "web/RaptMateServer.hpp"
#include <exception>
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
// TODO housekeeping

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

        httpd_uri_t post_uri = {
            .uri       = "/settings",  // Your endpoint
            .method    = HTTP_POST,
            .handler   = RaptMateServer::settings_post_handler,
            .user_ctx  = this->wm
        };
        httpd_register_uri_handler(server, &post_uri);

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
    // First check if it's a POST request
    if (req->method != HTTP_POST) {
        httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, "Method not allowed");
        return ESP_FAIL;
    }

    // Get content length
    int content_length = req->content_len;
    if (content_length > 1024) { // Set your max content length
        httpd_resp_send_err(req, HTTPD_413_CONTENT_TOO_LARGE, "Content too long");
        return ESP_FAIL;
    }

    // Read the content
    char *content = static_cast<char *>(malloc(content_length + 1));
    if (!content) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    int ret = httpd_req_recv(req, content, content_length);
    if (ret <= 0) {  // Returns 0 if connection closed, <0 if error
        free(content);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, "Request timeout");
        }
        return ESP_FAIL;
    }
    content[ret] = '\0'; // Null-terminate the received data

    // Process the received data (example: print it)
    ESP_LOGI(SERVER_TAG, "Received POST content: %s", content);
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        free(content);
        return ESP_FAIL;
    }

    cJSON *ssid = cJSON_GetObjectItem(json, "ssid");
    cJSON *password = cJSON_GetObjectItem(json, "password");
    
    if (cJSON_IsString(ssid) && cJSON_IsString(password)) {
        ESP_LOGI(SERVER_TAG, "SSID: %s", ssid->valuestring);
        ESP_LOGI(SERVER_TAG, "Password: %s", password->valuestring);
        WiFiManager *wifi_manager = static_cast<WiFiManager *>(req->user_ctx);

        // Set credentials in WiFiManager
        wifi_manager->setCredentials(ssid->valuestring, password->valuestring);
    } else {
        cJSON_Delete(json);
        free(content);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON format");
        return ESP_FAIL;
    }

    cJSON_Delete(json);
    // Send response
    const char *resp_str = "{\"status\":\"success\",\"received\":\"your data\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, strlen(resp_str));

    free(content);
    return ESP_OK;
}

esp_err_t RaptMateServer::index_get_handler(httpd_req_t *req)
{
    if (strcmp(req->uri, "/data") == 0)
    {
        return data_get_handler(req);
    }
    else if (strcmp(req->uri, "/reset") == 0)
    {
        RaptPillBLE *ble = static_cast<RaptPillBLE *>(req->user_ctx);
        ble->resetData();
        httpd_resp_sendstr(req, "Data reset successfully");
        return ESP_OK;
    }
    else
    {
        return static_file_get_handler(req);
    }
}

std::string RaptMateServer::formatRaptPillData(const RaptPillData &data)
{
    char json_response[512]; // Buffer for JSON response
    snprintf(json_response, sizeof(json_response),
             "{\"timestamp\": \"%lld\", \"gravity_velocity\": %f, \"temperature_celsius\": %f, \"specific_gravity\": %f, \"accel_x\": %f, \"accel_y\": %f, \"accel_z\": %f, \"battery\": %f}",
             data.timestamp, data.gravity_velocity, data.temperature_celsius, data.specific_gravity,
             data.accel_x, data.accel_y, data.accel_z, data.battery);
    return std::string(json_response);
}

esp_err_t RaptMateServer::data_get_handler(httpd_req_t *req)
{
    RaptPillBLE *ble = static_cast<RaptPillBLE *>(req->user_ctx);
    std::vector<RaptPillData> data = ble->getAllData();

    // Prepare CSV header
    std::string csv_response = "timestamp,gravity_velocity,temperature_celsius,specific_gravity,accel_x,accel_y,accel_z,battery\n";

    // Append data rows
    for (const auto &entry : data)
    {
        csv_response += std::to_string(entry.timestamp) + "," +
                        std::to_string(entry.gravity_velocity) + "," +
                        std::to_string(entry.temperature_celsius) + "," +
                        std::to_string(entry.specific_gravity) + "," +
                        std::to_string(entry.accel_x) + "," +
                        std::to_string(entry.accel_y) + "," +
                        std::to_string(entry.accel_z) + "," +
                        std::to_string(entry.battery) + "\n";
    }

    httpd_resp_set_type(req, "text/csv");

    esp_err_t send_result = httpd_resp_send(req, csv_response.c_str(), csv_response.length());
    if (send_result != ESP_OK)
    {
        ESP_LOGE(SERVER_TAG, "Error sending response: %d", send_result);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send response");
        return ESP_FAIL;
    }

    return ESP_OK;
}
