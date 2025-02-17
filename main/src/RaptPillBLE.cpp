#include "drivers/RaptPillBLE.hpp"

RaptPillBLE *RaptPillBLE::instance_ = nullptr;
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

QueueHandle_t dataQueue;

void RaptPillBLE::dataReceiverTask(void *param) {
    RaptPillData receivedData;
    RaptPillBLE *self = static_cast<RaptPillBLE*>(param);
    while (true) {
        if (xQueueReceive(self->dataQueue, &receivedData, portMAX_DELAY)) {
            self->m_most_recent_data = receivedData;
            ESP_LOGI(BLE_TAG, "Updating data");
        }
    }
}

RaptPillBLE::RaptPillBLE() {
    instance_ = this;

    // Create the queue to hold RaptPillData items
    dataQueue = xQueueCreate(10, sizeof(RaptPillData));
    if (dataQueue == nullptr) {
        ESP_LOGE(BLE_TAG, "Failed to create data queue");
        return;
    }

    // Create the data receiver task
    xTaskCreate(RaptPillBLE::dataReceiverTask, "DataReceiverTask", 4096, this, 5, nullptr);
        
}

RaptPillBLE::~RaptPillBLE() {
    // Destructor implementation
}

void RaptPillBLE::init() {
    // Initialize NimBLE host stack.
    nimble_port_init();
    ESP_ERROR_CHECK(esp_nimble_hci_init());

    // Create the FreeRTOS task
    xTaskCreate(bleHostTask, "nimble_host_task", 4096, this, 5, NULL);

}

void RaptPillBLE::startScan() {
    ble_app_scan();
}

void RaptPillBLE::ble_app_scan() {
    struct ble_gap_disc_params disc_params = {
        .itvl = 0,             // Default interval
        .window = 0,           // Default window
        .filter_policy = 0,    // Accept all advertisements
        .limited = 0,          // General discovery mode
        .passive = 1,          // Passive scanning (no scan requests)
        .filter_duplicates = 0 // Do not filter duplicate advertisements
    };

    ESP_LOGI(BLE_TAG, "Starting discovery");
    int rc = ble_gap_disc(0, BLE_HS_FOREVER, &disc_params, bleGapEvent, this);
    if (rc != 0) {
        ESP_LOGE(BLE_TAG, "Error starting discovery: %d", rc);
    }
}

int RaptPillBLE::parseManufacturerData(const uint8_t *data, size_t length, ble_addr_t addr) {
    uint8_t version = data[4];
    if (version == 0x01) {
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
        // Log parsed data.
        ESP_LOGI(BLE_TAG, "MAC Address: %s, Specific Gravity: %.4f, Temperature: %.2f °C, Battery: %.2f%%, Accelerometer (X, Y, Z): %.2f, %.2f, %.2f",
                 mac_address, sg, temp_celsius, battery, accel_x_scaled, accel_y_scaled, accel_z_scaled);

    }
    else if (version == 0x02) {
        // Parse gravity velocity validity.
        uint8_t cc = data[6];
        float gv = 0.0f;
        if (cc == 0x01) {
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
        RaptPillData parsed_data = {
            .gravity_velocity = gv,
            .temperature_celsius = temp_celsius,
            .specific_gravity = sg,
            .accel_x = accel_x_scaled,
            .accel_y = accel_y_scaled,
            .accel_z = accel_z_scaled,
            .battery = battery
        };
        
        if (xQueueSend(dataQueue, &parsed_data, portMAX_DELAY) != pdPASS) {
            ESP_LOGE(BLE_TAG, "Failed to send data to the queue");
        }

        // Log the parsed data.
        ESP_LOGI(BLE_TAG, "Gravity Velocity Valid: %s, Gravity Velocity: %.2f points/day, Temperature: %.2f °C, Specific Gravity: %.4f, "
                      "Accelerometer (X, Y, Z): %.2f, %.2f, %.2f, Battery: %.2f%%",
                 cc == 0x01 ? "Yes" : "No", gv, temp_celsius, sg,
                 accel_x_scaled, accel_y_scaled, accel_z_scaled, battery);
    }
    else {
        return -1; // Unknown version.
    }

    return 0;
}

int RaptPillBLE::bleGapEvent(struct ble_gap_event *event, void *arg) {
    RaptPillBLE *self = static_cast<RaptPillBLE*>(arg);
    if (self) {
        return self->handleBleGapEvent(event);
    } else {
        ESP_LOGE(BLE_TAG, "Instance pointer is null!");
        return -1;
    }
}

int RaptPillBLE::handleBleGapEvent(struct ble_gap_event *event) {
    switch (event->type) {
        case BLE_GAP_EVENT_DISC: {
            struct ble_hs_adv_fields fields;
            int rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
            if (rc != 0) {
                ESP_LOGE(BLE_TAG, "Failed to parse advertisement data");
                return 0;
            }
            if (fields.mfg_data != nullptr && fields.mfg_data_len == 25) {
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

void RaptPillBLE::bleHostTask(void *args) {
    // Retrieve the instance pointer stored in the static member.
    RaptPillBLE *self = static_cast<RaptPillBLE*>(args);
    if (!self) {
        ESP_LOGE(BLE_TAG, "Instance pointer is null!");
        return;
    }

    ESP_LOGI(BLE_TAG, "BLE Host Task Started");

    // Register a BLE GAP event listener using the instance pointer.
    ble_gap_event_listener listener = {
        .fn = bleGapEvent,
        .arg = self,
        .link = {}  // Zero-initialize the link field.
    };
    
    ble_gap_event_listener_register(&listener, bleGapEvent, self);

    // Run the NimBLE event loop (this call blocks until nimble_port_stop() is called).
    nimble_port_run();

    // Cleanup (unreachable in this example).
    nimble_port_freertos_deinit();
}