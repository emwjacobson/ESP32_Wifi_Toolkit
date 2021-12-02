#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_err.h"

void softap_init() {
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
}

void softap_deinit() {
    ESP_ERROR_CHECK(esp_wifi_deinit());
}

void promis_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    printf("Got packet\n");
}

void softap_promiscuous_enable() {
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(promis_cb));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
}

void softap_promiscuous_disable() {
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
}

esp_err_t softap_start(const char ssid[], const char password[]) {
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

void softap_stop() {
    esp_wifi_stop();
}