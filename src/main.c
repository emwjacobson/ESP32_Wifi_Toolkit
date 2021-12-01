#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "softap.h"

// extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
//     return 0;
// }

// uint8_t deauth_packet[] = {
//     0xc0, 0x00, // Frame Control
//     0x00, 0x00, // Duration
//     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination
//     0xda, 0xdd, 0xad, 0x01, 0x02, 0x03, // Source
//     0xda, 0xdd, 0xad, 0x01, 0x02, 0x03, // BSS ID
//     0xf0, 0xff, // Sequence seq[8:11]:frag, seq[0:7]
//     0x02, 0x00, // Reason
// };

// #define BEACON_SSID_OFFSET 38
// #define SRCADDR_OFFSET 10
// #define BSSID_OFFSET 16
// #define SEQNUM_OFFSET 22
// uint8_t beacon_frame[] = {
// 	0x80, 0x00,							// 0-1: Frame Control
// 	0x00, 0x00,							// 2-3: Duration
// 	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,				// 4-9: Destination address (broadcast)
// 	0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06,				// 10-15: Source address
// 	0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06,				// 16-21: BSSID
// 	0x00, 0x00,							// 22-23: Sequence / fragment number
// 	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,			// 24-31: Timestamp (GETS OVERWRITTEN TO 0 BY HARDWARE)
// 	0x64, 0x00,							// 32-33: Beacon interval
// 	0x31, 0x04,							// 34-35: Capability info
// 	0x00, 0x00, /* FILL CONTENT HERE */				// 36-38: SSID parameter set, 0x00:length:content
// 	0x01, 0x08, 0x82, 0x84,	0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24,	// 39-48: Supported rates
// 	0x03, 0x01, 0x01,						// 49-51: DS Parameter set, current channel 1 (= 0x01),
// 	0x05, 0x04, 0x01, 0x02, 0x00, 0x00,				// 52-57: Traffic Indication Map
// };

void init() {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    init_ap();
}

void app_main() {

    init();
    printf("Starting AP...\n");

    for(;;) {
        start_ap("IllTake", "password123");
        printf("AP Started, waiting 30 seconds\n");
        vTaskDelay(30000 / portTICK_PERIOD_MS);
        printf("Stopping AP\n");
        stop_ap();

        printf("AP Stopped, waiting 30 seconds\n");
        start_ap("TheGabagool", "password123");
        vTaskDelay(30000 / portTICK_PERIOD_MS);
        printf("Stopping AP\n");
        stop_ap();
    }

    // esp_err_t ret;

    // ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    // uint16_t seqnum = 0;
    // uint8_t beacon_rick[200];
    // memcpy(beacon_rick, beacon_frame, BEACON_SSID_OFFSET - 1);
    // beacon_rick[BEACON_SSID_OFFSET - 1] = strlen("POOPYDOOPIE");
    // memcpy(&beacon_rick[BEACON_SSID_OFFSET], "POOPYDOOPIE", strlen("POOPYDOOPIE"));
    // memcpy(&beacon_rick[BEACON_SSID_OFFSET + strlen("POOPYDOOPIE")], &beacon_frame[BEACON_SSID_OFFSET], sizeof(beacon_frame) - BEACON_SSID_OFFSET);

    // for (;;) {
    //     beacon_rick[SEQNUM_OFFSET] = (seqnum & 0x0f) << 4;
    //     beacon_rick[SEQNUM_OFFSET + 1] = (seqnum & 0xff0) >> 4;
    //     seqnum++;
    //     if (seqnum > 0xfff) {
    //         seqnum = 0;
    //     }

    //     ret = esp_wifi_80211_tx(WIFI_IF_AP, beacon_rick, sizeof(beacon_frame) + strlen("POOPYDOOPIE"), false);
    //     // printf("Return code: %i\n", ret);
    //     // vTaskDelay(100 / portTICK_RATE_MS);
    // }

    // // for(;;) {
    // //     ret = esp_wifi_80211_tx(WIFI_IF_AP, deauth_packet, sizeof(deauth_packet), true);
    // //     // printf("Return code: %i\n", ret);
    // //     // vTaskDelay(10 / portTICK_RATE_MS);
    // // }

    // printf("IDF_VER: " IDF_VER);
}
