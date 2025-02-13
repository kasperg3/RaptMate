#ifndef RAPT_PILL_BLE_HPP
#define RAPT_PILL_BLE_HPP

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
#include "common/core.hpp"
#define BLE_TAG "BLE"

class RaptPillBLE {
public:
    RaptPillBLE();
    static void dataReceiverTask(void *param);
    QueueHandle_t dataQueue;
    ~RaptPillBLE();

    void init();
    void startScan();

    RaptPillData getData() {
        return m_data;
    }

private:
    void ble_app_scan();
    int parseManufacturerData(const uint8_t *data, size_t length, ble_addr_t addr);
    static int bleGapEvent(struct ble_gap_event *event, void *arg);
    int handleBleGapEvent(struct ble_gap_event *event);
    static void bleHostTask(void *);
    RaptPillData m_data = {};
    static RaptPillBLE *instance_;
};

#endif // RAPT_PILL_BLE_HPP