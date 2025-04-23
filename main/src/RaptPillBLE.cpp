#include "drivers/RaptPillBLE.hpp"

RaptPillBLE *RaptPillBLE::instance_ = nullptr;

QueueHandle_t dataQueue;

void RaptPillBLE::dataReceiverTask(void *param)
{
    RaptPillData receivedData;
    RaptPillBLE *self = static_cast<RaptPillBLE *>(param);
    while (true)
    {
        if (xQueueReceive(self->dataQueue, &receivedData, portMAX_DELAY))
        {
            self->m_most_recent_data.push_back(receivedData);
            if (receivedData.timestamp != 0)
            {
                // ESP_LOGI(BLE_TAG, "Updating data");

                self->writeDataToFile(receivedData);

                ESP_LOGI(BLE_TAG, "Data received and written to file");
            }
            else
            {
                ESP_LOGE(BLE_TAG, "Received data with timestamp 0, not persisting to memory.");
            }
        }
    }
}

void RaptPillBLE::writeDataToFile(const RaptPillData &data)
{
    // TODO overwrite lines instead of appending if the file is too big

    FILE *file = fopen("/data/data.csv", "a");
    if (file)
    {
        fprintf(file, "%lld,%.2f,%.2f,%.4f,%.2f,%.2f,%.2f,%.2f\n",
                data.timestamp,
                data.gravity_velocity,
                data.temperature_celsius,
                data.specific_gravity,
                data.accel_x,
                data.accel_y,
                data.accel_z,
                data.battery);
        fclose(file);
    }
    else
    {
        ESP_LOGE(BLE_TAG, "Failed to open data.csv for writing");
    }
}

RaptPillBLE::RaptPillBLE()
{
    instance_ = this;

    // Create the queue to hold RaptPillData items
    dataQueue = xQueueCreate(10, sizeof(RaptPillData));
    if (dataQueue == nullptr)
    {
        ESP_LOGE(BLE_TAG, "Failed to create data queue");
        return;
    }

    // Initialize SPIFFS for data partition
    esp_vfs_spiffs_conf_t data_conf = {
        .base_path = "/data",
        .partition_label = "data",
        .max_files = 1,
        .format_if_mount_failed = false,
    };

    // Initialize data persistence
    esp_err_t data_ret = esp_vfs_spiffs_register(&data_conf);

    if (data_ret != ESP_OK)
    {
        ESP_LOGE(BLE_TAG, "Failed to mount or format data filesystem");
    }
    else
    {
        ESP_LOGI(BLE_TAG, "Data SPIFFS mounted");
        // Check if data.csv exists, if not create it
        const char *file_path = "/data/data.csv";
        FILE *file = fopen(file_path, "r");
        if (!file)
        {
            ESP_LOGW(BLE_TAG, "data.csv not found, creating a new one");
            file = fopen(file_path, "w");
            if (file)
            {
                fclose(file);
                ESP_LOGI(BLE_TAG, "data.csv created successfully");
            }
            else
            {
                ESP_LOGE(BLE_TAG, "Failed to create data.csv");
            }
        }
        else
        {
            fclose(file);
            ESP_LOGI(BLE_TAG, "data.csv already exists");
        }
    }
    // Open the file in read mode
    this->m_most_recent_data = readAllData();
    ESP_LOGI(BLE_TAG, "Number of records loaded from CSV: %zu", this->m_most_recent_data.size());
    // Create the data receiver task
    xTaskCreate(RaptPillBLE::dataReceiverTask, "DataReceiverTask", 4096, this, 5, nullptr);
}

void RaptPillBLE::resetData()
{
    // Reset the data to default values
    m_most_recent_data = {};
    ESP_LOGI(BLE_TAG, "Data reset to default values");
    // Delete the content of the CSV file
    FILE *file = fopen("/data/data.csv", "w");
    if (file)
    {
        fclose(file);
        ESP_LOGI(BLE_TAG, "CSV content deleted");
    }
    else
    {
        ESP_LOGE(BLE_TAG, "Failed to open data.csv for resetting");
    }
}

RaptPillData RaptPillBLE::readLatestData()
{
    RaptPillData lastData = {};
    auto filename = "/data/data.csv";
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        ESP_LOGE(BLE_TAG, "Failed to open file: %s", filename);
        return lastData;
    }

    if (fseek(file, -1, SEEK_END) != 0)
    {
        ESP_LOGE(BLE_TAG, "Failed to seek to the end of the file");
        fclose(file);
        return lastData;
    }

    long filePos = ftell(file);
    std::string lastLine;
    while (filePos > 0)
    {
        char ch;
        fseek(file, --filePos, SEEK_SET);
        fread(&ch, 1, 1, file);

        if (ch == '\n' && !lastLine.empty())
        {
            break;
        }
        lastLine.insert(lastLine.begin(), ch);
    }

    fclose(file);
    if (!lastLine.empty())
    {
        sscanf(lastLine.c_str(), "%lld,%f,%f,%f,%f,%f,%f,%f",
               &lastData.timestamp,
               &lastData.gravity_velocity,
               &lastData.temperature_celsius,
               &lastData.specific_gravity,
               &lastData.accel_x,
               &lastData.accel_y,
               &lastData.accel_z,
               &lastData.battery);
    }
    return lastData;
}

