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

// TODO move into seperate files
#include "web/RaptMateServer.hpp"
#include "drivers/RaptPillBLE.hpp"


extern "C" void app_main(void) {
    // Initialize NVS — required for Wi‑Fi.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    
    RaptPillBLE scanner;
    scanner.init();
    scanner.startScan();

    // Create and initialize the server.
    RaptMateServer raptMateServer(&scanner);
    raptMateServer.init();

    // The main task can now wait forever.
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}