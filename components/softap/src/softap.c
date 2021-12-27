#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"

#include "sd_card.h"

typedef struct {
    uint32_t magic_number;   /* magic number: 0xa1b2c3d4 */
    uint16_t version_major;  /* major version number: 2 */
    uint16_t version_minor;  /* minor version number: 4 */
    int32_t  thiszone;       /* GMT to local correction: 0 */
    uint32_t sigfigs;        /* accuracy of timestamps: 0 */
    uint32_t snaplen;        /* max length of captured packets, in octets: 65535 */
    uint32_t network;        /* data link type: 105(?) */
} pcap_hdr_t;

// https://wiki.wireshark.org/Development/LibpcapFileFormat
typedef struct {
    uint32_t ts_sec;         /* timestamp seconds */
    uint32_t ts_usec;        /* timestamp microseconds */
    uint32_t incl_len;       /* number of octets of packet saved in file */
    uint32_t orig_len;       /* actual length of packet */
} pcaprec_hdr_t;

QueueHandle_t packet_queue;
TaskHandle_t packet_task;

static const char* TAG = "SoftAP";

void softap_init() {
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    packet_queue = xQueueCreate(16, sizeof(wifi_promiscuous_pkt_t*));
    ESP_LOGI(TAG, "Initialized");
}

void softap_deinit() {
    ESP_ERROR_CHECK(esp_wifi_deinit());
    vQueueDelete(packet_queue);
    ESP_LOGI(TAG, "Uninitialized");
}

// **********************
// Start Packet Capture *
// **********************

void packet_runner() {
    wifi_promiscuous_pkt_t* packet;
    pcaprec_hdr_t pcap_packet;
    while(true) {
        if (xQueueReceive(packet_queue, &packet, portMAX_DELAY)) {
            // 1 second = 1000 ms = 1000000 us
            pcap_packet.ts_sec = packet->rx_ctrl.timestamp / 1000000;
            pcap_packet.ts_usec = packet->rx_ctrl.timestamp % 1000000;
            pcap_packet.incl_len = packet->rx_ctrl.sig_len;
            pcap_packet.orig_len = packet->rx_ctrl.sig_len;

            // ESP_LOGI(TAG, "Processed Packet Length: %i", packet->rx_ctrl.sig_len);
            ESP_LOGI(TAG, "Processed Packet Length: %032u", packet->rx_ctrl.timestamp);
            free(packet);
        }
    }
}

void promis_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* packet = (wifi_promiscuous_pkt_t*)buf;

    // Need to create a new copy to be able to preserve it
    wifi_promiscuous_pkt_t* payload = malloc(sizeof(wifi_promiscuous_pkt_t) + packet->rx_ctrl.sig_len);
    memcpy(payload, packet, sizeof(wifi_promiscuous_pkt_t) + packet->rx_ctrl.sig_len);

    xQueueSendToBack(packet_queue, (void*)&payload, 0);
    ESP_LOGD(TAG, "Queued packet length: %i", payload->rx_ctrl.sig_len);
}

void softap_promiscuous_enable() {
    ESP_LOGI(TAG, "Enabling promiscuous mode");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(promis_cb));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    
    pcap_hdr_t pcap_header = {
        .magic_number = 0xa1b2c3d4,
        .version_major = 2,
        .version_minor = 4,
        .thiszone = 0,
        .sigfigs = 0,
        .snaplen = 65535,
        .network = 105
    };
    // TODO: Write header to file

    ESP_LOGI(TAG, "Logged header");
    xTaskCreate(packet_runner, "Packet", 2048, NULL, tskIDLE_PRIORITY, &packet_task);
    ESP_LOGI(TAG, "Promiscuous mode enabled");
}

void softap_promiscuous_disable() {
    ESP_LOGI(TAG, "Disabling promiscuous mode");
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));
    if (packet_task != NULL) vTaskDelete(packet_task);

    // We need to clear the queue to prevent memory leaks!
    wifi_promiscuous_pkt_type_t* packet;
    while (xQueueReceive(packet_queue, &packet, 0)) {
        ESP_LOGI(TAG, "Cleared element from queue");
        free(packet);
    }

    ESP_LOGI(TAG, "Promiscuous mode disabled");
}

// **********************
// End Packet Capture *
// **********************


// *****************
// Start SSID Scan *
// *****************

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

// ***************
// End SSID Scan *
// ***************


// **************
// Start SoftAP *
// **************

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

// ************
// End SoftAP *
// ************