std::vector<RaptPillData> RaptPillBLE::readAllData()
{
    std::vector<RaptPillData> allData;
    const char *file_path = "/data/data.csv";
    FILE *file = fopen(file_path, "r");
    if (!file)
    {
        ESP_LOGE(BLE_TAG, "Failed to open file: %s", file_path);
        return allData;
    }

    char line[256];
    
    // Read each line and parse the data
    while (fgets(line, sizeof(line), file))
    {
        RaptPillData data = {};
        if (sscanf(line, "%lld,%f,%f,%f,%f,%f,%f,%f",
                   &data.timestamp,
                   &data.gravity_velocity,
                   &data.temperature_celsius,
                   &data.specific_gravity,
                   &data.accel_x,
                   &data.accel_y,
                   &data.accel_z,
                   &data.battery) == 8)
        {
            allData.push_back(data);
        }
        else
        {
            ESP_LOGW(BLE_TAG, "Failed to parse line: %s", line);
        }
    }

    fclose(file);
    return allData;
}
RaptPillBLE::~RaptPillBLE()
{
    // Destructor implementation
}

void RaptPillBLE::init()
{
    // Initialize NimBLE host stack.
    nimble_port_init();
    ESP_ERROR_CHECK(esp_nimble_hci_init());

    // Create the FreeRTOS task
    xTaskCreate(bleHostTask, "nimble_host_task", 4096, this, 5, NULL);
    startScan();
}

void RaptPillBLE::startScan()
{
    ble_app_scan();
}

void RaptPillBLE::ble_app_scan()
{
    struct ble_gap_disc_params disc_params = {
        .itvl = 0,             // Default interval
        .window = 0,           // Default window
        .filter_policy = 0,    // Accept all advertisements
        .limited = 0,          // General discovery mode
        .passive = 1,          // Passive scanning (no scan requests)
        .filter_duplicates = 0 // Do not filter duplicate advertisements
    };

    for (int i = 0; i < 10; ++i)
    {
        int rc = ble_gap_disc(0, BLE_HS_FOREVER, &disc_params, bleGapEvent, this);
        if (rc == 0)
        {
            break;
        }
        ESP_LOGW(BLE_TAG, "Retrying discovery, attempt %d", i + 1);
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1 second before retrying
    }
}

