#ifndef RAPT_PILL_BLE_HPP
#define RAPT_PILL_BLE_HPP

#include <cstdio>
#include <cstring>
#include <string>
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
#include "common/core.hpp"
#include "esp_spiffs.h"
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <time.h>
#include <sys/time.h>
#include <string>
#include "esp_bt.h"

#define BLE_TAG "BLE"

class RaptPillBLE
{
public:
    RaptPillBLE();
    static void dataReceiverTask(void *param);
    QueueHandle_t dataQueue;
    ~RaptPillBLE();

    void init();
    void startScan();

    RaptPillData getLatestData()
    {
        return m_most_recent_data.back();
    }

    std::vector<RaptPillData> getAllData()
    {
        return m_most_recent_data;
    }

    void resetData();

private:
    void ble_app_scan();
    RaptPillData readLatestData();
    std::vector<RaptPillData> readAllData();
    static std::string getCsvHeader();
    int parseManufacturerData(const uint8_t *data, size_t length, ble_addr_t addr);
    static int bleGapEvent(struct ble_gap_event *event, void *arg);
    int handleBleGapEvent(struct ble_gap_event *event);
    static std::string formatDataAsString(const RaptPillData &data);
    void createFileIfNotExist(const char *filename);
    static void writeToFile(const char *filepath, const std::string &content);
    static void bleHostTask(void *);
    std::vector<RaptPillData> m_most_recent_data = std::vector<RaptPillData>();
    static RaptPillBLE *instance_;
    int64_t last_timestamp = 0;
};

#endif // RAPT_PILL_BLE_HPP