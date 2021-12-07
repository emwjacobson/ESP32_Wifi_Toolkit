#include <string.h>
#include "freertos/FreeRTOS.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "softap.h"
#include "webserver.h"
#include "attack.h"

static const char* TAG = "Main";

void init() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());
    softap_init();
    webserver_init();
    attack_init();
}

void deinit() {
    attack_deinit();
    webserver_deinit();
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
    softap_start(CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
    webserver_start();

    ip_addr_t target_addr = {
        .type = IPADDR_TYPE_V4,
        .u_addr = {
            .ip4 = {
                .addr = ESP_IP4TOADDR(192,168,4,0)
            }
        }
    };
    attack_ip_scan_start(target_addr, 24);

    // while (attack_ip_scan_in_progress() == pdTRUE) {
    //     ESP_LOGI(TAG, "Attack still in progress....");
    //     vTaskDelay(5000 / portTICK_PERIOD_MS);
    // }

    ESP_LOGI(TAG, "Attack still in progress....");
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    ESP_LOGI(TAG, "Attack is done!!");
    attack_ip_scan_stop();
}