int RaptPillBLE::parseManufacturerData(const uint8_t *data, size_t length, ble_addr_t addr)
{
    RaptPillData parsed_data = {};
    time_t now;
    time(&now);
    struct tm timeinfo;
    int64_t epoch_time = static_cast<int64_t>(now);
    parsed_data.timestamp = epoch_time;

    uint8_t version = data[4];
    if (version == 0x01)
    {
        if (last_timestamp == parsed_data.timestamp){
            // ESP_LOGI(BLE_TAG, "Duplicate timestamp detected, ignoring data");
            return 0;
        }else{
            last_timestamp = parsed_data.timestamp;
        }
    
        // Parse MAC address.
        char mac_address[18];
        snprintf(mac_address, sizeof(mac_address), "%02X:%02X:%02X:%02X:%02X:%02X",
                 data[5], data[6], data[7], data[8], data[9], data[10]);

        uint16_t temperature = (data[11] << 8) | data[12];
        float temp_celsius = temperature / 256.0f;

        // Parse specific gravity (raw value).
        uint32_t sg_raw = (data[13] << 24) | (data[14] << 16) | (data[15] << 8) | data[16];
        float sg = static_cast<float>(sg_raw);

        // Parse raw accelerometer data.
        int16_t raw_accel_x = (data[17] << 8) | data[18];
        int16_t raw_accel_y = (data[19] << 8) | data[20];
        int16_t raw_accel_z = (data[21] << 8) | data[22];

        // Scale the raw accelerometer data.
        float accel_x_scaled = raw_accel_x / 16.0f;
        float accel_y_scaled = raw_accel_y / 16.0f;
        float accel_z_scaled = raw_accel_z / 16.0f;

        // Parse battery state-of-charge (percentage * 256).
        uint16_t battery_raw = (data[23] << 8) | data[24];
        float battery = battery_raw / 256.0f;

        // Populate the struct with parsed data.
        parsed_data = {
            .timestamp = epoch_time,
            .gravity_velocity = 0.0f, // Not available in version 0x01
            .temperature_celsius = temp_celsius,
            .specific_gravity = sg,
            .accel_x = accel_x_scaled,
            .accel_y = accel_y_scaled,
            .accel_z = accel_z_scaled,
            .battery = battery};

        // Log parsed data.
        ESP_LOGI(BLE_TAG, "MAC Address: %s, Specific Gravity: %.4f, Temperature: %.2f °C, Battery: %.2f%%, Accelerometer (X, Y, Z): %.2f, %.2f, %.2f",
                 mac_address, sg, temp_celsius, battery, accel_x_scaled, accel_y_scaled, accel_z_scaled);
    }
    else if (version == 0x02)
    {
        if (last_timestamp == epoch_time){
            ESP_LOGI(BLE_TAG, "Duplicate timestamp detected, ignoring data");
            return 0;
        }else{
            last_timestamp = epoch_time;
        }
    
        // Parse gravity velocity validity.
        uint8_t cc = data[6];
        float gv = 0.0f;
        if (cc == 0x01)
        {
            // Gravity velocity is valid; parse it.
            uint32_t gv_raw = (data[7] << 24) | (data[8] << 16) | (data[9] << 8) | data[10];
            memcpy(&gv, &gv_raw, sizeof(float));
        }

        // Parse temperature (Kelvin * 128).
        uint16_t temp_raw = (data[11] << 8) | data[12];
        float temp = temp_raw / 128.0f;

        // Parse specific gravity (as a float).
        uint32_t sg_raw = (data[13] << 24) | (data[14] << 16) | (data[15] << 8) | data[16];
        float sg;
        memcpy(&sg, &sg_raw, sizeof(float));

        // Parse accelerometer data (x, y, z).
        int16_t accel_x = (data[17] << 8) | data[18];
        int16_t accel_y = (data[19] << 8) | data[20];
        int16_t accel_z = (data[21] << 8) | data[22];
        float accel_x_scaled = accel_x / 16.0f;
        float accel_y_scaled = accel_y / 16.0f;
        float accel_z_scaled = accel_z / 16.0f;

        // Parse battery state-of-charge (percentage * 256).
        uint16_t battery_raw = (data[23] << 8) | data[24];
        float battery = battery_raw / 256.0f;

        // Convert temperature to Celsius (from Kelvin).
        float temp_celsius = temp - 273.15f;

        // Populate the struct with parsed data.
        parsed_data = {
            .timestamp = epoch_time,
            .gravity_velocity = gv,
            .temperature_celsius = temp_celsius,
            .specific_gravity = sg,
            .accel_x = accel_x_scaled,
            .accel_y = accel_y_scaled,
            .accel_z = accel_z_scaled,
            .battery = battery};

        // Log the parsed data.
        ESP_LOGI(BLE_TAG, "Timestamp: %lld,Gravity Velocity Valid: %s, Gravity Velocity: %.2f points/day, Temperature: %.2f °C, Specific Gravity: %.4f, "
                          "Accelerometer (X, Y, Z): %.2f, %.2f, %.2f, Battery: %.2f%%",
                 parsed_data.timestamp,
                 cc == 0x01 ? "Yes" : "No", gv, temp_celsius, sg,
                 accel_x_scaled, accel_y_scaled, accel_z_scaled, battery);
    }
    else
    {
        return -1; // Unknown version.
    }


    // Send the parsed data to the queue.
    if (xQueueSend(dataQueue, &parsed_data, portMAX_DELAY) != pdPASS)
    {
        ESP_LOGE(BLE_TAG, "Failed to send data to the queue");
    }

    return 0;
}

int RaptPillBLE::bleGapEvent(struct ble_gap_event *event, void *arg)
{
    RaptPillBLE *self = static_cast<RaptPillBLE *>(arg);
    if (self)
    {
        return self->handleBleGapEvent(event);
    }
    else
    {
        ESP_LOGE(BLE_TAG, "Instance pointer is null!");
        return -1;
    }
}

int RaptPillBLE::handleBleGapEvent(struct ble_gap_event *event)
{
    switch (event->type)
    {
    case BLE_GAP_EVENT_DISC:
    {
        struct ble_hs_adv_fields fields;
        int rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);

        // TODO figure out why this is failing to parse
        // if (rc != 0) {
        //     ESP_LOGE(BLE_TAG, "Failed to parse advertisement data");
        //     return 0;
        // }
        if (fields.mfg_data != nullptr && fields.mfg_data_len == 25)
        {
            parseManufacturerData(fields.mfg_data, fields.mfg_data_len, event->disc.addr);
        }
        break;
    }
    case BLE_GAP_EVENT_DISC_COMPLETE:
        ESP_LOGI(BLE_TAG, "Discovery complete");
        break;
    default:
        ESP_LOGI(BLE_TAG, "Unhandled event: %d", event->type);
        break;
    }
    return 0;
}

void RaptPillBLE::bleHostTask(void *args)
{
    // Retrieve the instance pointer stored in the static member.
    RaptPillBLE *self = static_cast<RaptPillBLE *>(args);
    if (!self)
    {
        ESP_LOGE(BLE_TAG, "Instance pointer is null!");
        return;
    }

    ESP_LOGI(BLE_TAG, "BLE Host Task Started");

    // Register a BLE GAP event listener using the instance pointer.
    ble_gap_event_listener listener = {
        .fn = bleGapEvent,
        .arg = self,
        .link = {} // Zero-initialize the link field.
    };

    ble_gap_event_listener_register(&listener, bleGapEvent, self);

    // Run the NimBLE event loop (this call blocks until nimble_port_stop() is called).
    nimble_port_run();

    // Cleanup (unreachable in this example).
    nimble_port_freertos_deinit();
}