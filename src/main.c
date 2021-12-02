#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "softap.h"
#include "attack.h"

static const char* TAG = "Main";

void init() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    softap_init();
    attack_init();
}

void deinit() {
    attack_deinit();
    softap_deinit();
    ESP_ERROR_CHECK(esp_netif_deinit());
}

// ESP_LOGE - error (lowest)
// ESP_LOGW - warning
// ESP_LOGI - info
// ESP_LOGD - debug
// ESP_LOGV - verbose (highest)

void app_main() {
    init();
    softap_start("Boonie", "BeenieWeenie");

    attack_beacon_spam_start();
}
