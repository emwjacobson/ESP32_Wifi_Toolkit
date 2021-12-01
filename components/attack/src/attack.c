#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "frames.h"

// SoftAP should be initialized before running this.
void attack_init() {}

void send_beacon(const char str[]) {
  uint8_t frame[200];
  memset(frame, 0, 200);
  // Copy initial beacon frame
  memcpy(frame, beacon_frame, BEACON_SSID_LENGTH_OFFSET);
  frame[BEACON_SSID_LENGTH_OFFSET] = strlen(str);
  memcpy(frame + BEACON_SSID_OFFSET, str, strlen(str));
  memcpy(frame + BEACON_SSID_OFFSET + strlen(str), beacon_frame + BEACON_SSID_END_OFFSET, sizeof(beacon_frame) - BEACON_SSID_END_OFFSET);

  // TODO:
  // Set SA
  // Set BSSID

  // size = sizeof(beacon_frame) + strlen(str)
  esp_wifi_80211_tx(WIFI_IF_AP, frame, sizeof(frame) + strlen(frame), false);
}

void attack_beacon(const char str[]) {
  send_beacon(str);
}
