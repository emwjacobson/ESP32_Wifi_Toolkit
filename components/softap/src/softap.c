#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_err.h"

void init_ap() {
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
}

esp_err_t start_ap(const char ssid[], const char password[]) {
    wifi_config_t wifi_config = {
        .ap = {
            .channel = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .ssid_hidden = 0,
            .max_connection = 1,
            .beacon_interval = 1000
        }
    };
    memcpy(wifi_config.ap.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.ap.password, password, strlen(password));
    wifi_config.ap.ssid_len = strlen(ssid);

    esp_err_t err;
    if ((err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config)) != ESP_OK){
        return err;
    }

    if ((err = esp_wifi_start()) != ESP_OK) {
        return err;
    }

    return ESP_OK;
}

void stop_ap() {
    esp_wifi_stop();
}

void deinit_ap() {
    ESP_ERROR_CHECK(esp_netif_deinit());
}