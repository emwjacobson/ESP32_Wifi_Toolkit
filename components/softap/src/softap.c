#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"

#define NUM_SSIDS 16

static const char* TAG = "SoftAP";

void softap_init() {
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_LOGI(TAG, "Initialized");
}

void softap_deinit() {
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_LOGI(TAG, "Uninitialized");
}

void promis_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* packet = (wifi_promiscuous_pkt_t*)buf;
    ESP_LOGI(TAG, "GOT PACKET Channel: %i", packet->rx_ctrl.channel);
}

void softap_promiscuous_enable() {
    ESP_LOGI(TAG, "Enabling promiscuous mode");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(promis_cb));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_LOGI(TAG, "Promiscuous mode enabled");
}

void softap_promiscuous_disable() {
    ESP_LOGI(TAG, "Disabling promiscuous mode");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    ESP_LOGI(TAG, "Promiscuous mode disabled");
}

uint16_t softap_scan_ssids() {
    ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, true));
    uint16_t num_aps;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&num_aps));
    ESP_LOGI(TAG, "Scanned %i APs", num_aps);
    return num_aps;
}

void softap_get_scanned_ssids(wifi_ap_record_t ap_list[], uint16_t* num_aps) {
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(num_aps, ap_list));
}

esp_err_t softap_start(const char ssid[], const char password[]) {
    ESP_LOGI(TAG, "Starting AP");
    wifi_config_t wifi_config = {
        .ap = {
            .channel = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .ssid_hidden = 0,
            .max_connection = 5,
            .beacon_interval = 1000
        }
    };
    memcpy(wifi_config.ap.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.ap.password, password, strlen(password));
    wifi_config.ap.ssid_len = strlen(ssid);

    esp_err_t err;
    if ((err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config)) != ESP_OK){
        ESP_LOGE(TAG, "Unable to initialize SoftAP config");
        ESP_LOGE(TAG, "Error: %s", esp_err_to_name(err));
        return err;
    }

    if ((err = esp_wifi_start()) != ESP_OK) {
        ESP_LOGE(TAG, "Unable to start SoftAP");
        ESP_LOGE(TAG, "Error: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Started");
    return ESP_OK;
}

void softap_stop() {
    esp_wifi_stop();
    ESP_LOGI(TAG, "Stopped");
}