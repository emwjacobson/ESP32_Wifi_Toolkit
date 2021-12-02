#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "softap.h"
#include "attack.h"

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

void app_main() {
    init();

    softap_start("Boonie", "BeenieWeenie");
    printf("AP Started\n");

    printf("Waiting...\n");
    vTaskDelay(30000 / portTICK_PERIOD_MS);

    printf("Enabling promiscuous mode\n");
    softap_promiscuous_enable();
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    printf("Disabling promiscuous mode\n");
    softap_promiscuous_disable();

    // printf("Starting deauth attack\n");
    // attack_deauth_start((mac_addr_t){ 0xde, 0xad, 0xde, 0xad, 0xde, 0xad }, 100);
    // vTaskDelay(5000 / portTICK_PERIOD_MS);
    // printf("Ending deauth attack..");
    // attack_deauth_stop();
    // printf("I should be able to chill now...");
}
