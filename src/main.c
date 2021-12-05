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
    // webserver_start();

    ip_addr_t target_addr;
    memset(&target_addr, 0, sizeof(target_addr));
    struct in_addr addr4 = {
        .s_addr = 0xc0a80400
    };
    inet_addr_to_ip4addr(ip_2_ip4(&target_addr), &addr4);
    
    attack_ip_scan(target_addr, 24);
    ESP_LOGI(TAG, "\n\n%s\n", inet_ntoa(target_addr));
}
